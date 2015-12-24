#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string>
#include <errno.h>
#include <unistd.h>
#include <cstdio>
#include <cstdarg>

// #include <thread>

/*
void EXTLOG(uint8_t level, const char* fmt, ...) {
	static char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 1023, fmt, args);
    va_end(args);

	switch(level) {
	case EXT_FATAL:
	case EXT_ERROR:
		S3ERROR<<buf<<std::endl;
		break;
	case EXT_WARNING:
		S3WARN<<buf<<std::endl;
		break;
	case EXT_INFO:
		S3INFO<<buf<<std::endl;
		break;
	case EXT_DEBUG:
	default:
		S3DEBUG<<buf<<std::endl;
		break;
	}
}
*/

void _LogMessage(const char* fmt, va_list args) {
	vfprintf(stderr, fmt, args);
}


int s3conf_loglevel = 1;
int s3conf_logtype = 1;

void LogMessage(int loglevel, const char* fmt, ...) {
	if(loglevel < s3conf_loglevel) 
		return;
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	//vsnprintf(buf, 1023, fmt, args);
	// va_end(args);
	switch(s3conf_logtype) {
	case 1:
		_LogMessage(fmt, args);
		break;
	case 2:
		vfprintf(stderr,  fmt, args);
		
		break;
	default:
		break;
	}
	va_end(args);
}

#define PRINTFUNCTION(i, format, ...)      LogMessage(i, format, __VA_ARGS__)
//#define PRINTFUNCTION(i, format, ...)      fprintf(stderr, format, __VA_ARGS__)



#define LOG_FMT   "[%s]%s:%d\t"
#define LOG_ARGS(LOGLEVEL)  LOGLEVEL, __FILE__, __LINE__
#define NEWLINE     "\n"

#define LOG_DEBUG(message, args...)     PRINTFUNCTION(1, LOG_FMT message NEWLINE, LOG_ARGS("D"), ## args)
#define LOG_INFO(message, args...)      PRINTFUNCTION(2, LOG_FMT message NEWLINE, LOG_ARGS("I"), ## args)
#define LOG_ERROR(message, args...)     PRINTFUNCTION(3, LOG_FMT message NEWLINE, LOG_ARGS("E"), ## args)


int main() {
	LOG_DEBUG("%s\t%d","asdfasdfasdfa",324);
	LOG_INFO("asdfasdfasdfa");
	LOG_ERROR("asdfasdfasdfa");
	return 0;
}
