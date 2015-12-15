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
    const char *teststr = "aaabbbcccaaatttaaa";
    EXPECT_EQ(find_Nth(teststr, 1, "aaa"), 0);
    EXPECT_EQ(find_Nth(teststr, 2, "aaa"), 9);
    EXPECT_EQ(find_Nth(teststr, 3, "aaa"), 15);
    EXPECT_EQ(find_Nth(teststr, 1, "abc"), -1);
    EXPECT_EQ(find_Nth(teststr, 1, ""), 0);
}

#define MD5TESTBUF "abcdefghijklmnopqrstuvwxyz\n"
TEST(utils, md5) {
    MD5Calc m;
    m.Update(MD5TESTBUF, strlen(MD5TESTBUF));
    EXPECT_STREQ("e302f9ecd2d189fa80aac1c3392e9b9c", m.Get());
    m.Update(MD5TESTBUF, strlen(MD5TESTBUF));
    m.Update(MD5TESTBUF, strlen(MD5TESTBUF));
    m.Update(MD5TESTBUF, strlen(MD5TESTBUF));
    EXPECT_STREQ("3f8c2c6e2579e864071c33919fac61ee", m.Get());
}

#define TEST_STRING "The quick brown fox jumps over the lazy dog"
TEST(utils, base64) {
    EXPECT_STREQ("VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw==",
                 Base64Encode(TEST_STRING, strlen(TEST_STRING)));
}

TEST(utils, sha256) {
    char hash_str[65] = {0};
    EXPECT_TRUE(sha256(TEST_STRING, hash_str));
    EXPECT_STREQ(
        "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592",
        hash_str);
}

TEST(utils, sha256hmac) {
    char hash_str[65] = {0};
    EXPECT_TRUE(sha256hmac(TEST_STRING, hash_str, "key"));
    EXPECT_STREQ(
        "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
        hash_str);
}
