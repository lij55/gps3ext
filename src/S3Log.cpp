#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string>
#include <errno.h>
#include <unistd.h>

#include <thread>

// This define must be before including "cpplog.hpp"
// #define LOG_LEVEL(level, logger) CustomLogMessage(__FILE__, __func__, __LINE__, (level), logger).getStream()

#include "cpplog.hpp"
using cpplog::StdErrLogger;

#include "S3Log.h"
#include "utils.h"

class CustomLogMessage : public cpplog::LogMessage
{
public:
    CustomLogMessage(const char* file, const char* function,
        unsigned int line, cpplog::loglevel_t logLevel,
        cpplog::BaseLogger &outputLogger)
    : cpplog::LogMessage(file, line, logLevel, outputLogger, false),
      m_function(function)
    {
        InitLogMessage();
    }

    static const char* shortLogLevelName(cpplog::loglevel_t logLevel)
    {
        switch( logLevel )
        {
            case LL_TRACE: return "T";
            case LL_DEBUG: return "D";
            case LL_INFO:  return "I";
            case LL_WARN:  return "W";
            case LL_ERROR: return "E";
            case LL_FATAL: return "F";
            default:       return "O";
        };
    }

protected:
    virtual void InitLogMessage()
    {
        m_logData->stream
            << "["
            << m_logData->fileName << ":"
            // << m_function          << ":"
            << m_logData->line
            << "]["
            << shortLogLevelName(m_logData->level)
            << "] ";
    }
private:
    const char *m_function;
};


class UDPStream_buf : public std::basic_stringbuf<char> {
private:
	int fd;
	struct sockaddr_in si_logserver;
	std::string m_host;
	unsigned short m_port;
public:
	UDPStream_buf();
	~UDPStream_buf();
	bool open(std::string host, unsigned short port);
	virtual int sync();
};

UDPStream_buf::UDPStream_buf() 
	:fd(-1)
{
}

UDPStream_buf::~UDPStream_buf() {
	if(this->fd != -1) {
		close(fd);
	}
}

bool UDPStream_buf::open(std::string host, unsigned short port) {
	if(fd == -1) {
		m_host = host;
		m_port = port;
		si_logserver.sin_family = AF_INET;
		si_logserver.sin_port = htons(port);
		if (inet_aton(host.c_str(), &si_logserver.sin_addr) == 0) {  // error
            // log error here
			goto error;
        }
        this->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(this->fd == -1) {
			goto error;
		}
	}
	return true;
 error:
	std::cerr<<"Create UDP socket fail:" << strerror(errno) << std::endl;
	return false;
}

int UDPStream_buf::sync() {
	if(this->fd != -1) {
		sendto(this->fd, str().c_str(), str().size(), 0, (struct sockaddr *)&si_logserver,
			   sizeof(sockaddr_in));
        setbuf((char*)"", 0);
	}
	return 0;
}

class UDPStream : public std::basic_ostream<char> {
public:
	UDPStream() : std::basic_ostream<char>(new UDPStream_buf()) {}
	~UDPStream() { delete rdbuf();}
	void open(std::string host, unsigned short port) {
		UDPStream_buf* buf = (UDPStream_buf*) rdbuf();
		buf->open(host, port);
	}
};


class UDPLogger : public cpplog::OstreamLogger {
private:
	UDPStream m_out;
public:
	UDPLogger(std::string host, unsigned short port);
    ~UDPLogger();
};

UDPLogger::UDPLogger(std::string host, unsigned short port) 
	: OstreamLogger(m_out)
{
	m_out.open(host, port);
}

UDPLogger::~UDPLogger() {
}

using cpplog::LogData;
class BackgroundLogger : public cpplog::BaseLogger
{
private:
	BaseLogger*                 m_forwardTo;
	concurrent_queue<cpplog::LogData*>  m_queue;

	std::thread               m_backgroundThread;
	LogData*                    m_dummyItem;

	void backgroundFunction()
	{
		LogData* nextLogEntry;
		bool deleteMessage = true;
		
		do
            {
                m_queue.deQ(nextLogEntry);
				
                if( nextLogEntry != m_dummyItem )
                    deleteMessage = m_forwardTo->sendLogMessage(nextLogEntry);

                if( deleteMessage )
                    delete nextLogEntry;
            } while( nextLogEntry != m_dummyItem );
        }

	void Init()
	{
		m_dummyItem = new LogData(LL_TRACE);
		
		m_backgroundThread = std::thread(std::bind(&BackgroundLogger::backgroundFunction, this));
	}

public:
	BackgroundLogger(BaseLogger* forwardTo)
		: m_forwardTo(forwardTo)
	{
		Init();
	}
	
	BackgroundLogger(BaseLogger& forwardTo)
		: m_forwardTo(&forwardTo)
	{
		Init();
	}
	
	void Stop()
	{
		m_queue.enQ(m_dummyItem);

		m_backgroundThread.join();
	}
	
	~BackgroundLogger()
	{
		Stop();
	}

	virtual bool sendLogMessage(LogData* logData)
	{
		m_queue.enQ(logData);
		
		// Don't delete - the background thread should handle this.
		return false;
	}
	
};

// requrei c++11
cpplog::BaseLogger& GetDefaultLogger() {
#if 0
	static StdErrLogger   errlog;
	static BackgroundLogger log(&errlog);
#else
	static UDPLogger log("127.0.0.1", 1111);
#endif
	return log;
}

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

