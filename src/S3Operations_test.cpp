#include "gtest/gtest.h"
#include "S3Operations.cpp"

#define S3HOST "s3-us-west-2.amazonaws.com"
#define S3BUCKET "metro.pivotal.io"

#define KEYID "AKIAIAFSMJUMQWXB2PUQ"
#define SECRET "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"

TEST(Uploader, get_upload_id) {
    S3Credential g_cred = {KEYID, SECRET};
    const char *upload_id = GetUploadId(S3HOST, S3BUCKET, "data1234", g_cred);

    //printf("uploadid = %s\n", upload_id);

    EXPECT_NE(upload_id, (void *)NULL);
}

TEST(Uploader, get_upload_id_directories) {
    S3Credential g_cred = {KEYID, SECRET};
    const char *upload_id = GetUploadId(S3HOST, S3BUCKET, "test/upload/data1234", g_cred);

    //printf("uploadid = %s\n", upload_id);

    EXPECT_NE(upload_id, (void *)NULL);
}

TEST(Uploader, get_upload_id_long_url) {
    S3Credential g_cred = {KEYID, SECRET};
    const char *upload_id = GetUploadId(S3HOST, S3BUCKET, "test/upload/data1234aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbccccccccccccccccccccccddddddddddddddddddddddddeeeeeeeeeeeeeeeeeeeffffffffffffffffgggggggggggggggggggg", g_cred);

    //printf("uploadid = %s\n", upload_id);

    EXPECT_NE(upload_id, (void *)NULL);
}

TEST(Uploader, get_upload_id_long_url_directories) {
    S3Credential g_cred = {KEYID, SECRET};
    const char *upload_id = GetUploadId(S3HOST, S3BUCKET, "test/upload/data1234/aaaaaaaaaaaaaaaaaaaaaaaaa/bbbbbbbbbbbbbbbbbbbccccccccccccccccccccccddddddddddddddddddddddddeeeeeeeeeeeeeeeeeeeffffffffffffffffggggggggggggggggggg/g", g_cred);

    //printf("uploadid = %s\n", upload_id);

    EXPECT_NE(upload_id, (void *)NULL);
}

/*
// need to convert url string to Percent-encoding
// https://en.wikipedia.org/wiki/Percent-encoding
TEST(Uploader, get_upload_id_spaces) {
    S3Credential g_cred = {KEYID, SECRET};
    const char *upload_id = GetUploadId(S3HOST, S3BUCKET, "data 1234", g_cred);

    //printf("uploadid = %s\n", upload_id);

    EXPECT_NE(upload_id, (void *)NULL);
}

TEST(Uploader, get_upload_id_spaces_directories) {
    S3Credential g_cred = {KEYID, SECRET};
    const char *upload_id = GetUploadId(S3HOST, S3BUCKET, "test/ up load/data 1234", g_cred);

    //printf("uploadid = %s\n", upload_id);

    EXPECT_NE(upload_id, (void *)NULL);
}
*/
