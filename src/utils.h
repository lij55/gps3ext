#ifndef _UTILFUNCTIONS_
#define _UTILFUNCTIONS_

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include "ini.h"
bool gethttpnow(char datebuf[65]);

bool lowercase(char* out, const char* in);
void _tolower(char* buf);

bool trim(char* out, const char* in, const char* trimed = " \t\r\n");

//! return value is malloced
char* Base64Encode(const char* buffer, size_t length);

bool sha256(char* string, char outputBuffer[65]);

char* sha1hmac(const char* str, const char* secret);

bool sha256hmac(char* str, char out[65], char* secret);

// return malloced because Base64Encode
char* SignatureV2(const char* date, const char* path, const char* key);
char* SignatureV4(const char* date, const char* path, const char* key);

#ifdef DEBUGS3
void InitLog();
#endif

#include <string>
using std::string;

#include <openssl/md5.h>

size_t find_Nth(const string& str,  // where to work
                unsigned N,         // N'th ocurrence
                const string& find  // what to 'find'
                );

class MD5Calc {
   public:
    MD5Calc();
    ~MD5Calc(){};
    bool Update(const char* data, int len);
    const char* Get();

   private:
    MD5_CTX c;
    unsigned char md5[17];
    string result;
};

#include <mutex>
#include <queue>
#include <condition_variable>
using std::mutex;
using std::condition_variable;
using std::queue;
using std::unique_lock;

template <typename Data>
class concurrent_queue {
   private:
    queue<Data> _q;
    mutable mutex _m;
    condition_variable _c;

   public:
    void enQ(Data const& data) {
        _m.lock();
        bool isEmpty = _q.empty();
        _q.push(data);
        if (isEmpty) {
            _c.notify_all();
        }
        _m.unlock();
    }

    void deQ(Data& popped_value) {
        unique_lock<mutex> lk(_m);
        while (_q.empty()) {
            _c.wait(lk);
        }
        popped_value = _q.front();
        _q.pop();
    }
};

class DataBuffer {
   public:
    DataBuffer(uint64_t size);
    ~DataBuffer();
    void reset() { length = 0; };

    uint64_t append(const char* buf, uint64_t len);  // ret < len means full
    const char* getdata() { return data; };
    uint64_t len() { return this->length; };
    bool full() { return maxsize == length; };
    bool empty() { return 0 == length; };

   private:
    const uint64_t maxsize;
    uint64_t length;
    // uint64_t offset;
    char* data;
};

class Config {
   public:
    Config(const char* filename);
    ~Config();
    const char* Get(const char* sec, const char* key, const char* defaultvalue);
    bool Scan(const char* sec, const char* key, const char* scanfmt, void* dst);

   private:
    ini_t* _conf;
};

bool to_bool(std::string str);

#endif  // _UTILFUNCTIONS_
