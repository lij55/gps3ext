#include "S3Common.cpp"
#include "gtest/gtest.h"

TEST(S3Common, UrlParser) {
	UrlParser *p = new UrlParser(
								 "https://www.google.com/search?sclient=psy-ab&site=&source=hp");
	EXPECT_NE(p, (void*)NULL);
	if(p) {
		EXPECT_STREQ(p->Schema(), "https");
		EXPECT_STREQ(p->Host(), "www.google.com");
		EXPECT_STREQ(p->Path(), "/search");
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
	if(h) {
	    ASSERT_TRUE(h->Add(HOST, HOSTSTR));
		ASSERT_TRUE(h->Add(RANGE, RANGESTR));
		ASSERT_TRUE(h->Add(CONTENTMD5, MD5STR));
	}

	// test Get
	if(h) {
		EXPECT_STREQ(HOSTSTR, h->Get(HOST));
		EXPECT_STREQ(RANGESTR, h->Get(RANGE));
		EXPECT_STREQ(MD5STR, h->Get(CONTENTMD5));
	}


	// test GetList
	if(h) {
		curl_slist* l = h->GetList();
		ASSERT_NE(l, (void*) NULL);
		if(l) {
			curl_slist_free_all(l);
		}
	}
	if(h)
		delete h;
}
