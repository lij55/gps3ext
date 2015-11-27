#include "S3Downloader.h"

#include <algorithm>    // std::min
#include <sstream>
#include <iostream>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "utils.h"


#define __STDC_FORMAT_MACROS
#include <inttypes.h>

OffsetMgr::OffsetMgr(uint64_t m, uint64_t c)
    :maxsize(m)
    ,chunksize(c)
    ,curpos(0)
{
    pthread_mutex_init(&this->offset_lock, NULL);
}

Range OffsetMgr::NextOffset() {
    Range ret;
    pthread_mutex_lock(&this->offset_lock);
    if(this->curpos < this->maxsize) {
        ret.offset = this->curpos;
    } else {
        ret.offset = this->maxsize;
    }

    if(this->curpos + this->chunksize > this->maxsize) {
        ret.len = this->maxsize - this->curpos;
        this->curpos = this->maxsize;
    }
    else {
        ret.len = this->chunksize;
        this->curpos += this->chunksize;
    }
    pthread_mutex_unlock(&this->offset_lock);
    //std::cout<<ret.offset<<std::endl;
    return ret;
}

void OffsetMgr::Reset(uint64_t n) {
    pthread_mutex_lock(&this->offset_lock);
    this->curpos = n;
    pthread_mutex_unlock(&this->offset_lock);
}


BlockingBuffer::BlockingBuffer(const char* url, OffsetMgr* o)
    :sourceurl(url)
    ,readpos(0)
    ,realsize(0)
    ,status(BlockingBuffer::STATUS_EMPTY)
    ,eof(false)
    ,mgr(o)
    ,error(false)
{
    this->nextpos = o->NextOffset();
    this->bufcap = o->Chunksize();
}

BlockingBuffer::~BlockingBuffer() {
    if(this->bufferdata) {
        free(this->bufferdata);
        pthread_mutex_destroy(&this->stat_mutex);
        pthread_cond_destroy(&this->stat_cond);
    }

};

bool BlockingBuffer::Init() {
    this->bufferdata = (char*) malloc(this->bufcap);
    if(!this->bufferdata) {
        // malloc fail
        std::cout<<"alloc error"<<std::endl;
        return false;
    }
    pthread_mutex_init(&this->stat_mutex, NULL);
    pthread_cond_init (&this->stat_cond, NULL);
    return true;
}



// ret < len means EMPTY
uint64_t BlockingBuffer::Read(char* buf, uint64_t len) {
    // assert buf not null
    // assert len > 0, len < this->bufcap
    pthread_mutex_lock(&this->stat_mutex);
    while (this->status == BlockingBuffer::STATUS_EMPTY) {
        pthread_cond_wait(&this->stat_cond, &this->stat_mutex);
    }
    uint64_t left_data_length = this->realsize - this->readpos;
    int length_to_read = std::min(len, left_data_length);

    memcpy(buf, this->bufferdata + this->readpos, length_to_read);
    if(left_data_length >= len) {
        this->readpos += len; // not empty
    } else { // empty, reset everything
        this->readpos = 0;
        if(this->status == BlockingBuffer::STATUS_READY)
            this->status = BlockingBuffer::STATUS_EMPTY;
        if(!this->EndOfFile()) {
            this->nextpos = this->mgr->NextOffset();
            pthread_cond_signal(&this->stat_cond);
        }
    }
    pthread_mutex_unlock(&this->stat_mutex);
    return length_to_read;
}

uint64_t BlockingBuffer::Fill() {
    // assert offset > 0, offset < this->bufcap
    pthread_mutex_lock(&this->stat_mutex);
    while (this->status == BlockingBuffer::STATUS_READY) {
        pthread_cond_wait(&this->stat_cond, &this->stat_mutex);
    }
    uint64_t offset = this->nextpos.offset;
    uint64_t leftlen = this->nextpos.len;
    // assert this->status != BlockingBuffer::STATUS_READY
    int readlen = 0;
    this->realsize = 0;
    while(this->realsize < this->bufcap) {
        if(leftlen != 0 ) {
            //readlen = this->fetchdata(offset, this->bufferdata + this->realsize, this->bufcap - this->realsize);
            readlen = this->fetchdata(offset, this->bufferdata + this->realsize, leftlen);
            //std::cout<<"return "<<readlen<<" from libcurl\n";
        } else {
            readlen = 0; // EOF
        }
        if(readlen == 0) {// EOF!!
            if(this->realsize == 0) {
                this->eof = true;
            }
            std::cout<<"reach end of file ?"<<std::endl;
            break;
        } else if (readlen == -1) { // Error, network error or sth.
            // perror, retry
            this->error = true;
            // Ensure status is still empty
            //this->status = BlockingBuffer::STATUS_READY;
            //pthread_cond_signal(&this->stat_cond);
            break;
        } else { // > 0
            offset += readlen;
            leftlen -= readlen;
            this->realsize += readlen;
            // this->status = BlockingBuffer::STATUS_READY;
        }
    }
    this->status = BlockingBuffer::STATUS_READY;
    if(this->realsize >= 0) {
        pthread_cond_signal(&this->stat_cond);
    }

    pthread_mutex_unlock(&this->stat_mutex);
    return (readlen == -1) ? -1 : this->realsize ;
}


