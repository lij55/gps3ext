#include "S3Common.cpp"
#include "gtest/gtest.h"

TEST(S3Common, UrlParser) {
    UrlParser* p = new UrlParser(
        "https://www.google.com/search?sclient=psy-ab&site=&source=hp");
    EXPECT_NE(p, (void*)NULL);
    if (p) {
        EXPECT_STREQ(p->Schema(), "https");
        EXPECT_STREQ(p->Host(), "www.google.com");
        EXPECT_STREQ(p->Path(), "/search");
        delete p;
    }
}

TEST(S3Common, UrlParser_LongURL) {
    UrlParser* p = new UrlParser(
        "http://s3-us-west-2.amazonaws.com/metro.pivotal.io/test/"
        "data1234?partNumber=1&uploadId=."
        "CXn7YDXxGo7aDLxEyX5wxaDivCw5ACWfaMQts8_4M6."
        "NbGeeaI1ikYlO5zWZOpclVclZRAq5758oCxk_DtiX5BoyiMr7Ym6TKiEqqmNpsE-");
    EXPECT_NE(p, (void*)NULL);
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
    HeaderContent* h = new HeaderContent();
    EXPECT_NE(h, (void*)NULL);

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
        curl_slist* l = h->GetList();
        ASSERT_NE(l, (void*)NULL);
        if (l) {
            curl_slist_free_all(l);
        }
    }
    if (h) delete h;
}

TEST(S3Common, Config) {
    auto c = GetGlobalS3Config();
    // EXPECT_STREQ(c.Get("logserver", "server", "") ,"127.0.0.1");
    // EXPECT_STREQ(c.Get("logserver", "serverxx", "tttt") ,"tttt");
    EXPECT_STREQ(c.Get("testbucket", "accesskey", "tttt") ,"testaccesskey");
    EXPECT_STREQ(c.Get("testbucket", "accesskeyaa", "tttt") ,"tttt");
    EXPECT_STREQ(c.Get("testbucket", "secret", "tttt") ,"testsecret");
    EXPECT_STREQ(c.Get("testbucket", "secretaa", "tttt") ,"tttt");
    EXPECT_STREQ(c.Get("testbucket", "token", "tttt") ,"testtoken");
    EXPECT_STREQ(c.Get("testbucket", "tokenaa", "tttt") ,"tttt");
}
