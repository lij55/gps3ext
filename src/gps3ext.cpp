#define PGDLLIMPORT "C"

#include <cstdlib>

#include "postgres.h"
#include "funcapi.h"

#include "access/extprotocol.h"
#include "catalog/pg_proc.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "fmgr.h"

#include "S3ExtWrapper.h"
#include "S3Common.h"
#include "S3Log.h"
#include "utils.h"

#include "gps3ext.h"

/* Do the module magic dance */

PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(s3_export);
PG_FUNCTION_INFO_V1(s3_import);
PG_FUNCTION_INFO_V1(s3_validate_urls);

extern "C" {
Datum s3_export(PG_FUNCTION_ARGS);
Datum s3_import(PG_FUNCTION_ARGS);
Datum s3_validate_urls(PG_FUNCTION_ARGS);
}

string s3ext_secret;
string s3ext_accessid;

int s3ext_segid = -1;
int s3ext_segnum = -1;

int s3ext_chunksize = -1;
int s3ext_threadnum = -1;

/*
 * Import data into GPDB.
 */
Datum s3_import(PG_FUNCTION_ARGS) {
    S3ExtBase *myData;
    char *data;
    int data_len;
    size_t nread = 0;

    /* Must be called via the external table format manager */
    if (!CALLED_AS_EXTPROTOCOL(fcinfo))
        elog(ERROR,
             "extprotocol_import: not called by external protocol manager");

    /* Get our internal description of the protocol */
    myData = (S3ExtBase *)EXTPROTOCOL_GET_USER_CTX(fcinfo);
    // S3DEBUG("%d myData: 0x%x\n", __LINE__, myData);
    if (EXTPROTOCOL_IS_LAST_CALL(fcinfo)) {
        if (!myData->Destroy()) {
            ereport(ERROR, (0, errmsg("Cleanup S3 extention failed")));
        }
        delete myData;
        PG_RETURN_INT32(0);
    }

    if (myData == NULL) {
/* first call. do any desired init */
#ifdef DEBUGS3
        InitLog();
#endif
        const char *p_name = "s3";
        char *url_with_options = EXTPROTOCOL_GET_URL(fcinfo);

        // truncate url
        const char *delimiter = " ";
        char *options = strstr(url_with_options, delimiter);
        int url_len = strlen(url_with_options) - strlen(options);
        char url[url_len + 1];
        memcpy(url, url_with_options, url_len);
        url[url_len] = 0;

        s3ext_secret = get_opt_s3(options, "secret");
        s3ext_accessid = get_opt_s3(options, "accessid");

        s3ext_chunksize = atoi(get_opt_s3(options, "chunksize"));
        s3ext_threadnum = atoi(get_opt_s3(options, "threadnum"));

        myData = CreateExtWrapper(url);

        S3DEBUG("%d myData: 0x%x\n", __LINE__, myData);

        if (!myData ||
            !myData->Init(s3ext_segid, s3ext_segnum, s3ext_chunksize)) {
            if (myData) delete myData;
            ereport(ERROR, (0, errmsg("Init S3 extension fail")));
        }
        /*
                  if(strcasecmp(parsed_url->protocol, p_name) != 0) {
                  elog(ERROR, "internal error: s3prot called with a different
                  protocol
                  (%s)",
                  parsed_url->protocol);
                  }
        */

        EXTPROTOCOL_SET_USER_CTX(fcinfo, myData);
    }

    /* =======================================================================
     *                            DO THE IMPORT
     * =======================================================================
     */

    data = EXTPROTOCOL_GET_DATABUF(fcinfo);
    data_len = EXTPROTOCOL_GET_DATALEN(fcinfo);
    // S3DEBUG("%d myData: 0x%x\n", __LINE__, myData);
    if (data_len > 0) {
        nread = data_len;
        if (!myData->TransferData(data, nread))
            ereport(ERROR, (0, errmsg("s3_import: could not read data")));
        S3DEBUG("read %d data from S3\n", nread);
    }

    PG_RETURN_INT32((int)nread);
}

/*
 * Export data out of GPDB.
 */
Datum s3_export(PG_FUNCTION_ARGS) {
    S3ExtBase *myData;
    char *data;
    int data_len;
    size_t nwrite = 0;

    /* Must be called via the external table format manager */
    if (!CALLED_AS_EXTPROTOCOL(fcinfo))
        elog(ERROR,
             "extprotocol_import: not called by external protocol manager");

    /* Get our internal description of the protocol */
    myData = (S3ExtBase *)EXTPROTOCOL_GET_USER_CTX(fcinfo);
    // S3DEBUG("%d myData: 0x%x\n", __LINE__, myData);
    if (EXTPROTOCOL_IS_LAST_CALL(fcinfo)) {
        if (!myData->Destroy()) {
            ereport(ERROR, (0, errmsg("Cleanup S3 extention failed")));
        }
        delete myData;
        PG_RETURN_INT32(0);
    }

    if (myData == NULL) {
/* first call. do any desired init */
#ifdef DEBUGS3
        InitLog();
#endif
        const char *p_name = "s3";
        char *url_with_options = EXTPROTOCOL_GET_URL(fcinfo);

        // truncate url
        const char *delimiter = " ";
        char *options = strstr(url_with_options, delimiter);
        int url_len = strlen(url_with_options) - strlen(options);
        char url[url_len + 1];
        memcpy(url, url_with_options, url_len);
        url[url_len] = 0;

        s3ext_secret = get_opt_s3(options, "secret");
        s3ext_accessid = get_opt_s3(options, "accessid");

        s3ext_chunksize = atoi(get_opt_s3(options, "chunksize"));
        s3ext_threadnum = atoi(get_opt_s3(options, "threadnum"));

        myData = CreateExtWrapper(url);

        // S3DEBUG("%d myData: 0x%x\n", __LINE__, myData);

        if (!myData ||
            !myData->Init(s3ext_segid, s3ext_segnum, s3ext_chunksize)) {
            if (myData) delete myData;
            ereport(ERROR, (0, errmsg("Init S3 extension fail")));
        }

        EXTPROTOCOL_SET_USER_CTX(fcinfo, myData);
    }

    /* =======================================================================
     *                            DO THE EXPORT
     * =======================================================================
     */

    data = EXTPROTOCOL_GET_DATABUF(fcinfo);
    data_len = EXTPROTOCOL_GET_DATALEN(fcinfo);
    S3DEBUG("%d myData: 0x%x\n", __LINE__, myData);
    if (data_len > 0) {
        nwrite = data_len;
        if (!myData->TransferData(data, nwrite))
            ereport(ERROR, (0, errmsg("s3_export: could not write data")));
        S3DEBUG("write %d data from S3\n", nwrite);
    }

    PG_RETURN_INT32((int)nwrite);
}

Datum s3_validate_urls(PG_FUNCTION_ARGS) {
    int nurls;
    int i;
    ValidatorDirection direction;
    PG_RETURN_VOID();
}
