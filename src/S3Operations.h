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

const char *GetUploadId(const char *host, const char *bucket,
                        const char *obj_name, const S3Credential &cred);

const char *PartPutS3Object(const char *host, const char *bucket,
                            const char *obj_name, const S3Credential &cred,
                            const char *data, uint64_t data_size,
                            uint64_t part_number, const char *upload_id);

bool CompleteMultiPutS3(const char *host, const char *bucket,
                        const char *obj_name, const char *upload_id,
                        const char **etag_array, uint64_t count,
                        const S3Credential &cred);

#endif
