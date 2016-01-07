#include "S3Common.h"
#include "utils.h"

#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <iostream>
#include <sstream>
#include <string>
#include <map>

#include "gps3conf.h"

using std::string;
using std::stringstream;

bool SignGETv2(HeaderContent *h, const char *path_with_query,
               const S3Credential &cred) {
    char timestr[64];

    // CONTENT_LENGTH is not a part of StringToSign
    h->Add(CONTENTLENGTH, "0");

    gethttpnow(timestr);
    h->Add(DATE, timestr);
    stringstream sstr;
    sstr << "GET\n\n\n" << timestr << "\n" << path_with_query;
    char *tmpbuf = sha1hmac(sstr.str().c_str(), cred.secret.c_str());
    int len = strlen(tmpbuf);
    char *signature = Base64Encode(tmpbuf, len);
    sstr.clear();
    sstr.str("");
    sstr << "AWS " << cred.keyid << ":" << signature;
    free(signature);
    h->Add(AUTHORIZATION, sstr.str().c_str());

    return true;
}

bool SignPUTv2(HeaderContent *h, const char *path_with_query,
               const S3Credential &cred) {
    char timestr[64];
    string typestr;

    gethttpnow(timestr);
    h->Add(DATE, timestr);
    stringstream sstr;

    typestr = h->Get(CONTENTTYPE);

    sstr << "PUT\n\n" << typestr << "\n" << timestr << "\n" << path_with_query;
    char *tmpbuf = sha1hmac(sstr.str().c_str(), cred.secret.c_str());
    int len = strlen(tmpbuf);
    char *signature = Base64Encode(tmpbuf, len);
    sstr.clear();
    sstr.str("");
    sstr << "AWS " << cred.keyid << ":" << signature;
    free(signature);
    h->Add(AUTHORIZATION, sstr.str().c_str());

    return true;
}

bool SignPOSTv2(HeaderContent *h, const char *path_with_query,
                const S3Credential &cred) {
    char timestr[64];
    // string md5str;

    gethttpnow(timestr);
    h->Add(DATE, timestr);
    stringstream sstr;
    // md5str = h->Get(CONTENTMD5);
    const char *typestr = h->Get(CONTENTTYPE);

    if (typestr != NULL) {
        sstr << "POST\n"
             << "\n" << typestr << "\n" << timestr << "\n" << path_with_query;
    } else {
        sstr << "POST\n"
             << "\n"
             << "\n" << timestr << "\n" << path_with_query;
    }
    printf("%s\n", sstr.str().c_str());
    char *tmpbuf = sha1hmac(sstr.str().c_str(), cred.secret.c_str());
    int len = strlen(tmpbuf);
    char *signature = Base64Encode(tmpbuf, len);
    sstr.clear();
    sstr.str("");
    sstr << "AWS " << cred.keyid << ":" << signature;
    free(signature);
    h->Add(AUTHORIZATION, sstr.str().c_str());

    return true;
}

const char *GetFieldString(HeaderField f) {
    switch (f) {
        case HOST:
            return "Host";
        case RANGE:
            return "Range";
        case DATE:
            return "Date";
        case CONTENTLENGTH:
            return "Content-Length";
        case CONTENTMD5:
            return "Content-MD5";
        case CONTENTTYPE:
            return "Content-Type";
        case EXPECT:
            return "Expect";
        case AUTHORIZATION:
            return "Authorization";
        case ETAG:
            return "ETag";
        default:
            return "unknown";
    }
}

bool HeaderContent::Add(HeaderField f, const std::string &v) {
    if (!v.empty()) {
        this->fields[f] = std::string(v);
        return true;
    } else {
        return false;
    }
}

const char *HeaderContent::Get(HeaderField f) {
    const char *ret = NULL;
    if (!this->fields[f].empty()) {
        ret = this->fields[f].c_str();
    }
    return ret;
}

struct curl_slist *HeaderContent::GetList() {
    struct curl_slist *chunk = NULL;
    std::map<HeaderField, std::string>::iterator it;
    for (it = this->fields.begin(); it != this->fields.end(); it++) {
        std::stringstream sstr;
        sstr << GetFieldString(it->first) << ": " << it->second;
        chunk = curl_slist_append(chunk, sstr.str().c_str());
    }
    return chunk;
}

