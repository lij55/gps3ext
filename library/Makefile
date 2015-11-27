
GTEST_DIR=gtest

CPP=g++
CFLAGS= -g -std=c++0x
INCLUDES = -I. -I/usr/include/libxml2 -Iinclude
LDFLAGS=-lpthread -lcrypto -lcurl -lxml2

sources = S3Downloader.cpp  utils.cpp extlib/http_parser.cpp \
    main.cpp  S3Common.cpp
testcources = utils_test.cpp OffsetMgr_test.cpp URLParser_test.cpp http_parser.cpp


objects = $(sources:.cpp=.o) 
testobjs = $(testcources:.cpp=.o) 

app = sfetch
testapp = $(app)_test

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)


# all: build buildtest
all: build

$(testobjs) gtest-all.o gtest_main.o: INCLUDES += -I$(GTEST_DIR)/include -I$(GTEST_DIR)/


# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) $(INCLUDES) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) $(INCLUDES)  $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

.PHONY: test clean

build: $(objects)
	g++  -o $(app) $(objects) $(csources:.c=.o)  $(LDFLAGS)

%.o: %.cpp 
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<  -o $@  


buildtest: gtest_main.a $(testobjs)
	$(CPP) $(CPPFLAGS) $(CXXFLAGS)  $^ -o $(testapp)  $(LDFLAGS)

test: buildtest
	@./$(testapp)


clean:
	rm -f *.o $(app) $(testapp) *.a a.out *.orig extlib/*.o

tags:
	@etags *.cpp *.h

lint:
	cppcheck -v --enable=warning *.cpp *.h

style:
	astyle -s4 *.cpp *.h
