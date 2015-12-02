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

bool PutS3Object(const char *host, const char *bucket, const char *url,
                 const S3Credential &cred, const char *file) {
    struct stat file_info;
    FILE *hd_src;
    char size_str[256];

    /* get the file size of the local file */
    stat(file, &file_info);

    /* get a FILE * of the same file, could also be made with
       fdopen() from the previous descriptor, but hey this is just
       an example! */
    hd_src = fopen(file, "rb");

    CURL *curl = curl_easy_init();
    if (curl) {
        /* enable uploading */
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* HTTP PUT please */
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);

        /* specify target URL, and note that this URL should include a file
           name, not only a directory */
        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* now specify which file to upload */
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

        /* provide the size of the upload, we specicially typecast the value
           to curl_off_t since we must be sure to use the correct data size */
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                         (curl_off_t)file_info.st_size);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
    } else {
        return false;
    }

    std::stringstream sstr;
    HeaderContent *header = new HeaderContent();

    sstr << bucket << ".s3.amazonaws.com";
    header->Add(HOST, host);
    // TODO detect the real file type, libmagic?
    header->Add(CONTENTTYPE, "text/plain");
    header->Add(EXPECT, "100-continue");
    header->Add(CONTENTLENGTH, std::to_string(file_info.st_size));
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
    fclose(hd_src); /* close the local file */

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
