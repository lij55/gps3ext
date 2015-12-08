#include "gtest/gtest.h"

#include "utils.cpp"
TEST(utils, lower) {
    char data[] = "aAbBcCdDEeFfGgHhIiJjKkLlMmNnOopPQqRrSsTtuUVvwWxXYyZz";
    tolower(data);
    EXPECT_STREQ("aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz", data);
}

// Tests factorial of positive numbers.
TEST(utils, trim) {
    char data[] = " \t\n\r  abc \r\r\n\r \t";
    char out[8] = {0};
    bool ret;
    ret = trim(out, data);
    EXPECT_EQ(ret, true);
    EXPECT_STREQ("abc", out);
}

TEST(utils, time) {
    char data[65];
    gethttpnow(data);
}

TEST(signature, v2) {}

#include <curl/curl.h>
TEST(utils, simplecurl) {
    CURL *c = CreateCurlHandler(NULL);
    EXPECT_EQ(c, (void *)NULL);
    c = CreateCurlHandler("www.google.com");
    EXPECT_NE(c, (void *)NULL);
    curl_easy_cleanup(c);
}


TEST(utils, nth) {
	EXPECT_TRUE(0);
}

#define MD5TESTBUF "abcdefghijklmnopqrstuvwxyz\n"
TEST(utils, md5) {
	MD5Calc m;
	m.Update(MD5TESTBUF, strlen(MD5TESTBUF));
	EXPECT_STREQ("e302f9ecd2d189fa80aac1c3392e9b9c",m.Get());
	m.Update(MD5TESTBUF, strlen(MD5TESTBUF));
	m.Update(MD5TESTBUF, strlen(MD5TESTBUF));
	m.Update(MD5TESTBUF, strlen(MD5TESTBUF));
	EXPECT_STREQ("3f8c2c6e2579e864071c33919fac61ee",m.Get());
}
