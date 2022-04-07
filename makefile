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

CURRENTDIR := ${CURDIR}
BUILDDIR = $(CURRENTDIR)/build

LZ4LIBA = $(BUILDDIR)/lz4.a
LZ4INCLUDES = -I$(CURRENTDIR)/lz4
LZ4SOURCES = $(CURRENTDIR)/lz4/lz4.cpp
LZ4OBJECTS = $(BUILDDIR)/lz4.o
LZ4LINK = -L$(BUILDDIR) -llz4

$(LZ4LIBA) : $(LZ4SOURCES) lz4/*.h
	$(CXX) -g  -c -std=c++2a $(LZ4INCLUDES) $(WARNINGS) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) -o $(LZ4OBJECTS) $(LZ4SOURCES)
	ar r $(LZ4LIBA) $(LZ4OBJECTS)

$(PCH) : *.h */*.h $(ALLH)
	@echo -n precompile start:
	@date
	$(CXX) -g -x c++-header $(CPPFLAGS) $(SERVERINCLUDES) $(CLIENTINCLUDES) $(TESTINCLUDES) $(ALLH) -o $@
	@echo -n precompile end:
	@date

COMMONLIBA = $(BUILDDIR)/common.a
COMMONINCLUDES = -Icommon
COMMONSOURCES = $(wildcard common/*.cpp)
COMMONLINK = -L$(BUILDDIR) -lcommon

$(COMMONLIBA) : $(PCH) $(COMMONSOURCES) common/*h
	@echo -n common start:
	@date
	$(CXX) $(CPPFLAGS) -c $(COMMONSOURCES) -pthread
	ar r $(COMMONLIBA) *.o
	rm -f *.o
	@echo -n common end:
	@date

SERVERINCLUDES=-I. -Icommon -Ififo -Itcp
SERVERSOURCES=$(wildcard *.cpp) $(wildcard fifo/*.cpp) $(wildcard tcp/*.cpp)

server : $(PCH) $(COMMONLIBA) $(SERVERSOURCES) $(LZ4LIBA) *.h fifo/*.h tcp/*.h
	@echo -n server start:
	@date
	$(CXX) $(CPPFLAGS) $(SERVERINCLUDES) $(SERVERSOURCES) $(COMMONLIBA) $(LZ4LINK) -o $@
	@echo -n server end:
	@date

CLIENTINCLUDES = -Iclient_src -Icommon
CLIENTSOURCES=$(wildcard client_src/*.cpp)

$(CLIENTBINDIR)/client : $(PCH) $(COMMONLIBA) $(CLIENTSOURCES) $(LZ4LIBA) client_src/*.h
	@echo -n client start:
	@date
	$(CXX) $(CPPFLAGS) $(CLIENTINCLUDES) $(CLIENTSOURCES) $(COMMONLIBA) $(LZ4LINK) -o $@
	@echo -n client end:
	@date

TESTINCLUDES = -I. -Itests -Iclient_src -Icommon -Ififo -Itcp
TESTSOURCES=$(wildcard $(TESTDIR)/*.cpp) $(wildcard fifo/*.cpp) $(wildcard tcp/*.cpp) $(filter-out client_src/Main.cpp, $(wildcard client_src/*.cpp)) $(filter-out Main.cpp, $(wildcard *.cpp))

$(TESTDIR)/runtests : $(PCH) $(COMMONLIBA) $(TESTSOURCES) $(SERVERSOURCES) $(CLIENTSOURCES) $(LZ4LIBA) fifo/*.h tcp/*.h client_src/*.h $(TESTDIR)/*.h
	@echo -n tests start:
	@date
	$(CXX) $(CPPFLAGS) $(TESTINCLUDES) $(TESTSOURCES) -lgtest -lgtest_main $(COMMONLIBA) $(LZ4LINK) -o $@
	@echo -n tests end:
	@date
	@cd $(TESTDIR); ln -sf ../$(CLIENTBINDIR)/requests.log .; ln -sf ../$(CLIENTBINDIR)/outputD.txt .; ln -sf ../$(CLIENTBINDIR)/outputND.txt .; ln -sf ../ads.txt .;./runtests

.PHONY: clean
clean:
	rm -f */*.d server $(CLIENTBINDIR)/client gmon.out */gmon.out $(TESTDIR)/runtests *.gcov *.gcno *.gcda $(TESTDIR)/requests.log $(TESTDIR)/outputD.txt $(TESTDIR)/outputND.txt $(TESTDIR)/ads.txt $(BUILDDIR)/* $(PCH) $(COMMONLIBA)
