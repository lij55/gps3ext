#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <time.h>
#include <string.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <openssl/des.h>
#include <stdbool.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdint.h>

#include <curl/curl.h>

bool gethttpnow(char datebuf[65]) { //('D, d M Y H:i:s T')
    struct tm* tm_info;
    time_t t;
    if(!datebuf) {
        return false;
    }
    time(&t);
    tm_info = localtime(&t);
    strftime(datebuf, 64, "%a, %d %b %Y %H:%M:%S %z", tm_info);
    return true;
}

bool lowercase(char* out, const char* in) {
    if(!out || !in) { // invalid string params
        return false;
    }
    while(*in) {
        //*out++ = (*in >= 'A' && *in <= 'Z') ? *in|0x60 : *in;
        *out++ = *in;
        in++;
    }
    *out=0;
    return true;
}

void tolower(char* buf) {
    do {
        if(*buf >= 'A' && *buf <= 'Z')
            *buf |= 0x60;
    } while(*buf++);
    return;
}

bool trim(char* out, const char* in, const char* trimed = " \t\r\n") {
    int targetlen;

    if(!out || !in) { // invalid string params
        return false;
    }

    targetlen = strlen(in);

    while(targetlen > 0) {
        if(strchr(trimed, in[targetlen-1]) == NULL) // can't find stripped char
            break;
        else
            targetlen--;
    }

    while(targetlen > 0) {
        if(strchr(trimed, *in) == NULL) // normal string
            break;
        else {
            in++;
            targetlen--;
        }
    }

    memcpy(out, in, targetlen);
    out[targetlen] = 0;
    return true;
}

//! return value is malloced
char* Base64Encode(const char* buffer, size_t length) { //Encodes a binary safe base 64 string
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;
    char* ret;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    ret = (char*) malloc(bufferPtr->length+1);
    memcpy(ret, bufferPtr->data, bufferPtr->length);
    ret[bufferPtr->length] = 0;

    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    return ret; //s
}

bool sha256(char *string, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    if(!string)
        return false;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
    return true;
}

char* sha1hmac(const char* str, const char* secret) {
    return (char*)HMAC(EVP_sha1(), secret, strlen(secret), (const unsigned char*)str, strlen(str), NULL, NULL);
}

bool sha256hmac(char* str, char out[65], char* secret) {
    unsigned char hash[32];
    unsigned int len = 32;
    int i;
    HMAC_CTX hmac;
    HMAC_CTX_init(&hmac);
    HMAC_Init_ex(&hmac, secret, strlen(secret), EVP_sha256(), NULL);
    HMAC_Update(&hmac, (unsigned char*)str, strlen(str));

    HMAC_Final(&hmac, hash, &len);
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(out + (i * 2), "%02x", hash[i]);
    }
    HMAC_CTX_cleanup(&hmac);
    out[64]=0;
    return true;
}

//return malloced because Base64Encode
char* SignatureV2(const char* date, const char* path, const char* key) {
    int maxlen, len;
    char* tmpbuf, *outbuf;
    if( !date || !path || !key) {
        return false;
    }
    maxlen = strlen(date) + strlen(path) + 20;
    tmpbuf = (char*)alloca(maxlen);
    sprintf(tmpbuf, "GET\n\n\n%s\n%s", date, path);
    //printf("%s\n",tmpbuf);
    outbuf = sha1hmac(tmpbuf, key);
    len = strlen(outbuf);
    return Base64Encode(outbuf, len);
}

char* SignatureV4(const char* date, const char* path, const char* key) {
    return NULL;
}

CURL* CreateCurlHandler(const char* path) {
    CURL *curl = NULL;
    if(!path) {
        return NULL;
    } else {
        curl = curl_easy_init();
    }

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, path);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    } else {
        return NULL;
    }
    return curl;
}
