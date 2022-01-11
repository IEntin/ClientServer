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

CLIENTBINDIR = client_bin

TESTDIR = tests

all: server $(CLIENTBINDIR)/client $(TESTDIR)/runtests

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

WARNINGS = -Wall -pedantic-errors

SERVERINCLUDES:=-I. -Icommon 
SERVERSOURCES=$(wildcard *.cpp) $(wildcard common/*.cpp)

server : $(SERVERSOURCES) *.h common/*.h
	$(CXX) -g -MMD -std=c++2a $(WARNINGS) $(SERVERINCLUDES) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) $(SERVERSOURCES) -pthread -o $@

CLIENTINCLUDES = -Iclient_src -Icommon 
CLIENTSOURCES=$(wildcard client_src/*.cpp) $(wildcard common/*.cpp)

$(CLIENTBINDIR)/client : $(CLIENTSOURCES) common/*.h client_src/FifoClient.h
	$(CXX) -g -MMD -std=c++2a $(WARNINGS) $(CLIENTINCLUDES) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) $(CLIENTSOURCES) -pthread -o $@

TESTINCLUDES = -Iclient_src -Icommon 
TESTSOURCES=$(wildcard $(TESTDIR)/*.cpp) $(wildcard common/*.cpp)

$(TESTDIR)/runtests : $(TESTSOURCES)
	$(CXX) -g -MMD -std=c++2a $(WARNINGS) $(TESTINCLUDES) $(OPTIMIZATION) $(SANBLD) $(TESTSOURCES) -lgtest -lgtest_main -pthread -o $@

.PHONY: clean
clean:
	rm -f *.d server $(CLIENTBINDIR)/client $(CLIENTBINDIR)/*.d $(CLIENTBINDIR)/gmon.out $(TESTDIR)/runtests $(TESTDIR)/*.d
	rm -f gmon.out *.gcov *.gcno *.gcda
