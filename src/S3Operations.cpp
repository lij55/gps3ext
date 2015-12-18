#include "S3Operations.h"
#include "S3Common.h"
#include "S3Downloader.h"

#include <algorithm>  // std::min
#include <sstream>
#include <iostream>

#include "utils.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <string>
using std::string;
using std::stringstream;

struct MemoryData {
    char *data;
    size_t size;
};

static size_t mem_read_callback(void *ptr, size_t size, size_t nmemb,
                                void *userp) {
    struct MemoryData *puppet = (struct MemoryData *)userp;

    if (size * nmemb < 1) return 0;

    if (puppet->size) {
        *(char *)ptr = puppet->data[0]; /* copy one single byte */

        puppet->data++; /* advance pointer */
        puppet->size--; /* less data left */

        return 1; /* we return 1 byte at a time! */
    }

    return 0; /* no more data left to deliver */
}

static size_t header_write_callback(void *contents, size_t size, size_t nmemb,
                                    void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryData *puppet = (struct MemoryData *)userp;

    puppet->data = (char *)realloc(puppet->data, puppet->size + realsize + 1);
    if (puppet->data == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(puppet->data[puppet->size]), contents, realsize);
    puppet->size += realsize;
    puppet->data[puppet->size] = 0;

    return realsize;
}

// XXX need free
const char *GetUploadId(const char *host, const char *bucket,
                        const char *obj_name, const S3Credential &cred) {
    // POST /ObjectName?uploads HTTP/1.1
    // Host: BucketName.s3.amazonaws.com
    // Date: date
    // Authorization: authorization string (see Authenticating Requests (AWS
    // Signature Version 4))

    // HTTP/1.1 200 OK
    // x-amz-id-2: Uuag1LuByRx9e6j5Onimru9pO4ZVKnJ2Qz7/C1NPcfTWAtRPfTaOFg==
    // x-amz-request-id: 656c76696e6727732072657175657374
    // Date:  Mon, 1 Nov 2010 20:34:56 GMT
    // Content-Length: 197
    // Connection: keep-alive
    // Server: AmazonS3
    //
    // <?xml version="1.0" encoding="UTF-8"?>
    // <InitiateMultipartUploadResult
    // xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
    //   <Bucket>example-bucket</Bucket>
    //     <Key>example-object</Key>
    //       <UploadId>VXBsb2FkIElEIGZvciA2aWWpbmcncyBteS1tb3ZpZS5tMnRzIHVwbG9hZA</UploadId>
    //       </InitiateMultipartUploadResult>
    std::stringstream url;
    std::stringstream path_with_query;
    XMLInfo xml;
    xml.ctxt = NULL;

    if (!host || !bucket || !obj_name) return NULL;

    url << "http://" << host << "/" << bucket << "/" << obj_name;

    HeaderContent *header = new HeaderContent();
    header->Add(HOST, host);
    UrlParser p(url.str().c_str());
    path_with_query << p.Path() << "?uploads";
    SignPOSTv2(header, path_with_query.str().c_str(), cred);

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ParserCallback);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "uploads");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen("uploads"));
    } else {
        return NULL;
    }

    struct curl_slist *chunk = header->GetList();
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

    xmlParseChunk(xml.ctxt, "", 0, 1);
    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    xmlNode *root_element = xmlDocGetRootElement(xml.ctxt->myDoc);

    char *upload_id = NULL;
    xmlNodePtr cur = root_element->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"UploadId")) {
            upload_id = strdup((const char *)xmlNodeGetContent(cur));
            break;
        }

        cur = cur->next;
    }

    /* always cleanup */
    xmlFreeParserCtxt(xml.ctxt);

    return upload_id;
}