BlockingBuffer* BlockingBuffer::CreateBuffer(const char* url, OffsetMgr* o, S3Credential* pcred) {
    BlockingBuffer* ret = NULL;
    if ( url == NULL )
        return NULL;

    if(pcred) {
        ret = new S3Fetcher(url, o, *pcred);
    } else {
        ret = new HTTPFetcher(url, o);
    }

    return ret;
}


void* DownloadThreadfunc(void* data) {
    BlockingBuffer* buffer = (BlockingBuffer*)data;
    size_t filled_size = 0;
    void* pp = (void*)pthread_self();
    // assert offset > 0
    do {
        filled_size = buffer->Fill();
        std::cout<<"Fillsize is "<<filled_size<<" of "<<pp<<std::endl;
        if(buffer->EndOfFile())
            break;
        if (filled_size == -1) { // Error
            // retry?
            if(buffer->Error()) {
                break;
            } else
                continue;
        }
    } while(1);
    std::cout<<"quit\n";
    return NULL;
}

Downloader::Downloader(uint8_t part_num)
    :num(part_num)
{
    this->threads = (pthread_t*) malloc(num * sizeof(pthread_t));
    this->buffers = (BlockingBuffer**) malloc (num * sizeof(BlockingBuffer*));
}

bool Downloader::init(const char* url, uint64_t size, uint64_t chunksize, S3Credential* pcred) {
    this->o = new OffsetMgr(size, chunksize);
    for(int i = 0; i < this->num; i++) {
        this->buffers[i] = BlockingBuffer::CreateBuffer(url, o, pcred);  // decide buffer according to url
        if(!this->buffers[i]->Init()) {
            std::cerr<<"init fail"<<std::endl;
        }
        pthread_create(&this->threads[i], NULL, DownloadThreadfunc, this->buffers[i]);
    }
    readlen = 0;
    chunkcount = 0;
}

bool Downloader::get(char* data, uint64_t& len) {
    uint64_t filelen = this->o->Size();
    BlockingBuffer* buf = buffers[this->chunkcount % this->num];
    uint64_t tmplen = buf->Read(data, len);
    this->readlen += tmplen;
    if(tmplen < len) {
        this->chunkcount++;
        len = tmplen;
        if(buf->Error()) {
            return false;
        }
    }
    if(this->readlen ==  filelen) {
        if(buf->EndOfFile())
            return false;
    }
    return true;
}

void Downloader::destroy() {
    // if error
    for(int i = 0; i < this->num; i++) {
        pthread_cancel(this->threads[i]);
    }
    for(int i = 0; i < this->num; i++) {
        pthread_join(this->threads[i],NULL);
        delete this->buffers[i];

    }
    delete this->o;
}

Downloader::~Downloader() {
    free(this->threads);
    free(this->buffers);
}



static uint64_t WriterCallback(void *contents, uint64_t size, uint64_t nmemb, void *userp)
{
    uint64_t realsize = size * nmemb;
    Bufinfo *p = (Bufinfo*) userp;
    //std::cout<<"in writer"<<std::endl;
    // assert p->len + realsize < p->maxsize
    memcpy(p->buf + p->len, contents, realsize);
    p->len += realsize;
    return realsize;
}

HTTPFetcher::HTTPFetcher(const char* url, OffsetMgr* o)
    :BlockingBuffer(url, o)
    ,urlparser(url)
{
    this->curl = curl_easy_init();
    //curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:8080");
    curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, WriterCallback);
    curl_easy_setopt(this->curl, CURLOPT_FORBID_REUSE, 1L);
    this->AddHeaderField(HOST,urlparser.Host());
}

HTTPFetcher::~HTTPFetcher() {
    curl_easy_cleanup(this->curl);
}

bool HTTPFetcher::SetMethod(Method m) {
    this->method = m;
    return true;
}

bool HTTPFetcher::AddHeaderField(HeaderField f, const char* v) {
    if(v == NULL) {
        // log warning
        return false;
    }
    return this->headers.Add(f, v);
}

// buffer size should be at lease len
// read len data from offest
uint64_t HTTPFetcher::fetchdata(uint64_t offset, char* data, uint64_t len) {
    if(len == 0)  return 0;

RETRY:
    Bufinfo bi;
    bi.buf = data;
    bi.maxsize = len;
    bi.len = 0;

    CURL *curl_handle = this->curl;

    char rangebuf[128];

    curl_easy_setopt(curl_handle, CURLOPT_URL, this->sourceurl);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&bi);

    sprintf(rangebuf, "bytes=%" PRIu64 "-%" PRIu64, offset, offset + len - 1);
    this->AddHeaderField(RANGE, rangebuf);

    this->processheader();

    struct curl_slist *chunk = this->headers.GetList();
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        bi.len = -1;
    } else {
        curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &res);
        if(this->retry(res))
            goto RETRY;
        if(! ((res == 200) || (res == 206)) ) {
            fprintf(stderr, "%.*s", (int)len, data);
            bi.len = -1;
        }
    }

    return bi.len;
}


