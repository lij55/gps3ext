#ifndef _UTILFUNCTIONS_
#define _UTILFUNCTIONS_

bool gethttpnow(char datebuf[65]);

bool lowercase(char* out, const char* in);
void tolower(char* buf);

bool trim(char* out, const char* in, const char* trimed = " \t\r\n");

//! return value is malloced
char* Base64Encode(const char* buffer, size_t length);

bool sha256(char* string, char outputBuffer[65]);

char* sha1hmac(const char* str, const char* secret);

bool sha256hmac(char* str, char out[65], char* secret);

// return malloced because Base64Encode
char* SignatureV2(const char* date, const char* path, const char* key);
char* SignatureV4(const char* date, const char* path, const char* key);

void InitLog();
void EXTLOG(const char* fmt, ...);

#include <string>
using std::string;

#include <openssl/md5.h> 


size_t find_Nth(
				const string & str ,   // where to work
				unsigned            N ,     // N'th ocurrence
				const string & find    // what to 'find'
				);

class MD5Calc {
 public:
	MD5Calc();
	~MD5Calc(){};
	bool Update(const char* data, int len);
	const char* Get();
 private:
	MD5_CTX c;  
	unsigned char md5[17];
	string result;
};

#endif  // _UTILFUNCTIONS_
