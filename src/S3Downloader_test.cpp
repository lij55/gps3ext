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

TEST(Downloader, divisible) {
    InitLog();
    uint64_t len = 4 * 1024;
    char *buf = (char *)malloc(4 * 1024);
    Downloader *d = new Downloader(8);
    MD5Calc m;

    ASSERT_NE((void *)NULL, buf);

    EXPECT_TRUE(d->init(
        "http://localhost/metro.pivotal.io/debian-8.2.0-amd64-netinst.iso",
        258998272, 4 * 1024 * 1024, NULL));

    while (1) {
        EXPECT_TRUE(d->get(buf, len));
        if (len == 0) {
            break;
        }
        m.Update(buf, len);
        len = 4 * 1024;
    }

    EXPECT_STREQ("762eb3dfc22f85faf659001ebf270b4f", m.Get());
    delete d;
    free(buf);
}

TEST(Downloader, equal) {
    InitLog();
    uint64_t len = 4 * 1024 * 1024;
    char *buf = (char *)malloc(4 * 1024 * 1024);
    Downloader *d = new Downloader(8);
    MD5Calc m;

    ASSERT_NE((void *)NULL, buf);

    EXPECT_TRUE(d->init(
        "http://localhost/metro.pivotal.io/debian-8.2.0-amd64-netinst.iso",
        258998272, 4 * 1024 * 1024, NULL));

    while (1) {
        EXPECT_TRUE(d->get(buf, len));
        if (len == 0) {
            break;
        }
        m.Update(buf, len);
        len = 4 * 1024 * 1024;
    }

    EXPECT_STREQ("762eb3dfc22f85faf659001ebf270b4f", m.Get());
    delete d;
    free(buf);
}
