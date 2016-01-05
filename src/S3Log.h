#ifndef __S3LOG__
#define __S3LOG__

#include <cstdarg>
#include <cstdio>

#include <pthread.h>

// log level
enum LOGLEVEL { EXT_FATAL, EXT_ERROR, EXT_WARNING, EXT_INFO, EXT_DEBUG };

// log type
enum LOGTYPE {
    REMOTE_LOG,    // log to remote udp server
    LOCAL_LOG,     // log to local unix dgram domain socket
    INTERNAL_LOG,  // use pg elog
    STDERR_LOG     // use stderr
};

uint8_t loglevel();
void LogMessage(LOGLEVEL level, const char* fmt, ...);

LOGTYPE getLogType(const char* v);
LOGLEVEL getLogLevel(const char* v);

#define PRINTFUNCTION(i, format, ...) LogMessage(i, format, __VA_ARGS__)

#define LOG_FMT "[%s](%0X)%s:%d  "
#define LOG_ARGS(LOGLEVELSTR) LOGLEVELSTR, pthread_self(), __FILE__, __LINE__
#define NEWLINE "\n"

#define S3DEBUG(message, args...) \
    if (EXT_DEBUG <= loglevel())  \
    PRINTFUNCTION(EXT_DEBUG, LOG_FMT message NEWLINE, LOG_ARGS("D"), ##args)

#define S3INFO(message, args...) \
    if (EXT_INFO <= loglevel())  \
    PRINTFUNCTION(EXT_INFO, LOG_FMT message NEWLINE, LOG_ARGS("I"), ##args)

#define S3WARN(message, args...)   \
    if (EXT_WARNING <= loglevel()) \
    PRINTFUNCTION(EXT_WARNING, LOG_FMT message NEWLINE, LOG_ARGS("W"), ##args)

#define S3ERROR(message, args...) \
    if (EXT_ERROR <= loglevel())  \
    PRINTFUNCTION(EXT_ERROR, LOG_FMT message NEWLINE, LOG_ARGS("E"), ##args)

void InitLog();

#endif  // __S3LOG__
