MODULE_big = gps3ext
OBJS = src/gps3ext.o src/S3ExtWrapper.o lib/http_parser.o src/S3Common.o src/S3Downloader.o src/S3Uploader.o src/S3Operations.o src/utils.o

PG_CPPFLAGS = -std=c++11 -fPIC -I$(libpq_srcdir) -Isrc -Ilib -I/usr/include/libxml2
SHLIB_LINK = -lstdc++ -lxml2 -lpthread -lcrypto -lcurl

PGXS := $(shell pg_config --pgxs)
include $(PGXS)
