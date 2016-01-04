#include "S3Common.cpp"
#include "gtest/gtest.h"

TEST(S3Common, UrlParser) {
    UrlParser *p = new UrlParser(
        "https://www.google.com/search?sclient=psy-ab&site=&source=hp");
    EXPECT_NE(p, (void *)NULL);
    if (p) {
        EXPECT_STREQ(p->Schema(), "https");
        EXPECT_STREQ(p->Host(), "www.google.com");
        EXPECT_STREQ(p->Path(), "/search");
        delete p;
    }
}

TEST(S3Common, UrlParser_LongURL) {
    UrlParser *p = new UrlParser(
        "http://s3-us-west-2.amazonaws.com/metro.pivotal.io/test/"
        "data1234?partNumber=1&uploadId=."
        "CXn7YDXxGo7aDLxEyX5wxaDivCw5ACWfaMQts8_4M6."
        "NbGeeaI1ikYlO5zWZOpclVclZRAq5758oCxk_DtiX5BoyiMr7Ym6TKiEqqmNpsE-");
    EXPECT_NE(p, (void *)NULL);
    if (p) {
        EXPECT_STREQ(p->Schema(), "http");
        EXPECT_STREQ(p->Host(), "s3-us-west-2.amazonaws.com");
        EXPECT_STREQ(p->Path(), "/metro.pivotal.io/test/data1234");
        delete p;
    }
}

#define HOSTSTR "www.google.com"
#define RANGESTR "1-10000"
#define MD5STR "xxxxxxxxxxxxxxxxxxx"

TEST(S3Common, HeaderContent) {
    HeaderContent *h = new HeaderContent();
    EXPECT_NE(h, (void *)NULL);

    // test Add
    if (h) {
        ASSERT_TRUE(h->Add(HOST, HOSTSTR));
        ASSERT_TRUE(h->Add(RANGE, RANGESTR));
        ASSERT_TRUE(h->Add(CONTENTMD5, MD5STR));
    }

    // test Get
    if (h) {
        EXPECT_STREQ(HOSTSTR, h->Get(HOST));
        EXPECT_STREQ(RANGESTR, h->Get(RANGE));
        EXPECT_STREQ(MD5STR, h->Get(CONTENTMD5));
    }

    // test GetList
    if (h) {
        curl_slist *l = h->GetList();
        ASSERT_NE(l, (void *)NULL);
        if (l) {
            curl_slist_free_all(l);
        }
    }
    if (h) delete h;
}

TEST(S3Common, UrlOptions) {
    EXPECT_EQ(get_opt_s3("s3://neverland.amazonaws.com", "secret"),
              (char *)NULL);
    EXPECT_STREQ(
        get_opt_s3("s3://neverland.amazonaws.com secret=secret_test", "secret"),
        "secret_test");
    EXPECT_STREQ(
        get_opt_s3(
            "s3://neverland.amazonaws.com accessid=\".\\!@#$%^&*()DFGHJK\"",
            "accessid"),
        "\".\\!@#$%^&*()DFGHJK\"");
    EXPECT_STREQ(get_opt_s3("s3://neverland.amazonaws.com chunksize=3456789",
                            "chunksize"),
                 "3456789");

    EXPECT_STREQ(
        get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                   "accessid=\".\\!@#$%^&*()DFGHJK\" chunksize=3456789",
                   "secret"),
        "secret_test");
    EXPECT_STREQ(
        get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                   "accessid=\".\\!@#$%^&*()DFGHJK\" chunksize=3456789",
                   "accessid"),
        "\".\\!@#$%^&*()DFGHJK\"");
    EXPECT_STREQ(
        get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                   "accessid=\".\\!@#$%^&*()DFGHJK\" chunksize=3456789",
                   "chunksize"),
        "3456789");

    EXPECT_STREQ(get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                            "blah=whatever accessid=\".\\!@#$%^&*()DFGHJK\" "
                            "chunksize=3456789 KingOfTheWorld=sanpang",
                            "secret"),
                 "secret_test");
    EXPECT_STREQ(get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                            "blah= accessid=\".\\!@#$%^&*()DFGHJK\" "
                            "chunksize=3456789 KingOfTheWorld=sanpang",
                            "secret"),
                 "secret_test");

    EXPECT_EQ(get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                         "blah=whatever accessid= chunksize=3456789 "
                         "KingOfTheWorld=sanpang",
                         "accessid"),
              (char *)NULL);
    EXPECT_EQ(get_opt_s3(NULL, "accessid"), (char *)NULL);
    EXPECT_EQ(get_opt_s3("", "accessid"), (char *)NULL);
    EXPECT_EQ(
        get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                   "blah=whatever chunksize=3456789 KingOfTheWorld=sanpang",
                   ""),
        (char *)NULL);
    EXPECT_EQ(
        get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                   "blah=whatever chunksize=3456789 KingOfTheWorld=sanpang",
                   NULL),
        (char *)NULL);
    EXPECT_STREQ(get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                            "chunksize=3456789 KingOfTheWorld=sanpang ",
                            "chunksize"),
                 "3456789");
    EXPECT_STREQ(get_opt_s3("s3://neverland.amazonaws.com   secret=secret_test "
                            "chunksize=3456789  KingOfTheWorld=sanpang ",
                            "chunksize"),
                 "3456789");
    EXPECT_EQ(get_opt_s3("s3://neverland.amazonaws.com secret=secret_test "
                         "chunksize=3456789 KingOfTheWorld=sanpang ",
                         "chunk size"),
              (char *)NULL);
}
