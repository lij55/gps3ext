#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string>
#include <errno.h>
#include <unistd.h>

// This define must be before including "cpplog.hpp"
#define LOG_LEVEL(level, logger) CustomLogMessage(__FILE__, __func__, __LINE__, (level), logger).getStream()

#include "cpplog.hpp"
using cpplog::StdErrLogger;



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
            << m_function          << ":"
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
	if(this->fd == -1) {
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
		seekpos(0);
		
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
public:
	UDPStream m_out;
public:
	UDPLogger(std::string host, unsigned short port);
};

UDPLogger::UDPLogger(std::string host, unsigned short port) 
	: OstreamLogger(m_out){
	m_out.open(host, port);
}

UDPLogger& GetUDPLogger() {
	std::string host = "127.0.0.1";
	unsigned short port = 1111;
	static UDPLogger log(host, port);
	return log;
}

int main () 
{
	cpplog::FilteringLogger log(LL_INFO, GetUDPLogger());
	//UDPLogger log("127.0.0.1", 1111);
	LOG_DEBUG(log) << "debug message" << std::endl;
    LOG_WARN(log) << "Log message here" << std::endl;
    CHECK_EQUAL(log, 1,  2) << "Some other message" << std::endl;
    CHECK_STREQ(log, "a", "a") << "Strings should be equal" << std::endl;
}
