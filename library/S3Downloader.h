#ifndef __S3DOWNLOADER_H__
#define __S3DOWNLOADER_H__

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include <iostream>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>

using std::vector;


#include <curl/curl.h>

#include "S3Common.h"

struct Range
{
    /* data */
    uint64_t offset;
    uint64_t len;
};

class OffsetMgr {
public:
    OffsetMgr(uint64_t maxsize, uint64_t chunksize);
    ~OffsetMgr() {
        pthread_mutex_destroy(&this->offset_lock);
    };
    Range NextOffset(); // ret.len == 0 means EOF
    void Reset(uint64_t n);
    uint64_t Chunksize() {
        return this->chunksize;
    };
    uint64_t Size() {
        return this->maxsize;
    };
private:
    pthread_mutex_t offset_lock;
    uint64_t maxsize;
    uint64_t chunksize;
    uint64_t curpos;
};


class BlockingBuffer
{
public:
    static BlockingBuffer* CreateBuffer(const char* url, OffsetMgr* o, S3Credential* pcred);
    BlockingBuffer(const char* url,OffsetMgr* o);
    virtual ~BlockingBuffer();
    bool Init();
    bool EndOfFile() {
        return this->eof;
    };
    bool Error() {
        return this->error;
    };

    uint64_t Read(char* buf, uint64_t len);
    uint64_t Fill();

    static const int STATUS_EMPTY = 0;
    static const int STATUS_READY = 1;

    /* data */
protected:
    const char* sourceurl;
    uint64_t bufcap;
    virtual uint64_t fetchdata(uint64_t offset, char* data, uint64_t len) = 0;
private:
    int status;
    bool eof;
    bool error;
    pthread_mutex_t stat_mutex;
    pthread_cond_t   stat_cond;
    uint64_t readpos;
    uint64_t realsize;
    char* bufferdata;
    OffsetMgr* mgr;
    Range nextpos;
};


struct Downloader {
    Downloader(uint8_t part_num);
    ~Downloader();
    bool init(const char* url, uint64_t size, uint64_t chunksize, S3Credential* pcred);
    bool get(char* buf, uint64_t& len);
    void destroy();
    //reset
    // init(url)
private:
    const uint8_t num;
    pthread_t* threads;
    BlockingBuffer** buffers;
    OffsetMgr* o;
    uint8_t chunkcount;
    uint64_t readlen;
};

struct Bufinfo
{
    /* data */
    char* buf;
    uint64_t maxsize;
    uint64_t len;
};


class HTTPFetcher : public BlockingBuffer
{
public:
    HTTPFetcher(const char* url, OffsetMgr* o);
    ~HTTPFetcher();
    bool SetMethod(Method m);
    bool AddHeaderField(HeaderField f, const char* v);
protected:
    uint64_t fetchdata(uint64_t offset, char* data, uint64_t len);
    virtual bool processheader() {
        return true;
    };
    virtual bool retry(CURLcode c) {
        return false;
    };
    CURL *curl;
    Method method;
    HeaderContent headers;
    UrlParser urlparser;

};

class S3Fetcher : public HTTPFetcher
{
public:
    S3Fetcher(const char* url, OffsetMgr* o,const S3Credential& cred);
    ~S3Fetcher() {};
protected:
    virtual bool processheader();
    virtual bool retry(CURLcode c);
private:
    S3Credential cred;
};

struct BucketContent;

struct ListBucketResult
{
    const char* Name;
    const char* Prefix;
    unsigned int MaxKeys;
    vector<BucketContent*> contents;
};

BucketContent* CreateBucketContentItem(const char* key, uint64_t size);

struct BucketContent
{
    friend BucketContent* CreateBucketContentItem(const char* key, uint64_t size);
    BucketContent();
    ~BucketContent();
    const char* Key() const {
        return this->key;
    };
    uint64_t Size() const {
        return this->size;
    };
private:
    BucketContent(const BucketContent& b) {};
    BucketContent operator=(const BucketContent& b) {};

    const char* key;
    //const char* etags;
    uint64_t size;

};

// need free
ListBucketResult*  ListBucket(const char* host, const char* bucket, const char* path, const S3Credential &cred);




#endif