UrlParser::UrlParser(const char *url) {
    if (!url) {
        // throw exception
        return;
    }

    this->schema = NULL;
    this->host = NULL;
    this->path = NULL;

    struct http_parser_url u;
    int len, result;

    len = strlen(url);
    this->fullurl = (char *)malloc(len + 1);
    if (!this->fullurl) return;

    memcpy(this->fullurl, url, len);
    this->fullurl[len] = 0;

    // only parse len, no need to memset this->fullurl
    result = http_parser_parse_url(this->fullurl, len, false, &u);
    if (result != 0) {
        S3ERROR("Parse error : %d\n", result);
        return;
    }

    // std::cout<<u.field_set<<std::endl;
    this->schema = extract_field(&u, UF_SCHEMA);
    this->host = extract_field(&u, UF_HOST);
    this->path = extract_field(&u, UF_PATH);
}

UrlParser::~UrlParser() {
    if (this->schema) free(this->schema);
    if (this->host) free(this->host);
    if (this->path) free(this->path);
    if (this->fullurl) free(this->fullurl);

    this->schema = NULL;
    this->host = NULL;
    this->path = NULL;
    this->fullurl = NULL;
}

char *UrlParser::extract_field(const struct http_parser_url *u,
                               http_parser_url_fields i) {
    char *ret = NULL;
    if ((u->field_set & (1 << i)) != 0) {
        ret = (char *)malloc(u->field_data[i].len + 1);
        if (ret) {
            memcpy(ret, this->fullurl + u->field_data[i].off,
                   u->field_data[i].len);
            ret[u->field_data[i].len] = 0;
        }
    }
    return ret;
}

uint64_t ParserCallback(void *contents, uint64_t size, uint64_t nmemb,
                        void *userp) {
    uint64_t realsize = size * nmemb;
    int res;
    // printf("%.*s",realsize, (char*)contents);
    struct XMLInfo *pxml = (struct XMLInfo *)userp;
    if (!pxml->ctxt) {
        pxml->ctxt = xmlCreatePushParserCtxt(NULL, NULL, (const char *)contents,
                                             realsize, "resp.xml");
        return realsize;
    } else {
        res = xmlParseChunk(pxml->ctxt, (const char *)contents, realsize, 0);
    }
    return realsize;
}

Config &GetGlobalS3Config() {
    static Config s3cfg("s3config.ini");
    return s3cfg;
}

char *get_opt_s3(const char *url, const char *key) {
    const char *key_f;
    const char *key_tailing;
    char *key_val;
    int val_len;

    if (!url || !key) {
        return NULL;
    }

    char *key2search = (char *)malloc(strlen(key) + 3);
    if (!key2search) {
        return NULL;
    }

    int key_len = strlen(key);

    key2search[0] = ' ';
    memcpy(key2search + 1, key, key_len);
    key2search[key_len + 1] = '=';
    key2search[key_len + 2] = 0;

    // printf("key2search=%s\n", key2search);
    const char *delimiter = " ";
    const char *options = strstr(url, delimiter);
    if (!options) {  // no options string
        goto FAIL;
    }
    key_f = strstr(options, key2search);
    if (key_f == NULL) {
        goto FAIL;
    }

    key_f += strlen(key2search);
    if (*key_f == ' ') {
        goto FAIL;
    }
    // printf("key_f=%s\n", key_f);

    key_tailing = strstr(key_f, delimiter);
    // printf("key_tailing=%s\n", key_tailing);
    val_len = 0;
    if (key_tailing) {
        val_len = strlen(key_f) - strlen(key_tailing);
    } else {
        val_len = strlen(key_f);
    }

    key_val = (char *)malloc(val_len + 1);
    if (!key_val) {
        goto FAIL;
    }

    memcpy(key_val, key_f, val_len);
    key_val[val_len] = 0;

    free(key2search);

    return key_val;
FAIL:
    if (!key2search) {
        free(key2search);
    }
    return NULL;
}