#if 0
const char *PartPutS3Object(const char *host, const char *bucket,
                            const char *obj_name, const S3Credential &cred,
                            const char *data, uint64_t data_size,
                            uint64_t part_number, const char *upload_id) {
    std::stringstream url;
    std::stringstream path_with_query;
    XMLInfo xml;
    xml.ctxt = NULL;
    struct MemoryData read_data = {data, data_size};
    struct MemoryData header_data = {malloc(1), 0};

    if (!host || !bucket || !obj_name) return NULL;

    url << "http://" << host << "/" << bucket << "/" << obj_name;

    // PUT /ObjectName?partNumber=PartNumber&uploadId=UploadId HTTP/1.1
    // Host: BucketName.s3.amazonaws.com
    // Date: date
    // Content-Length: Size
    // Authorization: authorization string

    url << "?partNumber=" << part_number << "&uploadId=" << upload_id;

    HeaderContent *header = new HeaderContent();
    header->Add(HOST, host);
    // MIME type doesn't matter actually, server wouldn't store it either
    header->Add(CONTENTTYPE, "text/plain");
    header->Add(CONTENTLENGTH, std::to_string(data_size));
    UrlParser p(url.str().c_str());
    // XXX to make SignXXXv2() generic
    path_with_query << p.Path() << "?partNumber=" << part_number
                    << "&uploadId=" << upload_id;
    SignPUTv2(header, path_with_query.str().c_str(), cred);

    CURL *curl = curl_easy_init();
    if (curl) {
        /* specify target URL, and note that this URL should include a file
           name, not only a directory */
        curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

        /* now specify which file/data to upload */
        curl_easy_setopt(curl, CURLOPT_READDATA, (void *)read_data);

        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, mem_read_callback);

        /* provide the size of the upload, we specicially typecast the value
           to curl_off_t since we must be sure to use the correct data size */
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)data_size);

        /* enable uploading */
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* HTTP PUT please */
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);

        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

        curl_easy_setopt(curl, CURLOPT_HEADERDATA, header_data);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_write_callback);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ParserCallback);
    } else {
        return NULL;
    }

    struct curl_slist *chunk = header->GetList();
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

    // to get the Etag from response
    // HTTP/1.1 200 OK
    // x-amz-id-2: Vvag1LuByRx9e6j5Onimru9pO4ZVKnJ2Qz7/C1NPcfTWAtRPfTaOFg==
    // x-amz-request-id: 656c76696e6727732072657175657374
    // Date:  Mon, 1 Nov 2010 20:34:56 GMT
    // ETag: "b54357faf0632cce46e942fa68356b38"
    // Content-Length: 0
    // Connection: keep-alive
    // Server: AmazonS3

    std::ostringstream out;
    out << header_data.data;

    // TODO general header content extracting func
    uint64_t etag_start_pos = out.str().find("ETag: ") + 6;
    std::string etag_to_end = out.str().substr(etag_start_pos);
    uint64_t etag_len = etag_to_end.find("\n") - 1;

    const char *etag = etag_to_end.substr(0, etag_len).c_str();

    if (etag) {
        curl_slist_free_all(chunk);
        curl_easy_cleanup(curl);
        return etag;
    }

    // <Error>
    //   <Code>AccessDenied</Code>
    //   <Message>Access Denied</Message>
    //   <RequestId>656c76696e6727732072657175657374</RequestId>
    //   <HostId>Uuag1LuByRx9e6j5Onimru9pO4ZVKnJ2Qz7/C1NPcfTWAtRPfTaOFg==</HostId>
    // </Error>
    xmlParseChunk(xml.ctxt, "", 0, 1);
    xmlNode *root_element = xmlDocGetRootElement(xml.ctxt->myDoc);

    char *response_code = NULL;
    xmlNodePtr cur = root_element->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"Code")) {
            response_code = xmlNodeGetContent(cur);
            break;
        }

        cur = cur->next;
    }

    if (response_code) {
        std::cout << response_code << std::endl;
    }

    xmlFreeParserCtxt(xml.ctxt);

    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    return NULL;
}

