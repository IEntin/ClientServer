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

CMPLR=
ifeq ($(CMPLR),)
	CXX=clang++
else
	CXX=$(CMPLR)
endif

CLIENTBINDIR = client_bin

TESTDIR = tests

PRECOMPILED_HEADERS = true

ifeq ($(PRECOMPILED_HEADERS),true) 
	ALLH = common/all.h
else
	ALLH = common/none.h
endif

ifeq ($(CXX),clang++)
	PCH = $(ALLH).pch
else
	PCH = $(ALLH).gch
endif

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

SERVERINCLUDES:=-I. -Icommon -Ififo -Itcp
SERVERSOURCES=$(wildcard *.cpp) $(wildcard common/*.cpp) $(wildcard fifo/*.cpp) $(wildcard tcp/*.cpp)

$(PCH) : $(ALLH)
	@echo -n precompile start:
	@date
	$(CXX) -g -x c++-header -std=c++2a $(WARNINGS) $(SERVERINCLUDES) $(CLIENTINCLUDES) $(TESTINCLUDES) $(ALLH) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) -pthread -o $@
	@echo -n precompile end:
	@date


server : $(SERVERSOURCES) $(PCH) *.h common/*.h fifo/*.h tcp/*.h
	@echo -n server start:
	@date
	$(CXX) -g -include $(ALLH) -std=c++2a $(WARNINGS) $(SERVERINCLUDES) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) $(SERVERSOURCES) -pthread -o $@
	@echo -n server end:
	@date


CLIENTINCLUDES = -Iclient_src -Icommon 
CLIENTSOURCES=$(wildcard client_src/*.cpp) $(wildcard common/*.cpp)

$(CLIENTBINDIR)/client : $(CLIENTSOURCES) $(PCH) common/*.h client_src/*.h
	@echo -n client start:
	@date
	$(CXX) -g -include $(ALLH) -std=c++2a $(WARNINGS) $(CLIENTINCLUDES) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) $(CLIENTSOURCES) -pthread -o $@
	@echo -n client end:
	@date

TESTINCLUDES = -I. -Itests -Iclient_src -Icommon -Ififo -Itcp
TESTSOURCES=$(wildcard $(TESTDIR)/*.cpp) $(wildcard common/*.cpp) $(wildcard fifo/*.cpp) $(wildcard tcp/*.cpp) $(filter-out client_src/Main.cpp, $(wildcard client_src/*.cpp)) $(filter-out Main.cpp, $(wildcard *.cpp))

$(TESTDIR)/runtests : $(TESTSOURCES) server $(CLIENTBINDIR)/client $(PCH) $(TESTDIR)/*.h
	@echo -n tests start:
	@date
	$(CXX) -g -include $(ALLH) -std=c++2a $(WARNINGS) $(TESTINCLUDES) $(OPTIMIZATION) $(SANBLD) -DSANITIZE=$(SANITIZE) $(TESTSOURCES) -lgtest -lgtest_main -pthread -o $@
	@echo -n tests end:
	@date
	cd $(TESTDIR); ln -sf ../$(CLIENTBINDIR)/requests.log .; ln -sf ../$(CLIENTBINDIR)/outputD.txt .; ln -sf ../$(CLIENTBINDIR)/outputND.txt .; ln -sf ../ads.txt .; ./runtests

.PHONY: clean
clean:
	rm -f *.d server $(CLIENTBINDIR)/client $(CLIENTBINDIR)/*.d $(CLIENTBINDIR)/gmon.out $(TESTDIR)/runtests $(TESTDIR)/*.d
	rm -f gmon.out *.gcov *.gcno *.gcda $(TESTDIR)/requests.log $(TESTDIR)/outputD.txt $(TESTDIR)/outputND.txt $(TESTDIR)/ads.txt
	rm -f common/*.pch common/*.gch
