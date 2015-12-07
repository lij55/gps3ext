#include "S3ExtWrapper.h"
#include "utils.h"

#include "sstream"
using std::stringstream;

S3Protocol_t *CreateExtWrapper(const char *url) {
    S3Protocol_t *ret = new S3Protocol_t(url);
    return ret;
}

S3Protocol_t::S3Protocol_t(const char *url) {
    this->url = url;
    this->cred.secret = "oCTLHlu3qJ+lpBH/+JcIlnNuDebFObFNFeNvzBF0";
    this->cred.keyid = "AKIAIAFSMJUMQWXB2PUQ";
    this->bucket = "metro.pivotal.io";
    this->region = "us-west-2";
    this->prefix = "data/";
    this->schema = "http";

    this->paranum = 1;  // get from config

    filedownloader = NULL;
    fileuploader = NULL;
    keylist = NULL;
    segid = -1;
    segnum = -1;
    contentindex = -1;
}

S3Protocol_t::~S3Protocol_t() {
    if (filedownloader) delete filedownloader;
    if (fileuploader) delete filedownloader;
    if (keylist) delete keylist;
}

string S3Protocol_t::getKeyURL(const string &key) {
    stringstream sstr;
    sstr << this->schema << "://"
         << "s3-" << this->region << ".amazonaws.com/";
    sstr << this->bucket << "/" << key;
    return sstr.str();
}

void S3Protocol_t::getNextDownloader() {
    EXTLOG("download next file\n");
    if (this->filedownloader) {  // reset old downloader
        filedownloader->destroy();
        delete this->filedownloader;
        this->filedownloader = NULL;
    }

    if (this->contentindex >= this->keylist->contents.size()) {
        return;
    }
    this->filedownloader = new Downloader(this->paranum);

    if (!this->filedownloader) {
        EXTLOG("Create filedownloader fail");
        return;
    }
    BucketContent *c = this->keylist->contents[this->contentindex];
    string keyurl = this->getKeyURL(c->Key());
    EXTLOG("%s:%lld\n", keyurl.c_str(), c->Size());
    uint64_t chunksize = 5 * 1024 * 1024;
    if (!filedownloader->init(keyurl.c_str(), c->Size(), chunksize,
                              &this->cred)) {
        delete this->filedownloader;
        this->filedownloader = NULL;
    } else {  // move to next file
        this->contentindex += this->segnum;
    }
    return;
}

bool S3Protocol_t::Init(int segid, int segnum, enum s3protocol_purpose get_or_write) {
    // Validate url first

    // set segment id and num
    this->segid = segid;    // fake
    this->segnum = segnum;  // fake
    this->contentindex = this->segid;

    stringstream sstr;
    sstr << "s3-" << this->region << ".amazonaws.com";
    EXTLOG("%s\n", sstr.str().c_str());

    if (get_or_write == TO_GET) {
        // Create bucket file list
        this->keylist = ListBucket(sstr.str().c_str(), this->bucket.c_str(),
                                   this->prefix.c_str(), this->cred);

        if (!this->keylist) return false;

        this->getNextDownloader();

        if (!this->filedownloader) return false;
    } else if (get_or_write == TO_WRITE) {
        if (!this->fileuploader) return false;
    }

    return true;
}

// len == 0 means EOF for GPDB
bool S3Protocol_t::Get(char *data, size_t &len) {
    if (!this->filedownloader) {
        // not initialized?
        return false;
    }
    uint64_t buflen;
RETRY:
    buflen = len;
    EXTLOG("getlen is %d\n", len);
    bool result = filedownloader->get(data, buflen);
    if (!result) {  // read fail
        EXTLOG("get data from filedownloader fail\n");
        return false;
    }
    EXTLOG("getlen is %lld\n", buflen);
    if (buflen == 0) {
        // change to next downloader
        this->getNextDownloader();
        if (this->filedownloader) {  // download next file
            EXTLOG("retry\n");
            goto RETRY;
        }
    }
    len = buflen;
    return true;
}

bool S3Protocol_t::Write(char *data, size_t &len) {
    if (!this->fileuploader) {
        // not initialized?
        return false;
    }
    EXTLOG("write_len is %d\n", len);

    bool result = fileuploader->write(data, len);
    if (!result) {
        EXTLOG("write data via fileuploader fail\n");
        return false;
    }
    EXTLOG("write_len is %lld\n", len);
    return true;
}

bool S3Protocol_t::Destroy() {
    // reset filedownloader
    if (this->filedownloader) {
        this->filedownloader->destroy();
        delete this->filedownloader;
    }

    if (this->fileuploader) {
        this->fileuploader->destroy();
        delete this->fileuploader;
    }

    // Free keylist
    if (this->keylist) {
        delete this->keylist;
    }
    return true;
}
