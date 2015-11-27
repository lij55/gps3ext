#include "S3Common.h"

#include "utils.h"

#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <iostream>
#include <sstream>
#include <map>

using std::stringstream;

bool SignGetV2(HeaderContent* h, const char* path, const S3Credential& cred) {
    char timestr[64];
    char line[256];
    gethttpnow(timestr);
    h->Add(DATE ,timestr);
    h->Add(CONTENTLENGTH, "0");
    stringstream sstr;
    sstr<<"GET\n\n\n"<<timestr<<"\n"<<path;
    char* tmpbuf = sha1hmac(sstr.str().c_str(), cred.secret.c_str());
    int len  = strlen(tmpbuf);
    char* signature = Base64Encode(tmpbuf, len);
    sstr.clear();
    sstr.str("");
    sstr<<"AWS "<<cred.keyid<<":"<<signature;
    free(signature);
    h->Add(AUTHORIZATION,sstr.str().c_str());
    return true;
}



const char* GetFieldString(HeaderField f) {
    switch(f) {
    case HOST:
        return "Host";
    case RANGE:
        return "Range";
    case DATE:
        return "Date";
    case CONTENTLENGTH:
        return "Content-Length";
    case AUTHORIZATION:
        return "Authorization";
    default:
        return "unknown";
    }
}

bool HeaderContent::Add(HeaderField f, const std::string& v)
{
    if(!v.empty()) {
        this->fields[f] = std::string(v);
        return true;
    } else {
        return false;
    }

}

struct curl_slist *HeaderContent::GetList()
{
    struct curl_slist * chunk = NULL;
    std::map<HeaderField, std::string>::iterator it;
    for(it = this->fields.begin(); it != this->fields.end(); it++) {
        std::stringstream sstr;
        sstr<<GetFieldString(it->first)<<": "<<it->second;
        chunk = curl_slist_append(chunk, sstr.str().c_str());
    }
    return chunk;
}


UrlParser::UrlParser(const char* url)
{
    if(!url) {
        // throw exception
        return;
    }
    struct http_parser_url u;
    int len, result;
    len = strlen(url);
    this->fullurl = (char*)malloc(len+1);
    sprintf(this->fullurl, "%s", url);
    result = http_parser_parse_url(this->fullurl, len, false, &u);
    if (result != 0) {
        printf("Parse error : %d\n", result);
        return;
    }
    //std::cout<<u.field_set<<std::endl;
    this->host = extract_field(&u,UF_HOST);
    this->schema = extract_field(&u,UF_SCHEMA);
    this->path = extract_field(&u,UF_PATH);
}

UrlParser::~UrlParser() {
    if(host)
        free(host);
    if(schema)
        free(schema);
    if(path)
        free(path);
    if(fullurl)
        free(fullurl);
}

char* UrlParser::extract_field(const struct http_parser_url *u, http_parser_url_fields i) {
    char* ret = NULL;
    if((u->field_set & (1 << i)) != 0) {
        ret = (char*)malloc(u->field_data[i].len+1);
        if(ret) {
            memcpy(ret, this->fullurl + u->field_data[i].off, u->field_data[i].len);
            ret[u->field_data[i].len] = 0;
        }
    }
    return ret;
}
