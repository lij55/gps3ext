#include "S3Uploader.h"

#include <algorithm>  // std::min
#include <sstream>
#include <iostream>

#include "utils.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

bool Uploader::init(const char *data, S3Credential *cred) {
    // char *url = //TODO;

    return true;
}

Uploader::Uploader() {
    // fork //TODO

    // PutS3Object(host, bucket, url, cred, data);
}

Uploader::~Uploader() {
}

void Uploader::destroy() {
}

bool Uploader::write(char *data, uint64_t &len) {
}
