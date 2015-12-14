#define PGDLLIMPORT "C"

#include "postgres.h"
#include "funcapi.h"

#include "access/extprotocol.h"
#include "catalog/pg_proc.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "fmgr.h"

#include "S3ExtWrapper.h"
#include "utils.h"
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
    // EXTLOG("%d myData: 0x%x\n", __LINE__, myData);
    if (EXTPROTOCOL_IS_LAST_CALL(fcinfo)) {
        if (!myData->Destroy()) {
            ereport(ERROR, (0, errmsg("Cleanup S3 extention failed")));
        }
        delete myData;
        PG_RETURN_INT32(0);
    }

    if (myData == NULL) {
        /* first call. do any desired init */
        InitLog();
        const char *p_name = "s3";
        char *url = EXTPROTOCOL_GET_URL(fcinfo);

        myData = CreateExtWrapper(
            "http://s3-us-west-2.amazonaws.com/metro.pivotal.io/data/");

        EXTLOG("%d myData: 0x%x\n", __LINE__, myData);

		// TODO: Get real segment number and segment id
        if (!myData || !myData->Init(0, 1, 64 * 1024 * 1024)) {
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
    // EXTLOG("%d myData: 0x%x\n", __LINE__, myData);
    if (data_len > 0) {
        nread = data_len;
        if (!myData->TransferData(data, nread))
            ereport(ERROR, (0, errmsg("s3_import: could not read data")));
        EXTLOG("read %d data from S3\n", nread);
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
    // EXTLOG("%d myData: 0x%x\n", __LINE__, myData);
    if (EXTPROTOCOL_IS_LAST_CALL(fcinfo)) {
        if (!myData->Destroy()) {
            ereport(ERROR, (0, errmsg("Cleanup S3 extention failed")));
        }
        delete myData;
        PG_RETURN_INT32(0);
    }

    if (myData == NULL) {
        /* first call. do any desired init */
        InitLog();
        const char *p_name = "s3";
        char *url = EXTPROTOCOL_GET_URL(fcinfo);

        myData = CreateExtWrapper(
            "http://s3-us-west-2.amazonaws.com/metro.pivotal.io/data/");

		// EXTLOG("%d myData: 0x%x\n", __LINE__, myData);

		// TODO: Get real segment number and segid
        if (!myData || !myData->Init(0, 1, 64 * 1024 * 1024)) {
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
    EXTLOG("%d myData: 0x%x\n", __LINE__, myData);
    if (data_len > 0) {
        nwrite = data_len;
        if (!myData->TransferData(data, nwrite))
            ereport(ERROR, (0, errmsg("s3_export: could not write data")));
        EXTLOG("write %d data from S3\n", nwrite);
    }

    PG_RETURN_INT32((int)nwrite);
}

Datum s3_validate_urls(PG_FUNCTION_ARGS) {
    int nurls;
    int i;
    ValidatorDirection direction;
    PG_RETURN_VOID();
}
