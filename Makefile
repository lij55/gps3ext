MODULE_big = gps3ext
OBJS = src/gps3ext.o src/S3ExtWrapper.o src/lib/http_parser.o src/S3Common.o src/S3Downloader.o src/utils.o

PG_CPPFLAGS = -I$(libpq_srcdir) -Isrc -Isrc/lib -fPIC -std=c++0x -I/usr/include/libxml2
SHLIB_LINK = -lstdc++ -lxml2 -lpthread -lcrypto -lcurl

PGXS := $(shell pg_config --pgxs)
include $(PGXS)
