#include "gtest/gtest.h"
#include "S3ExtWrapper.cpp"

void ExtWrapperTest(const char *url, uint64_t buffer_size,
                    const char *md5_str) {
    MD5Calc m;
    S3ExtBase *myData;
    uint64_t nread = 0;
    uint64_t buf_len = buffer_size;
    char *buf = (char *)malloc(buffer_size);

    ASSERT_NE((void *)NULL, buf);

    InitLog();

    myData = CreateExtWrapper(url);

    ASSERT_NE((void *)NULL, myData);
    ASSERT_TRUE(myData->Init(0, 1, 64 * 1024 * 1024));

    while (1) {
        nread = buf_len;

        myData->TransferData(buf, nread);

        if (nread == 0) break;

        m.Update(buf, nread);
    }

    EXPECT_STREQ(md5_str, m.Get());

    delete myData;
    free(buf);
}

#ifdef AWSTEST

TEST(ExtWrapper, normal) {
    ExtWrapperTest("http://s3-us-west-2.amazonaws.com/metro.pivotal.io/data/",
                   64 * 1024, "138fc555074671912125ba692c678246");
}

#endif
