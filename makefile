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

# precompiled headers

ENABLEPCH = 1

ifeq ($(ENABLEPCH),1)
  ALLH = common/all.h

  ifeq ($(CXX),clang++)
    PCH = $(ALLH).pch
  else ifeq ($(CXX),g++)
    PCH = $(ALLH).gch
  endif

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

INCLUDES := -I. -Icommon -Iclient_src -I$(TESTDIR) -Ilz4

CPPFLAGS := -g $(INCLUDE_PRECOMPILED) -std=c++2a $(WARNINGS) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) $(MACROS)

OBJDIR = obj
LIBDIR = lib

vpath %.cpp common client_src $(TESTDIR) lz4

$(OBJDIR)/%.o : %.cpp $(PCH)
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

LZ4SOURCES = lz4/lz4.cpp
LZ4OBJECTS = $(OBJDIR)/lz4.o

$(PCH) : $(ALLH)
	$(CXX) -g -x c++-header $(CPPFLAGS) $(ALLH) -o $@

COMMONLIBA = $(LIBDIR)/common.a
COMMONSOURCES = $(wildcard common/*.cpp)
COMMONOBJECTS = $(patsubst common/%.cpp, $(OBJDIR)/%.o, $(COMMONSOURCES))

$(COMMONLIBA) : $(COMMONOBJECTS) $(LZ4OBJECTS)
	ar r $(COMMONLIBA) $(COMMONOBJECTS) $(LZ4OBJECTS)

SERVERLIBA = $(LIBDIR)/server.a
SERVERSOURCES = $(wildcard *.cpp)
SERVEROBJECTS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SERVERSOURCES))
SERVERLIBOBJECTS = $(filter-out $(OBJDIR)/ServerMain.o, $(SERVEROBJECTS))

$(SERVERLIBA) : $(SERVERLIBOBJECTS)
	ar r $(SERVERLIBA) $(SERVERLIBOBJECTS)

server : $(COMMONLIBA) $(SERVEROBJECTS)
	$(CXX) -o $@ $(SERVEROBJECTS) $(CPPFLAGS) $(COMMONLIBA) -pthread

CLIENTLIBA = $(LIBDIR)/client.a
CLIENTSOURCES = $(wildcard client_src/*.cpp)
CLIENTOBJECTS = $(patsubst client_src/%.cpp, $(OBJDIR)/%.o, $(CLIENTSOURCES))
CLIENTLIBOBJECTS = $(filter-out $(OBJDIR)/ClientMain.o, $(CLIENTOBJECTS))

$(CLIENTLIBA) : $(CLIENTLIBOBJECTS)
	ar r $(CLIENTLIBA) $(CLIENTLIBOBJECTS)

$(CLIENTBINDIR)/client : $(COMMONLIBA) $(CLIENTOBJECTS)
	$(CXX) -o $@ $(CLIENTOBJECTS) $(CPPFLAGS) $(COMMONLIBA) -pthread

TESTSOURCES = $(wildcard $(TESTDIR)/*.cpp)
TESTOBJECTS = $(patsubst $(TESTDIR)/%.cpp, $(OBJDIR)/%.o, $(TESTSOURCES))

$(TESTDIR)/runtests : $(TESTOBJECTS) $(COMMONLIBA) $(CLIENTLIBA) $(SERVERLIBA)
	$(CXX) -o $@ $(TESTOBJECTS) -lgtest -lgtest_main $(CPPFLAGS) $(CLIENTLIBA) $(COMMONLIBA) $(SERVERLIBA) -pthread
	@cd $(TESTDIR); ln -sf ../$(CLIENTBINDIR)/requests.log .; ln -sf ../$(CLIENTBINDIR)/outputD.txt .; ln -sf ../$(CLIENTBINDIR)/outputND.txt .; ln -sf ../ads.txt .;./runtests

.PHONY: clean
clean:
	rm -f */*.d server $(CLIENTBINDIR)/client gmon.out */gmon.out $(TESTDIR)/runtests *.gcov *.gcno *.gcda $(TESTDIR)/requests.log $(TESTDIR)/outputD.txt $(TESTDIR)/outputND.txt $(TESTDIR)/ads.txt $(LIBDIR)/* $(COMMONLIBA) $(OBJDIR)/*  *~ */*~

cleanall : clean
	rm -f common/*.gch common/*.pch
