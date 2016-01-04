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

int s3conf_logsock_local = -1;
int s3conf_logsock_udp = -1;

int s3conf_loglevel = -1;
int s3conf_threadnum = 5;
int s3conf_chunksize = 64 * 1024 * 1024;
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
#endif

    bool ret = 0;
    stringstream fullpath;
    const char *env_d = std::getenv("MASTER_DATA_DIRECTORY");

    fullpath << env_d << "/s3/s3.conf";

    Config *cfg = new Config(fullpath.str().c_str());

    s3conf_accessid = cfg->Get("s3", "accessid", "AKIAIAFSMJUMQWXB2PUQ");
    s3conf_secret =
        cfg->Get("s3", "secret", "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");

    ret &= cfg->Scan("s3", "threadnum", "%ld", &s3conf_threadnum);
    ret &= cfg->Scan("s3", "chunksize", "%ld", &s3conf_chunksize);

    if (ret) {
        printf("failed to get configrations\n");
    }

    s3ext_secret = s3conf_secret;
    s3ext_accessid = s3conf_accessid;

    s3ext_segid = GpIdentity.segindex;
    s3ext_segnum = GpIdentity.numsegments;

    s3ext_threadnum = s3conf_threadnum;
    s3ext_chunksize = s3conf_chunksize;

    delete cfg;
}

void ClearConfig() {}
