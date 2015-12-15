#include "gtest/gtest.h"
#include "S3ExtWrapper.cpp"

class S3Reader_fake : public S3Reader {
   public:
    S3Reader_fake(const char *url);
    virtual ~S3Reader_fake();
    virtual bool Init(int segid, int segnum, int chunksize);
    virtual bool Destroy();

   protected:
    virtual string getKeyURL(const string &key);
    virtual bool ValidateURL();
};

S3Reader_fake::S3Reader_fake(const char *url) : S3Reader(url) {}

S3Reader_fake::~S3Reader_fake() {}

bool S3Reader_fake::Destroy() {
    // reset filedownloader
    if (this->filedownloader) {
        this->filedownloader->destroy();
        delete this->filedownloader;
    }

    // Free keylist
    if (this->keylist) {
        delete this->keylist;
    }
    return true;
}

bool S3Reader_fake::Init(int segid, int segnum, int chunksize) {
    // set segment id and num
    this->segid = segid;    // fake
    this->segnum = segnum;  // fake
    this->contentindex = this->segid;

    this->chunksize = chunksize;

    // Validate url first
    if (!this->ValidateURL()) {
        EXTLOG("validate url fail %s\n", this->url.c_str());
    }

    // TODO: As separated function for generating url
    this->keylist = ListBucket_FakeHTTP("localhost", "metro.pivotal.io");

    if (!this->keylist) {
        return false;
    }

    this->getNextDownloader();

    return this->filedownloader ? true : false;
}

string S3Reader_fake::getKeyURL(const string &key) {
    stringstream sstr;
    sstr << this->schema << "://"
         << "localhost/";
    sstr << this->bucket << "/" << key;
    return sstr.str();
}

bool S3Reader_fake::ValidateURL() {
    this->schema = "http";

    this->region = "raycom";

    this->bucket = "metro.pivotal.io";

    this->prefix = "";

    // EXTLOG("schema: %s ", this->schema);
    // EXTLOG("region: %s ", this->region);
    // EXTLOG("bucket: %s ", this->bucket);
    // EXTLOG("prefix: %s ", this->prefix);
    return true;
}

void ExtWrapperTest(const char *url, uint64_t buffer_size,
                    const char *md5_str) {
    MD5Calc m;
    S3ExtBase *myData;
    uint64_t nread = 0;
    uint64_t buf_len = buffer_size;
    char *buf = (char *)malloc(buffer_size);

    ASSERT_NE((void *)NULL, buf);

    InitLog();

    if (strncmp(url, "http://localhost/", 17) == 0) {
        myData = new S3Reader_fake(url);
    } else {
        myData = new S3Reader(url);
    }

    ASSERT_NE((void *)NULL, myData);
    ASSERT_TRUE(myData->Init(0, 1, 64 * 1024 * 1024));

    while (1) {
        nread = buf_len;

        myData->TransferData(buf, nread);

        if (nread == 0) break;

        m.Update(buf, nread);
    }

    EXPECT_STREQ(md5_str, m.Get());

    delete myData;
    free(buf);
}

#ifdef AWSTEST

TEST(ExtWrapper, normal) {
    ExtWrapperTest("http://s3-us-west-2.amazonaws.com/metro.pivotal.io/data/",
                   64 * 1024, "138fc555074671912125ba692c678246");
}

#endif

TEST(FakeExtWrapper, normal) {
    ExtWrapperTest("http://localhost/metro.pivotal.io/", 64 * 1024,
                   "138fc555074671912125ba692c678246");
}
