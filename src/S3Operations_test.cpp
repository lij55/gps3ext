#include "gtest/gtest.h"
#include "S3Operations.cpp"

#define S3HOST "s3-us-west-2.amazonaws.com"
#define S3BUCKET "metro.pivotal.io"
#define S3OBJNAME "tmp/upload/data1234"

#define KEYID "AKIAIAFSMJUMQWXB2PUQ"
#define SECRET "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"

TEST(Uploader, upload_id) {
    S3Credential g_cred = {KEYID, SECRET};
    const char *upload_id = GetUploadId(S3HOST, S3BUCKET, S3OBJNAME, g_cred);

    printf("uploadid=%s\n", upload_id);
}