bool CompleteMultiPutS3(const char *host, const char *bucket,
                        const char *obj_name, const char *upload_id,
                        const char **etag_array, uint64_t count,
                        const S3Credential &cred) {
    std::stringstream url;
    std::stringstream query;
    std::stringstream path_with_query;
    XMLInfo xml;
    xml.ctxt = NULL;

    if (!host || !bucket || !obj_name) return NULL;

    url << "http://" << host << "/" << bucket << "/" << obj_name;

    // POST
    // /example-object?uploadId=AAAsb2FkIElEIGZvciBlbHZpbmcncyWeeS1tb3ZpZS5tMnRzIRRwbG9hZA
    // HTTP/1.1
    // Host: example-bucket.s3.amazonaws.com
    // Date:  Mon, 1 Nov 2010 20:34:56 GMT
    // Content-Length: 391
    // Authorization: authorization string
    //
    // <CompleteMultipartUpload>
    //   <Part>
    //     <PartNumber>1</PartNumber>
    //     <ETag>"a54357aff0632cce46d942af68356b38"</ETag>
    //   </Part>
    //   <Part>
    //     <PartNumber>2</PartNumber>
    //     <ETag>"0c78aef83f66abc1fa1e8477f296d394"</ETag>
    //   </Part>
    //   <Part>
    //     <PartNumber>3</PartNumber>
    //     <ETag>"acbd18db4cc2f85cedef654fccc4a4d8"</ETag>
    //   </Part>
    // </CompleteMultipartUpload>
    std::stringstream body;

    body << "<CompleteMultipartUpload>";
    for (i = 0; i < count; ++i) {
        body << "<Part><PartNumber>" << i << "</PartNumber><ETag>"
             << etag_array[i] << "</ETag></Part>";
    }
    body << "</CompleteMultipartUpload>\n";

    data = body.str();
    data_size = strlen(body.str());

    HeaderContent *header = new HeaderContent();
    header->Add(HOST, host);
    UrlParser p(url.str().c_str());
    path_with_query << p.Path() << "?uploadId=" << upload_id;
    SignPOSTv2(header, path_with_query.str().c_str(), cred);

    query << "uploadId=" << upload_id;

    CURL *curl = curl_easy_init();
    if (curl) {
        /* specify target URL, and note that this URL should include a file
           name, not only a directory */
        curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, mem_read_callback);

        /* now specify which file/data to upload */
        curl_easy_setopt(curl, CURLOPT_READDATA, data);

        /* provide the size of the upload, we specicially typecast the value
           to curl_off_t since we must be sure to use the correct data size */
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)data_size);

        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.str().c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                         (long)strlen(query.str().c_str()));

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ParserCallback);
    } else {
        return false;
    }

    struct curl_slist *chunk = header->GetList();
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

    // HTTP/1.1 200 OK
    // x-amz-id-2: Uuag1LuByRx9e6j5Onimru9pO4ZVKnJ2Qz7/C1NPcfTWAtRPfTaOFg==
    // x-amz-request-id: 656c76696e6727732072657175657374
    // Date: Mon, 1 Nov 2010 20:34:56 GMT
    // Connection: close
    // Server: AmazonS3
    //
    // <?xml version="1.0" encoding="UTF-8"?>
    // <CompleteMultipartUploadResult
    // xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
    //   <Location>http://Example-Bucket.s3.amazonaws.com/Example-Object</Location>
    //   <Bucket>Example-Bucket</Bucket>
    //   <Key>Example-Object</Key>
    //   <ETag>"3858f62230ac3c915f300c664312c11f-9"</ETag>
    // </CompleteMultipartUploadResult>

    // <Error>
    //   <Code>AccessDenied</Code>
    //   <Message>Access Denied</Message>
    //   <RequestId>656c76696e6727732072657175657374</RequestId>
    //   <HostId>Uuag1LuByRx9e6j5Onimru9pO4ZVKnJ2Qz7/C1NPcfTWAtRPfTaOFg==</HostId>
    // </Error>
    xmlParseChunk(xml.ctxt, "", 0, 1);
    xmlNode *root_element = xmlDocGetRootElement(xml.ctxt->myDoc);

    char *response_code = NULL;
    xmlNodePtr cur = root_element->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"Code")) {
            response_code = xmlNodeGetContent((char *)cur);
            break;
        }

        cur = cur->next;
    }

    if (response_code) {
        std::cout << response_code << std::endl;
    }

    xmlFreeParserCtxt(xml.ctxt);

    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    if (response_code) {
        return false;
    }

    return true;
}

//=================== XXX below is deprecated XXX ====================

bool PutS3Object(const char *host, const char *bucket, const char *url,
                 const S3Credential &cred, const char *data,
                 uint64_t data_size) {
    CURL *curl = curl_easy_init();
    if (curl) {
        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, mem_read_callback);

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

        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
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

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    return res;
}

bool DeleteS3Object(const char *host, const char *bucket, const char *url,
                    const S3Credential &cred) {
    CURL *curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
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

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    return res;
}
#endif
