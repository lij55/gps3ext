#ifndef __S3_EXT_WRAPPER__
#define __S3_EXT_WRAPPER__

#include "S3Downloader.h"
#include <string>

using std::string;

struct S3Protocol_t {
    S3Protocol_t(const char* url);
    virtual ~S3Protocol_t();
    virtual bool Init(int segid, int segnum);
    virtual bool Get(char* data, size_t& len);
    virtual bool Destroy();

   private:
    S3Credential cred;
    string schema;
    string bucket;
    string prefix;
    string region;

    int segid;
    int contentindex;
    int segnum;
    int paranum;
    string url;
    Downloader* filedownloader;
    ListBucketResult* keylist;
    string getKeyURL(const string& key);
    void getNextDownloader();
};

extern "C" S3Protocol_t* CreateExtWrapper(const char* url);

#endif
