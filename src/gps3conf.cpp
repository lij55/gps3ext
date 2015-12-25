#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string>
#include <errno.h>
#include <unistd.h>
#include <cstdio>
#include <cstdarg>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "S3Log.h"

#include <string>
using std::string;

int s3conf_logsock_local = -1;
int s3conf_logsock_udp = -1;

int s3conf_loglevel = -1;
int s3conf_threadnum = 5;
int s3conf_chunksize = 64*1024*1024;
int s3conf_segid = -1;
int s3conf_segnum = -1;
int s3conf_logtype = -1;
int s3conf_logserverport = -1;
string s3conf_logserverhost;
string s3conf_logpath;
string s3conf_accessid;
string s3conf_secret;
string s3conf_token;
string s3conf_config_path;

struct sockaddr_in s3conf_logserveraddr;
struct sockaddr_un s3conf_logserverpath;

#ifdef DEBUGS3
extern void InitLog();
#endif

// not thread safe!!
// Called only once.
void InitConfig() {

    int s3conf_loglevel = 1;
    int s3conf_logtype = LOCAL_LOG;

#ifdef DEBUGS3
    InitLog();
#else
    //
#endif
    // TO BE FINISHED

}

void ClearConfig() {
}

