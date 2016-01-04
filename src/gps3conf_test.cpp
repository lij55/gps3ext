#include "gtest/gtest.h"
#include "gps3conf.cpp"
#include <cstdlib>

string s3ext_secret;
string s3ext_accessid;

int s3ext_segid;
int s3ext_segnum;

int s3ext_chunksize;
int s3ext_threadnum;

TEST(Config, basic) {
    setenv("MASTER_DATA_DIRECTORY", "/tmp", 1);
    system("mkdir -p /tmp/s3");
    system("cp -f test/s3.conf /tmp/s3/s3.conf");
    // system("echo [s3] > /tmp/s3/s3.conf");
    // system("echo secret = \\\"secret_test\\\" >> /tmp/s3/s3.conf");
    // system("echo accessid = \\\"accessid_test\\\" >> /tmp/s3/s3.conf");
    // system("echo threadnum = 6 >> /tmp/s3/s3.conf");
    // system("echo chunksize = 67108865 >> /tmp/s3/s3.conf");
    InitConfig();

    EXPECT_STREQ(s3ext_secret.c_str(), "secret_test");
    EXPECT_STREQ(s3ext_accessid.c_str(), "accessid_test");

    EXPECT_EQ(s3ext_segid, 0);
    EXPECT_EQ(s3ext_segnum, 1);

    EXPECT_EQ(s3ext_threadnum, 6);
    EXPECT_EQ(s3ext_chunksize, 64 * 1024 * 1024 + 1);
}
