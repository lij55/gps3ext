#ifndef __S3UPLOADER_H__
#define __S3UPLOADER_H__

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

struct Uploader {
    Uploader();
    ~Uploader();

    bool init(const char* data, S3Credential* cred);
    bool write(char* buf, uint64_t& len);
    void destroy();

   private:
    // pthread_t* threads;
};

#endif