S3Fetcher::S3Fetcher(const char* url, OffsetMgr* o, const S3Credential& cred)
    :HTTPFetcher(url, o)
{
    this->cred = cred;
}

bool S3Fetcher::processheader() {
    return SignGetV2(&this->headers, this->urlparser.Path(), this->cred);
}

bool S3Fetcher::retry(CURLcode c) {
    if(c == 403)
        return true;
    else
        return false;
}


//CreateBucketContentItem
BucketContent::~BucketContent() {
    if(this->key) {
        free((void*)this->key);
    }
}

BucketContent::BucketContent()
    :key(NULL)
    ,size(0)
{
}

BucketContent* CreateBucketContentItem(const char* key, uint64_t size) {
    if(!key)
        return NULL;
    const char* tmp = strdup(key);
    if(!tmp)
        return NULL;
    BucketContent* ret = new BucketContent();
    if(!ret) {
        free((void*)tmp);
        return NULL;
    }
    ret->key = tmp;
    ret->size = size;
    return ret;
}

struct XMLInfo {
    xmlParserCtxtPtr ctxt;
};



//void S3fetch_curl(const char* url, )
static uint64_t ParserCallback(void *contents, uint64_t size, uint64_t nmemb, void *userp)
{
    uint64_t realsize = size * nmemb;
    int res;
    //printf("%.*s",realsize, (char*)contents);
    struct XMLInfo *pxml = (struct XMLInfo*)userp;
    if(!pxml->ctxt) {
        pxml->ctxt = xmlCreatePushParserCtxt(NULL, NULL,
                                             (const char*)contents, realsize, "resp.xml");
        return realsize;
    } else {
        res = xmlParseChunk(pxml->ctxt, (const char*) contents, realsize, 0);
    }
    return realsize;
}

// require curl 7.17 higher
xmlParserCtxtPtr DoGetXML(const char* host, const char* bucket, const char* url, const S3Credential &cred) {
    CURL *curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

    } else {
        return NULL;
    }
    std::stringstream sstr;
    XMLInfo xml;
    xml.ctxt = NULL;

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ParserCallback);

    HeaderContent* header = new HeaderContent();
    sstr<<bucket<<".s3.amazonaws.com";
    header->Add(HOST, host);
    UrlParser p(url);
    SignGetV2(header, p.Path(), cred);

    struct curl_slist * chunk = header->GetList();

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode res = curl_easy_perform(curl);

    if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    xmlParseChunk(xml.ctxt, "", 0, 1);
    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    return xml.ctxt;
}


bool BucketContentComp(BucketContent* a,BucketContent* b)
{
    return strcmp(a->Key(), b->Key()) > 0;
}


ListBucketResult*
ListBucket(const char* host, const char* bucket, const char* prefix, const S3Credential &cred) {
    std::stringstream sstr;
    if(prefix) {
        sstr<<"http://"<<host<<"/"<<bucket<<"?prefix="<<prefix;
        //sstr<<"http://"<<bucket<<"."<<host<<"?prefix="<<prefix;
    } else {
        sstr<<"http://"<<bucket<<"."<<host;
    }

    xmlParserCtxtPtr xmlcontext = DoGetXML(host, bucket, sstr.str().c_str(), cred);
    xmlNode *root_element = xmlDocGetRootElement(xmlcontext->myDoc);
    ListBucketResult* result = new ListBucketResult();

    xmlNodePtr cur;
    cur = root_element->xmlChildrenNode;
    while(cur != NULL) {
        if(!xmlStrcmp(cur->name, (const xmlChar *)"Name")) {
            result->Name = (const char*)xmlNodeGetContent(cur);
        }

        if(!xmlStrcmp(cur->name, (const xmlChar *)"Prefix")) {
            result->Prefix = (const char*)xmlNodeGetContent(cur);
        }

        if(!xmlStrcmp(cur->name, (const xmlChar *)"Contents")) {
            xmlNodePtr contNode = cur->xmlChildrenNode;
            const char* key;
            uint64_t size;
            while(contNode != NULL) {
                if(!xmlStrcmp(contNode->name, (const xmlChar *)"Key")) {
                    key = (const char*)xmlNodeGetContent(contNode);
                }
                if(!xmlStrcmp(contNode->name, (const xmlChar *)"Size")) {
                    xmlChar* v = xmlNodeGetContent(contNode);
                    size = atoll((const char*)v);
                }
                contNode = contNode->next;
            }
            BucketContent *item = CreateBucketContentItem(key, size);
            if(item)
                result->contents.push_back(item);
            else {
                // log error here
            }
        }
        cur = cur->next;
    }

    /* always cleanup */
    xmlFreeParserCtxt(xmlcontext);
    sort(result->contents.begin(), result->contents.end(), BucketContentComp);

    return result;
}

