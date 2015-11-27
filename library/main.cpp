#include "S3Downloader.h"


//#define ASMAIN

#define ASMAIN
#ifdef ASMAIN
#include <iostream>
//#include "spdlog/spdlog.h"

void printdata(const char* url, uint64_t len, S3Credential* pcred);

int main()
{
    S3Credential cred;
    cred.keyid = "AKIAIAFSMJUMQWXB2PUQ";
    cred.secret = "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";

    const char* bucket = "metro.pivotal.io";
    const char* prefix = "data";
    const char* region = "us-west-2";

    int segid = 3;
    int total_segs = 4;

    ListBucketResult* r = ListBucket("s3-us-west-2.amazonaws.com", bucket, prefix, cred);

    char urlbuf[256];

    vector<BucketContent*>::iterator i;
    for( i = r->contents.begin(); i != r->contents.end(); i++ ) {
        BucketContent* p = *i;
        sprintf(urlbuf, "http://s3-us-west-2.amazonaws.com/%s/%s", bucket,p->Key());
        printf("%s, %d\n", urlbuf, p->Size());
        //printdata(urlbuf, p->Size(), &cred);
    }

    delete r;
    return 0;
}


#endif // ASMAIN



void printdata(const char* url, uint64_t len, S3Credential* pcred) {
    /* code */
    Downloader* f = new Downloader(4);
    f->init(url, len, 1 * 1024 * 1024, pcred);
    char* data = (char*) malloc(4096);
    if(!data) {
        return;
    }
    len     = 4096;

    while(f->get(data, len)) {
        fprintf(stdout, "%.*s", (int)len, data);
        len = 4096;
    }
    if(len > 0)
        fprintf(stdout, "%.*s", (int)len, data);

    free(data);
    f->destroy();
    delete f;
}


#if 0

int main(int argc, char const *argv[])
{
    // filepath and file length
    if(argc < 3) {
        printf("not enough parameters\n");
        return 1;
    }
    uint64_t len = atoll(argv[2]);

    /* code */
    Downloader* f = new Downloader(8);
    f->init(argv[1], len, 64 * 1024 * 1024);
    char* data = (char*) malloc(4096);
    if(!data) {
        return 0;
    }
    len     = 4096;
    int fd = open("data.bin", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd == -1) {
        perror("create file error");
        if(data)
            free(data);
        return 1;
    }
    while(f->get(data, len)) {
        write(fd, data, len);
        len = 4096;
    }
    if(len > 0)
        write(fd, data, len);
    std::cout<<"exiting"<<std::endl;
    free(data);
    close(fd);
    f->destroy();
    delete f;
    return 0;
}

#endif // ASMAIN
