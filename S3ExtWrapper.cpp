#include "S3ExtWrapper.h"

#include "sstream"
using std::stringstream;

S3Protocol_t* CreateExtData(const char* url) {
	S3Protocol_t *ret = new S3Protocol_t(url);
	return ret;
}

S3Protocol_t::S3Protocol_t(const char* url) {
	this->url = url;
	this->cred.secret = "oCTLHlu3qJ+lpBH/+JcIlnNuDebFObFNFeNvzBF0";
	this->cred.keyid = "AKIAIAFSMJUMQWXB2PUQ";
	this->bucket = "metro.pivotal.io";
	this->region = "us-west-2";
	this->prefix = "data/";
	this->schema = "http";

	this->paranum = 4; // get from config

	filedownloader = NULL;
	keylist = NULL;
	segid = -1;
	segnum = -1;
	contentindex = -1;
}


S3Protocol_t::~S3Protocol_t() {
	if(filedownloader)
		delete filedownloader;
	if(keylist)
		delete keylist;
}

string S3Protocol_t::getKeyURL(const string& key) {
	stringstream sstr;
	sstr<<this->schema<<"://"<<"s3-"<<this->region<<".amazonaws.com/";
	sstr<<this->bucket<<"/"<<key;
	return sstr.str();
}

Downloader * S3Protocol_t::getNextDownloader() {
	if(this->filedownloader) { // reset old downloader
		filedownloader->destroy();
		delete this->filedownloader;
		this->filedownloader = NULL;
	}
	
	this->contentindex += this->segnum;

	if(this->contentindex >= this->keylist->contents.size() ) {
		return NULL;
	}
	filedownloader = new Downloader(this->paranum);

	if(!filedownloader)
		return NULL;
	BucketContent* c = this->keylist->contents[this->contentindex];
	string keyurl = this->getKeyURL(c->Key());
	uint64_t chunksize = 5 * 1024 * 1024;
	if( !filedownloader->init(keyurl.c_str(), c->Size(), chunksize, &this->cred) ) {
		delete this->filedownloader;
		this->filedownloader = NULL;
	};

	return filedownloader;
}

bool S3Protocol_t::Init(int segid, int segnum) {
	// Validate url first

	// set segment id and num
	this->segid = 0;  // fake
	this->segnum = 1;  // fake
	this->contentindex = this->segid;

	// Create bucket file list
	stringstream sstr;
	sstr<<this->schema<<"://"<<"s3-"<<this->region<<".amazonaws.com/";
	
	this->keylist = ListBucket(sstr.str().c_str(), this->bucket.c_str(), this->prefix.c_str(), this->cred);
	if(!this->keylist)
		return false;

	filedownloader = this->getNextDownloader();
	if(!filedownloader)
		return false;
	else
		return true;
}

// len == 0 means EOF for GPDB
bool S3Protocol_t::Get(char* data, size_t& len) {
	if(!filedownloader) {
		// not initialized?
		return false;
	}
	uint64_t buflen;
 RETRY:
	buflen = len;
	bool result = filedownloader->get(data, buflen);
	if(!result) { // read fail
		return false;
	}

	if(buflen == 0) {
		// change to next downloader
		filedownloader = this->getNextDownloader();
		if(filedownloader) { // download next file
			goto RETRY;
		} 
	} 
	len = buflen;
	return true;
}


bool S3Protocol_t::Destroy() {
	// reset filedownloader
	// Free keylist
	return true;
}
