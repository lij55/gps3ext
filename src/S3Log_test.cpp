#include "gtest/gtest.h"
#include "S3Log.cpp"

TEST(logger, simple) {
    S3DEBUG("hello");
    S3ERROR("hello");
}
