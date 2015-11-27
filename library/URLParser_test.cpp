#include <URLParser.cpp>

#include "gtest/gtest.h"

// int main(int argc, char* argv[]) {
// 	if(argc < 2) {
// 		std::cout<<"not enough args\n";
// 		return 1;
// 	}
// 	UrlParser* p = new UrlParser(argv[1]);
// 	std::cout<<(char*)NULL<<std::endl;
// 	std::cout<<p->Schema()<<std::endl;
// 	std::cout<<p->Host()<<std::endl;
// 	std::cout<<p->Path()<<std::endl;
// 	delete p;
// 	return 0;
// }

TEST (URLParser, simple ) {
    UrlParser* p = new UrlParser("https://www.google.com/search?sclient=psy-ab&site=&source=hp");
    EXPECT_STREQ(p->Schema(), "https");
    EXPECT_STREQ(p->Host(), "www.google.com");
    EXPECT_STREQ(p->Path(), "/search");
    delete p;

}
