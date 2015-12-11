#include "gtest/gtest.h"
#include "S3Downloader.cpp"

TEST(OffsetMgr, simple) {
    OffsetMgr *o = new OffsetMgr(4096, 1000);
    Range r = o->NextOffset();
    EXPECT_EQ(r.offset, 0);
    EXPECT_EQ(r.len, 1000);

    r = o->NextOffset();
    EXPECT_EQ(r.offset, 1000);
    EXPECT_EQ(r.len, 1000);

    r = o->NextOffset();
    EXPECT_EQ(r.offset, 2000);
    EXPECT_EQ(r.len, 1000);

    r = o->NextOffset();
    EXPECT_EQ(r.offset, 3000);
    EXPECT_EQ(r.len, 1000);

    r = o->NextOffset();
    EXPECT_EQ(r.offset, 4000);
    EXPECT_EQ(r.len, 96);
    delete o;
}

TEST(OffsetMgr, reset) {
    OffsetMgr *o = new OffsetMgr(1024, 100);

    o->NextOffset();
    o->NextOffset();
    o->Reset(333);
    Range r = o->NextOffset();

    EXPECT_EQ(r.offset, 333);
    EXPECT_EQ(r.len, 100);
    delete o;
}

#define HOSTSTR "localhost"
#define BUCKETSTR "s3"
TEST(ListBucket, fake) {
    ListBucketResult *r = ListBucket_FakeHTTP(HOSTSTR, BUCKETSTR);
    char urlbuf[256];
    EXPECT_NE(r, (void *)NULL);
    if (!r) return;
    vector<BucketContent *>::iterator i;
    for (i = r->contents.begin(); i != r->contents.end(); i++) {
        BucketContent *p = *i;
        sprintf(urlbuf, "http://%s/%s/%s", HOSTSTR, BUCKETSTR, p->Key());
        printf("%s, %d\n", urlbuf, p->Size());
        // printdata(urlbuf, p->Size(), &cred);
    }
    delete r;
}

void DownloaderTest(const char *url, uint64_t file_size, const char *md5_str,
                    uint8_t thread_num, uint64_t chunk_size,
                    uint64_t buffer_size) {
    InitLog();
    uint64_t buf_len = buffer_size;
    char *buf = (char *)malloc(buffer_size);
    Downloader *d = new Downloader(thread_num);
    MD5Calc m;

    ASSERT_NE((void *)NULL, buf);

    ASSERT_TRUE(d->init(url, file_size, chunk_size, NULL));

    while (1) {
        ASSERT_TRUE(d->get(buf, buf_len));
        if (buf_len == 0) {
            break;
        }
        m.Update(buf, buf_len);
        buf_len = buffer_size;
    }

    EXPECT_STREQ(md5_str, m.Get());
    delete d;
    free(buf);
}

TEST(Downloader, divisible) {
    DownloaderTest("http://localhost/metro.pivotal.io/8M", 8388608,
                   "22129b81bf8f96a06a8b7f3d2a683588", 4, 4 * 1024, 16 * 1024);
}

TEST(Downloader, equal) {
    DownloaderTest("http://localhost/metro.pivotal.io/8M", 8388608,
                   "22129b81bf8f96a06a8b7f3d2a683588", 4, 16 * 1024, 16 * 1024);
}

TEST(Downloader, one_byte) {
    DownloaderTest("http://localhost/metro.pivotal.io/8M", 8388608,
                   "22129b81bf8f96a06a8b7f3d2a683588", 4, 4 * 1024, 1);
}

TEST(Downloader, over_flow) {
    DownloaderTest("http://localhost/metro.pivotal.io/8M", 8388608,
                   "22129b81bf8f96a06a8b7f3d2a683588", 4, 7 * 1024, 15 * 1024);
}

TEST(Downloader, multi_thread) {
    DownloaderTest("http://localhost/metro.pivotal.io/1M", 1048576,
                   "06427456b575a3880936b4ae43448082", 64, 4 * 1024,
                   511 * 1023);
}

TEST(Downloader, single_thread) {
    DownloaderTest("http://localhost/metro.pivotal.io/1M", 1048576,
                   "06427456b575a3880936b4ae43448082", 1, 4 * 1024, 511 * 1023);
}

TEST(Downloader, random_parameters_1M) {
    DownloaderTest("http://localhost/metro.pivotal.io/1M", 1048576,
                   "06427456b575a3880936b4ae43448082", 3, 3 * 29, 571 * 1023);
}

TEST(Downloader, random_parameters_1G) {
    DownloaderTest("http://localhost/metro.pivotal.io/1G", 1073741824,
                   "a2cb2399eb8bc97084aed673e5d09f4d", 9, 42 * 1024,
                   513 * 1025 * 37);
}

TEST(Downloader, random_parameters_8M) {
    DownloaderTest("http://localhost/metro.pivotal.io/8M", 8388608,
                   "22129b81bf8f96a06a8b7f3d2a683588", 77, 7 * 1024,
                   777 * 1025);
}

TEST(Downloader, random_parameters_64M) {
    DownloaderTest("http://localhost/metro.pivotal.io/64M", 67108864,
                   "0871a7464a7ff85564f4f95b9ac77321", 51, 7 * 1053, 77 * 87);
}

TEST(Downloader, random_parameters_256M) {
    DownloaderTest("http://localhost/metro.pivotal.io/256M", 268435456,
                   "9cb7c287fdd5a44378798d3e75d2d2a6", 3, 1 * 10523, 77 * 879);
}
