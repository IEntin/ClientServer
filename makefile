#LD_PRELOAD=$LD_PRELOAD:/usr/local/jemalloc-dev/lib/libjemalloc.so ./server
#LD_PRELOAD=$LD_PRELOAD:/usr/local/jemalloc-dev/lib/libjemalloc.so ./client
#use clang++ (CMPLR=clang++) for sanitized build if use jemalloc
#gprof -b ./server gmon.out > profile.txt
#valgrind --tool=callgrind ./server
#kcachegrind callgrind.out.*
#valgrind --tool=massif ./server
#ms_print massif.out.*

#make PROFILE=[  | 1]
#make SANITIZE=[  | aul | thread ]
#make CMPLR=[ g++ | clang++ ]

CMPLR=
ifeq ($(CMPLR),)
	CXX=g++
else
	CXX=$(CMPLR)
endif

TESTCLIENTDIR = testClient

all: server $(TESTCLIENTDIR)/client

OPTIMIZE=
ifeq ($(OPTIMIZE),)
	OPTIMIZATION=-O3
else
	OPTIMIZATION=$(OPTIMIZE)
endif
SANITIZE=
SANBLD=
PROFILE=
PROFBLD=

ifeq ($(SANITIZE), aul)
	SANBLD=-fsanitize=address,undefined,leak
else ifeq ($(SANITIZE), thread)
	SANBLD=-fsanitize=thread
endif

ifeq ($(PROFILE), 1)
	PROFBLD=-fno-omit-frame-pointer -pg
endif

BOOSTDIR:=/usr/local/boost_1_77_0
BOOSTLIBDIR:=$(BOOSTDIR)/stage/lib/
BOOST_INCLUDE:=-I$(BOOSTDIR)

WARNINGS = -Wall -pedantic-errors

SERVERINCLUDES:=-I. -Icommon $(BOOST_INCLUDE) 
SERVERSOURCES=$(wildcard *.cpp) $(wildcard common/*.cpp) $(wildcard common/*.c)

server : $(SERVERSOURCES) *.h common/*.h
	$(CXX) -g -MMD -std=c++2a $(WARNINGS) $(SERVERINCLUDES) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) $(SERVERSOURCES) -pthread -o $@

CLIENTINCLUDES = -Iclient_src -Icommon $(BOOST_INCLUDE) 
CLIENTSOURCES=$(wildcard client_src/*.cpp) $(wildcard common/*.cpp) $(wildcard common/*.c)

$(TESTCLIENTDIR)/client : $(CLIENTSOURCES) common/*.h client_src/FifoClient.h
	$(CXX) -g -MMD -std=c++2a $(WARNINGS) $(CLIENTINCLUDES) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) $(CLIENTSOURCES) -pthread -o $@

.PHONY: clean
clean:
	rm -f *.d server $(TESTCLIENTDIR)/client $(TESTCLIENTDIR)/*.d $(TESTCLIENTDIR)/gmon.out
	rm -f gmon.out *.gcov *.gcno *.gcda
