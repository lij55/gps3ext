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
#include <sstream>

#include "S3Log.h"
#include "utils.h"
#include "gps3conf.h"
#include "gps3ext.h"

//#include <cdb/cdbvars.h>

#include <string>
using std::string;
using std::stringstream;

// configurable parameters
int32_t s3ext_loglevel = -1;
int32_t s3ext_threadnum = 5;
int32_t s3ext_chunksize = 64 * 1024 * 1024;
int32_t s3ext_logtype = -1;
int32_t s3ext_logserverport = -1;

string s3ext_logserverhost;
string s3ext_logpath;
string s3ext_accessid;
string s3ext_secret;
string s3ext_token;

// global variables
int32_t s3ext_segid = -1;
int32_t s3ext_segnum = -1;

string s3ext_config_path;
struct sockaddr_in s3ext_logserveraddr;
struct sockaddr_un s3ext_logserverpath;
int32_t s3ext_logsock_local = -1;
int32_t s3ext_logsock_udp = -1;
Config* s3cfg = NULL;

// not thread safe!!
// Called only once.
bool InitConfig(const char* conf_path,
                const char* section /*unused currently*/) {
    if (!conf_path) {
        // empty path, log error
        return false;
    }

    if (!s3cfg) {
        s3cfg = new Config(conf_path);
        if (!s3cfg) {
            // create s3cfg fail
            // log error
            return false;
        }
    }

    Config* cfg = s3cfg;
    bool ret = 0;
    string content;
    content = cfg->Get("default", "loglevel", "ERROR");
    s3ext_loglevel = getLogLevel(content.c_str());

    content = cfg->Get("default", "logtype", "INTERNAL");
    s3ext_logtype = getLogType(content.c_str());

    s3ext_accessid = cfg->Get("default", "accessid", "");
    s3ext_secret = cfg->Get("default", "secret", "");
    s3ext_token = cfg->Get("default", "token", "");

#ifdef DEBUGS3
// s3ext_loglevel = EXT_DEBUG;
// s3ext_logtype = LOCAL_LOG;
#endif

    s3ext_logpath = cfg->Get("default", "logpath", "/tmp/.s3log.sock");
    s3ext_logserverhost = cfg->Get("default", "logserverhost", "127.0.0.1");
    ret = cfg->Scan("default", "logserverport", "%d", &s3ext_logserverport);
    if (!ret) s3ext_logserverport = 1111;
    ret = 0;

    ret &= cfg->Scan("default", "threadnum", "%d", &s3ext_threadnum);
    ret &= cfg->Scan("default", "chunksize", "%d", &s3ext_chunksize);

    if (ret) {
        fprintf(stderr, "failed to get configrations\n");
    }

#ifdef DEBUGS3
    s3ext_segid = 0;
    s3ext_segnum = 1;
#else
    s3ext_segid = GpIdentity.segindex;
    s3ext_segnum = GpIdentity.numsegments;
#endif
    return true;
}

void ClearConfig() {
    if (s3cfg) {
        delete s3cfg;
        s3cfg = NULL;
    }
}
