#ifndef __S3_EXT_WRAPPER__
#define __S3_EXT_WRAPPER__

#include "S3Downloader.h"
#include "S3Uploader.h"
#include <string>

using std::string;

enum s3protocol_purpose {
    TO_GET,
    TO_WRITE,
};

struct S3Protocol_t {
    S3Protocol_t(const char* url);
    virtual ~S3Protocol_t();
    virtual bool Init(int segid, int segnum,
                      enum s3protocol_purpose get_or_write);
    virtual bool Get(char* data, size_t& len);
    virtual bool Write(char* data, size_t& len);
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
    Uploader* fileuploader;
    ListBucketResult* keylist;

    string getKeyURL(const string& key);
    void getNextDownloader();
    bool ValidateURL();
};

extern "C" S3Protocol_t* CreateExtWrapper(const char* url);

#endif
