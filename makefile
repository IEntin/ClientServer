#LD_PRELOAD=$LD_PRELOAD:/usr/local/jemalloc-dev/lib/libjemalloc.so ./server
#LD_PRELOAD=$LD_PRELOAD:/usr/local/jemalloc-dev/lib/libjemalloc.so ./client
#use clang++ (CMPLR=clang++) for sanitized build if use jemalloc
#gprof -b ./server gmon.out > profile.txt
#valgrind --leak-check=yes ./server
#valgrind --tool=callgrind ./server
#kcachegrind callgrind.out.*
#valgrind --tool=massif ./server
#ms_print massif.out.*
#ps huH p <pid> | wc -l
#~/ClientServer/tests$ ./runtests --gtest_repeat=10 --gtest_break_on_failure

#make PROFILE=[  | 1]
#make SANITIZE=[  | aul | thread ]
#make CMPLR=[ g++ | clang++ ]

ifeq ($(CMPLR),)
	CXX=clang++
else
	CXX=$(CMPLR)
endif

CLIENTBINDIR = client_bin

TESTDIR = tests

# enable precompiled headers for clang++

ifeq ($(CXX),clang++)
	ALLH = common/all.h
	PCH = $(ALLH).pch
	INCLUDE_PRECOMPILED = -include $(ALLH)
endif

all: server $(CLIENTBINDIR)/client $(TESTDIR)/runtests

OPTIMIZE=
ifeq ($(OPTIMIZE),)
	OPTIMIZATION=-O3
else
	OPTIMIZATION=$(OPTIMIZE)
endif

ifeq ($(SANITIZE), aul)
	SANBLD=-fsanitize=address,undefined,leak
else ifeq ($(SANITIZE), thread)
	SANBLD=-fsanitize=thread
endif

ifeq ($(PROFILE), 1)
	PROFBLD=-fno-omit-frame-pointer -pg
endif

WARNINGS = -Wall -pedantic-errors

MACROS = -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE)

CPPFLAGS = -g $(INCLUDE_PRECOMPILED) -std=c++2a $(WARNINGS) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) $(MACROS) -pthread

$(PCH) : $(ALLH)
	@echo -n precompile start:
	@date
	$(CXX) -g -x c++-header $(CPPFLAGS) $(SERVERINCLUDES) $(CLIENTINCLUDES) $(TESTINCLUDES) $(ALLH) -o $@
	@echo -n precompile end:
	@date

SERVERINCLUDES:=-I. -Icommon -Ififo -Itcp
SERVERSOURCES=$(wildcard *.cpp) $(wildcard common/*.cpp) $(wildcard fifo/*.cpp) $(wildcard tcp/*.cpp)

server : $(SERVERSOURCES) $(PCH) *.h common/*.h fifo/*.h tcp/*.h
	@echo -n server start:
	@date
	$(CXX) $(CPPFLAGS) $(SERVERINCLUDES) $(SERVERSOURCES) -o $@
	@echo -n server end:
	@date

CLIENTINCLUDES = -Iclient_src -Icommon 
CLIENTSOURCES=$(wildcard client_src/*.cpp) $(wildcard common/*.cpp)

$(CLIENTBINDIR)/client : $(PCH) $(CLIENTSOURCES) common/*.h client_src/*.h
	@echo -n client start:
	@date
	$(CXX) $(CPPFLAGS) $(CLIENTINCLUDES) $(CLIENTSOURCES) -o $@
	@echo -n client end:
	@date

TESTINCLUDES = -I. -Itests -Iclient_src -Icommon -Ififo -Itcp
TESTSOURCES=$(wildcard $(TESTDIR)/*.cpp) $(wildcard common/*.cpp) $(wildcard fifo/*.cpp) $(wildcard tcp/*.cpp) $(filter-out client_src/Main.cpp, $(wildcard client_src/*.cpp)) $(filter-out Main.cpp, $(wildcard *.cpp))

$(TESTDIR)/runtests : $(PCH) $(TESTSOURCES) server $(CLIENTBINDIR)/client $(TESTDIR)/*.h
	@echo -n tests start:
	@date
	$(CXX) $(CPPFLAGS) $(TESTINCLUDES) $(TESTSOURCES) -lgtest -lgtest_main -o $@
	@echo -n tests end:
	@date
	cd $(TESTDIR); ln -sf ../$(CLIENTBINDIR)/requests.log .; ln -sf ../$(CLIENTBINDIR)/outputD.txt .; ln -sf ../$(CLIENTBINDIR)/outputND.txt .; ln -sf ../ads.txt .; ./runtests

.PHONY: clean
clean:
	rm -f */*.d server $(CLIENTBINDIR)/client gmon.out */gmon.out $(TESTDIR)/runtests *.gcov *.gcno *.gcda $(TESTDIR)/requests.log $(TESTDIR)/outputD.txt $(TESTDIR)/outputND.txt $(TESTDIR)/ads.txt $(PCH)
