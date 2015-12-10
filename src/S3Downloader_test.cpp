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
    DownloaderTest(
        "http://localhost/metro.pivotal.io/debian-8.2.0-amd64-netinst.iso",
        258998272, "762eb3dfc22f85faf659001ebf270b4f", 8, 4 * 1024 * 1024,
        4 * 1024);
}

TEST(Downloader, equal) {
    DownloaderTest(
        "http://localhost/metro.pivotal.io/debian-8.2.0-amd64-netinst.iso",
        258998272, "762eb3dfc22f85faf659001ebf270b4f", 8, 4 * 1024 * 1024,
        4 * 1024 * 1024);
}
