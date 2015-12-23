#ifndef __S3LOG__
#define __S3LOG__

#include "cpplog.hpp"

cpplog::BaseLogger& GetDefaultLogger();

#define S3DEBUG  LOG_DEBUG(GetDefaultLogger())
#define S3INFO   LOG_INFO(GetDefaultLogger())
#define S3WARN   LOG_WARN(GetDefaultLogger())
#define S3ERROR  LOG_ERROR(GetDefaultLogger())

// EXTLOG

//extern uint8_t _LOGLEVEL;

#define EXT_FATAL             1
#define EXT_ERROR             2
#define EXT_WARNING           3
#define EXT_INFO              4
#define EXT_DEBUG             5


void EXTLOG(uint8_t level, const char* fmt, ...);

//#define EXTLOG(level, ...)								\
//	if ( level <= _LOGLEVEL ) _log_msg(level, __VA_ARGS__)


#endif // __S3LOG__
