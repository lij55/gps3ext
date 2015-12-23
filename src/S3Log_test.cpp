#include "gtest/gtest.h"
#include "S3Log.cpp"

TEST(logger, simple) {
	S3DEBUG<<"debug message"<<std::endl;
	S3INFO<<"info message"<<std::endl;
	S3WARN<<"warning message"<<std::endl;
	S3ERROR<<"error message"<<std::endl;
}
