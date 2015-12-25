#include <string>

// local socket to send log
extern int s3conf_logsock_local;

// udp socket to send log
extern int s3conf_logsock_udp;

// default log level
extern int s3conf_loglevel;

// thread number for downloading
extern int s3conf_threadnum;

// chunk size for each downloading
extern int s3conf_chunksize;

// segment id
extern int s3conf_segid;

// total segmeng number
extern int s3conf_segnum;

// log type
extern int s3conf_logtype;

// remote server port if use external log server
extern int s3conf_logserverport;

// remote server address if use external log server
extern string s3conf_logserverhost;

// local Unix domain socket path if local log
extern string s3conf_logpath;

// s3 access id
extern string s3conf_accessid;

// s3 secret
extern string s3conf_secret;

// s3 token
extern string s3conf_token;

// configuration file path
extern string s3conf_config_path;

// server address where log msg is sent to
extern struct sockaddr_in s3conf_logserveraddr;
extern struct sockaddr_un s3conf_logserverpath;

// not thread safe!!
// Called only once.
void InitConfig();

void ClearConfig();

