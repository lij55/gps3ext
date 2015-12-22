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

void* runlocaludpserver ( void* unusedn) {
    int sockfd;
    int portno = 1111;
    
    struct sockaddr_in serveraddr; /* server's addr */

    struct hostent *hostp; /* client host info */
    char buf[1024]; /* message buf */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int n; /* message byte size */

    /* 
     * socket: create the parent socket 
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return NULL;
    }

    /* setsockopt: Handy debugging trick that lets 
     * us rerun the server immediately after we kill it; 
     * otherwise we have to wait about 20 secs. 
     * Eliminates "ERROR on binding: Address already in use" error. 
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
               (const void *)&optval , sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /* 
     * bind: associate the parent socket with a port 
     */
    if (bind(sockfd, (struct sockaddr *) &serveraddr, 
             sizeof(serveraddr)) < 0)  {
        perror("ERROR on binding");
        return NULL;
    }
    printf("in udp server\n");
    while (1) {
        /*
         * recvfrom: receive a UDP datagram from a client
         */
        bzero(buf, 1024);
        n = recvfrom(sockfd, buf, 1024, 0, NULL, NULL);
        if (n < 0) {
            perror("ERROR in recvfrom");
            return NULL;
        }
        if( n == 0 )
            break;
        printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    }
}

class UDPLogger : public cpplog::OstreamLogger {
private:
	UDPStream m_out;
    bool m_runserver;
    pthread_t m_thread;
public:
	UDPLogger(std::string host, unsigned short port);
    ~UDPLogger();
};

UDPLogger::UDPLogger(std::string host, unsigned short port) 
	: OstreamLogger(m_out)
    , m_runserver(true)
{
	m_out.open(host, port);
    if(m_runserver) {
        pthread_create(&this->m_thread, NULL, runlocaludpserver, NULL);
    }
}

UDPLogger::~UDPLogger() {
    if(m_runserver) {
        
        pthread_join(m_thread, NULL);
    }
 }

UDPLogger& GetDefaultLogger() {
    static UDPLogger log("127.0.0.1", 1111);
    return log;
}


int main () 
{
    LOG_DEBUG(GetDefaultLogger()) << "debug message 1" << std::endl;
    LOG_WARN(GetDefaultLogger()) << "Log message here" << std::endl;
    LOG_DEBUG(GetDefaultLogger()) << "debug message 2" << std::endl;
    CHECK_EQUAL(GetDefaultLogger(), 1,  2) << "Some other message" << std::endl;
    LOG_DEBUG(GetDefaultLogger()) << "debug message 3" << std::endl;
    CHECK_STREQ(GetDefaultLogger(), "a", "a") << "Strings should be equal" << std::endl;
    LOG_DEBUG(GetDefaultLogger()) << "debug message 4" << std::endl;
}
