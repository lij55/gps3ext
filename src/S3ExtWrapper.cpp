#include "S3ExtWrapper.h"
#include "utils.h"
#include "gps3conf.h"
#include "gps3ext.h"

#include "sstream"
using std::stringstream;

S3ExtBase *CreateExtWrapper(const char *url) {
    S3ExtBase *ret = new S3Reader(url);
    return ret;
}

S3ExtBase::S3ExtBase(const char *url) {
    this->url = url;

    // get following from config
    this->cred.secret = s3ext_secret;
    this->cred.keyid = s3ext_accessid;

    this->segid = s3ext_segid;
    this->segnum = s3ext_segnum;

    this->chunksize = s3ext_chunksize;
    this->concurrent_num = s3ext_threadnum;

    // this->concurrent_num = 2;
    // this->cred.secret = "oCTLHlu3qJ+lpBH/+JcIlnNuDebFObFNFeNvzBF0";
    // this->cred.keyid = "AKIAIAFSMJUMQWXB2PUQ";

    // this->segid = -1;
    // this->segnum = -1;
    // this->chunksize = -1;
}

S3ExtBase::~S3ExtBase() {}

S3Reader::~S3Reader() {}

S3Reader::S3Reader(const char *url) : S3ExtBase(url) {
    this->contentindex = -1;
    this->filedownloader = NULL;
    this->keylist = NULL;
}

bool S3Reader::Init(int segid, int segnum, int chunksize) {
    // set segment id and num
    this->segid = s3ext_segid;
    this->segnum = s3ext_segnum;
    this->contentindex = this->segid;

    this->chunksize = chunksize;

    // Validate url first
    if (!this->ValidateURL()) {
        S3ERROR("validate url fail %s", this->url.c_str());
        return false;
    }

    // TODO: As separated function for generating url
    stringstream sstr;
    sstr << "s3-" << this->region << ".amazonaws.com";
    S3DEBUG("host url is %s", sstr.str().c_str());

    this->keylist = ListBucket(sstr.str().c_str(), this->bucket.c_str(),
                               this->prefix.c_str(), this->cred);

    if (!this->keylist) {
        S3ERROR("Can't get key list from bucket %s", this->bucket.c_str());
        return false;
    }

    if(this->keylist->contents.size() == 0) {
        S3ERROR("key list is empty");
        return false;
    }

    S3DEBUG("file list size is %d", this->keylist->contents.size());
    this->getNextDownloader();

    return this->filedownloader ? true : false;
}

void S3Reader::getNextDownloader() {
    if (this->filedownloader) {  // reset old downloader
        filedownloader->destroy();
        delete this->filedownloader;
        this->filedownloader = NULL;
    }

    if (this->contentindex >= this->keylist->contents.size() ) {
        S3DEBUG("no more file to download");
        return;
    }
    this->filedownloader = new Downloader(this->concurrent_num);

    if (!this->filedownloader) {
        S3ERROR("Create filedownloader fail");
        return;
    }
    BucketContent *c = this->keylist->contents[this->contentindex];
    string keyurl = this->getKeyURL(c->Key());
    S3DEBUG("%s:%lld", keyurl.c_str(), c->Size());

    if (!filedownloader->init(keyurl.c_str(), c->Size(), this->chunksize,
                              &this->cred)) {
        delete this->filedownloader;
        this->filedownloader = NULL;
    } else {  // move to next file
        this->contentindex += this->segnum;
    }
    return;
}

string S3Reader::getKeyURL(const string &key) {
    stringstream sstr;
    sstr << this->schema << "://"
         << "s3-" << this->region << ".amazonaws.com/";
    sstr << this->bucket << "/" << key;
    return sstr.str();
}

bool S3Reader::TransferData(char *data, size_t &len) {
    if (!this->filedownloader) {
        // not initialized?
        return false;
    }
    uint64_t buflen;
RETRY:
    buflen = len;
    // S3DEBUG("getlen is %d", len);
    bool result = filedownloader->get(data, buflen);
    if (!result) {  // read fail
        S3ERROR("get data from filedownloader fail");
        return false;
    }
    // S3DEBUG("getlen is %lld", buflen);
    if (buflen == 0) {
        // change to next downloader
        this->getNextDownloader();
        if (this->filedownloader) {  // download next file
            S3INFO("retry");
            goto RETRY;
        }
    }
    len = buflen;
    return true;
}

bool S3Reader::Destroy() {
    // reset filedownloader
    if (this->filedownloader) {
        this->filedownloader->destroy();
        delete this->filedownloader;
    }

    // Free keylist
    if (this->keylist) {
        delete this->keylist;
    }
    return true;
}

bool S3ExtBase::ValidateURL() {
    const char *awsdomain = ".amazonaws.com";
    int ibegin = 0;
    int iend = url.find("://");
    if (iend == string::npos) {  // -1
        // error
        return false;
    }
    
    this->schema = url.substr(ibegin, iend);
    if(this->schema == "s3") {
        this->schema = "http";
    }

    ibegin = url.find("-");
    iend = url.find(awsdomain);
    if ((iend == string::npos) || (ibegin == string::npos)) {
        return false;
    }
    this->region = url.substr(ibegin + 1, iend - ibegin - 1);

    ibegin = find_Nth(url, 3, "/");
    iend = find_Nth(url, 4, "/");
    if ((iend == string::npos) || (ibegin == string::npos)) {
        return false;
    }
    this->bucket = url.substr(ibegin + 1, iend - ibegin - 1);

    this->prefix = url.substr(iend + 1, url.length() - iend - 1);

    /*
    if (url.back() != '/') {
        return false;
    }
    */
    return true;
}

/*
bool S3Protocol_t::Write(char *data, size_t &len) {
    if (!this->fileuploader) {
        // not initialized?
        return false;
    }
    S3DEBUG("write_len is %d", len);

    bool result = fileuploader->write(data, len);
    if (!result) {
        S3DEBUG("write data via fileuploader fail");
        return false;
    }
    S3DEBUG("write_len is %ld", len);
    return true;
}
*/
