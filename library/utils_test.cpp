#include "gtest/gtest.h"

#include "utils.cpp"
TEST(utils, lower) {
    char data[]="aAbBcCdDEeFfGgHhIiJjKkLlMmNnOopPQqRrSsTtuUVvwWxXYyZz";
    tolower(data);
    EXPECT_STREQ("aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",data);
}

// Tests factorial of positive numbers.
TEST(utils, trim) {
    char data[] = " \t\n\r  abc \r\r\n\r \t";
    char out[8] = {0};
    bool ret;
    ret = trim(out, data);
    EXPECT_EQ(ret, true);
    EXPECT_STREQ("abc",out);
}

TEST(utils,time) {
    char data[65];
    gethttpnow(data);
}

TEST(signature, v2) {

}

#include <curl/curl.h>
TEST(curl, init) {

    CURL* c = CreateCurlHandler(NULL);
    EXPECT_EQ(c, (void*)NULL);
    c =  CreateCurlHandler("www.google.com");
    EXPECT_NE(c, (void*)NULL);
    curl_easy_cleanup(c);
}
