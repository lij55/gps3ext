MODULE_big = gpexts3
OBJS       = gpexts3.o S3ExtWrapper.o http_parser.o  S3Common.o  S3Downloader.o utils.o

PG_CPPFLAGS = -I$(libpq_srcdir) -fPIC -Isfetch -std=c++0x -I/usr/include/libxml2
SHLIB_LINK = -lstdc++
//CUSTOM_CC = g++

PGXS := $(shell pg_config --pgxs)
include $(PGXS)
