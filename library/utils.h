#ifndef _UTILFUNCTIONS_
#define _UTILFUNCTIONS_


bool gethttpnow(char datebuf[65]);

bool lowercase(char* out, const char* in);
void tolower(char* buf);

bool trim(char* out, const char* in, const char* trimed = " \t\r\n");

//! return value is malloced
char* Base64Encode(const char* buffer, size_t length);

bool sha256(char *string, char outputBuffer[65]);

char* sha1hmac(const char* str, const char* secret);

bool sha256hmac(char* str, char out[65], char* secret);


//return malloced because Base64Encode
char* SignatureV2(const char* date, const char* path, const char* key);
char* SignatureV4(const char* date, const char* path, const char* key);



#endif // _UTILFUNCTIONS_
