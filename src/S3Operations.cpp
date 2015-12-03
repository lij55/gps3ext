#include "S3Operations.h"

#include <algorithm>  // std::min
#include <sstream>
#include <iostream>

#include "utils.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <string>

static size_t put_read_callback(void *ptr, size_t size, size_t nmemb,
                                void *stream) {
    ptr = stream;

    return 0;
}

bool PutS3Object(const char *host, const char *bucket, const char *url,
                 const S3Credential &cred, const char *data,
                 const uint64_t data_size) {
    char size_str[256];

    CURL *curl = curl_easy_init();
    if (curl) {
        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, put_read_callback);

        /* enable uploading */
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* HTTP PUT please */
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);

        /* specify target URL, and note that this URL should include a file
           name, not only a directory */
        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* now specify which file/data to upload */
        curl_easy_setopt(curl, CURLOPT_READDATA, data);

        /* provide the size of the upload, we specicially typecast the value
           to curl_off_t since we must be sure to use the correct data size */
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)data_size);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
    } else {
        return false;
    }

    std::stringstream sstr;
    HeaderContent *header = new HeaderContent();

    sstr << bucket << ".s3.amazonaws.com";
    header->Add(HOST, host);
    // MIME type doesn't matter actually, server wouldn't store it either
    header->Add(CONTENTTYPE, "text/plain");
    header->Add(EXPECT, "100-continue");
    header->Add(CONTENTLENGTH, std::to_string(data_size));
    UrlParser p(url);
    SignPUTv2(header, p.Path(), cred);

    struct curl_slist *chunk = header->GetList();

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode ret = curl_easy_perform(curl);
    if (ret != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(ret));

    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    return ret;
}

bool DeleteS3Object(const char *host, const char *bucket, const char *url,
                    const S3Credential &cred) {
    CURL *curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
    } else {
        return false;
    }

    std::stringstream sstr;
    HeaderContent *header = new HeaderContent();

    sstr << bucket << ".s3.amazonaws.com";
    header->Add(HOST, host);
    UrlParser p(url);
    SignDELETEv2(header, p.Path(), cred);

    struct curl_slist *chunk = header->GetList();

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode ret = curl_easy_perform(curl);
    if (ret != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(ret));

    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    return ret;
}
