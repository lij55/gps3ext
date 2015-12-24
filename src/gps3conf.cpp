
// local socket to send log
int s3conf_logsock = -1;

// default log level
int s3conf_loglevel = -1;

// thread number for downloading
int s3conf_threadnum = 5;

// chunk size for each downloading
int s3conf_chunksize = 64*1024*1024;

// segment id
int s3conf_segid = -1;

// total segmeng number
int s3conf_segnum = -1;

// log type
int s3conf_logtype = -1;

// remote server port if use external log server
int s3conf_logserverport = -1;

// remote server address if use external log server
string s3conf_logserverhost;

// local Unix domain socket path if local log
string s3conf_logpath;

// s3 access id
string s3conf_accessid;

// s3 secret
string s3conf_secret;

// s3 token
string s3conf_token;

// configuration file path
string s3conf_config_path;

// server address where log msg is sent to
struct sockaddr_in s3conf_logserveraddr;
struct sockaddr_un s3conf_logserverpath;

// not thread safe!!
// Called only once.
void InitConfig();

void ClearConfig();

