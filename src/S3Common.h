#ifndef __S3_COMMON_H__
#define __S3_COMMON_H__

#include <curl/curl.h>
#include "http_parser.h"
#include <map>
#include <string>
using std::string;

struct S3Credential {
    string keyid;
    string secret;
};

enum HeaderField {
    HOST,
    RANGE,
    DATE,
    CONTENTLENGTH,
    CONTENTMD5,
    CONTENTTYPE,
    EXPECT,
    AUTHORIZATION,
};

enum Method { GET, PUT, POST, LIST, FETCH, DELETE, HEAD };

class HeaderContent {
   public:
    HeaderContent(){};
    ~HeaderContent(){};
    bool Add(HeaderField f, const string& value);
    bool Read(HeaderField f, string* dst);
    struct curl_slist* GetList();

   private:
    std::map<HeaderField, string> fields;
};

bool SignGETv2(HeaderContent* h, const char* path, const S3Credential& cred);
bool SignPUTv2(HeaderContent* h, const char* path, const S3Credential& cred);
bool SignPOSTv2(HeaderContent* h, const char* path, const S3Credential& cred);
bool SignLISTv2(HeaderContent* h, const char* path, const S3Credential& cred);
bool SignFETCHv2(HeaderContent* h, const char* path, const S3Credential& cred);
bool SignDELETEv2(HeaderContent* h, const char* path, const S3Credential& cred);

class UrlParser {
   public:
    UrlParser(const char* url);
    ~UrlParser();
    const char* Schema() { return this->schema; };
    const char* Host() { return this->host; };
    const char* Path() { return this->path; };
    /* data */
   private:
    char* extract_field(const struct http_parser_url* u,
                        http_parser_url_fields i);
    char* schema;
    char* host;
    char* path;
    char* fullurl;
};

const char* GetFieldString(HeaderField f);
CURL* CreateCurlHandler(const char* path);

#endif  // __S3_COMMON_H__
