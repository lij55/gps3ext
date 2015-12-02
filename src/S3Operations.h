#ifndef __S3_OPERATIONS__
#define __S3_OPERATIONS__

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

bool PutS3Object(const char *host, const char *bucket, const char *url,
                 const S3Credential &cred, const char *file);

bool DeleteS3Object(const char *host, const char *bucket, const char *url,
                    const S3Credential &cred);

#endif
