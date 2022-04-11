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

RM = rm -f

CLIENTSRCDIR = client_src
CLIENTBINDIR = client_bin
COMMONDIR = common
LZ4DIR = lz4
TESTDIR = tests
DATADIR = data

SERVERBINARY = server
CLIENTBINARY = $(CLIENTBINDIR)/client
TESTBINARY = $(TESTDIR)/runtests

# precompiled headers

ENABLEPCH = 1

ifeq ($(ENABLEPCH),1)
  ALLH = $(COMMONDIR)/all.h

  ifeq ($(CXX),clang++)
    PCH = $(ALLH).pch
  else ifeq ($(CXX),g++)
    PCH = $(ALLH).gch
  endif

  INCLUDE_PRECOMPILED = -include $(ALLH)
endif

all: $(SERVERBINARY) $(CLIENTBINARY) $(TESTBINARY)

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

INCLUDES := -I. -I$(COMMONDIR) -I$(CLIENTSRCDIR) -I$(TESTDIR) -I$(LZ4DIR)

CPPFLAGS := -g $(INCLUDE_PRECOMPILED) -std=c++2a $(WARNINGS) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) $(MACROS)

OBJDIR = obj

vpath %.cpp $(COMMONDIR) $(CLIENTSRCDIR) $(TESTDIR) $(LZ4DIR)

$(OBJDIR)/%.o : %.cpp $(PCH)
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

$(PCH) : $(ALLH)
	$(CXX) -g -x c++-header $(CPPFLAGS) $(ALLH) -o $@

LZ4SOURCES = $(LZ4DIR)/lz4.cpp
COMMONSOURCES = $(wildcard $(COMMONDIR)/*.cpp)
COMMONOBJECTS = $(patsubst $(COMMONDIR)/%.cpp, $(OBJDIR)/%.o, $(COMMONSOURCES)) $(patsubst $(LZ4DIR)/%.cpp, $(OBJDIR)/%.o, $(LZ4SOURCES))

SERVERSOURCES = $(wildcard *.cpp)
SERVEROBJECTS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SERVERSOURCES))
SERVERLIBOBJECTS = $(filter-out $(OBJDIR)/ServerMain.o, $(SERVEROBJECTS))

$(SERVERBINARY) : $(COMMONOBJECTS) $(SERVEROBJECTS)
	$(CXX) -o $@ $(SERVEROBJECTS) $(CPPFLAGS) $(COMMONOBJECTS) -pthread

CLIENTSOURCES = $(wildcard $(CLIENTSRCDIR)/*.cpp)
CLIENTOBJECTS = $(patsubst $(CLIENTSRCDIR)/%.cpp, $(OBJDIR)/%.o, $(CLIENTSOURCES))
CLIENTLIBOBJECTS = $(filter-out $(OBJDIR)/ClientMain.o, $(CLIENTOBJECTS))

$(CLIENTBINARY) : $(COMMONOBJECTS) $(CLIENTOBJECTS)
	$(CXX) -o $@ $(CLIENTOBJECTS) $(CPPFLAGS) $(COMMONOBJECTS) -pthread

TESTSOURCES = $(wildcard $(TESTDIR)/*.cpp)
TESTOBJECTS = $(patsubst $(TESTDIR)/%.cpp, $(OBJDIR)/%.o, $(TESTSOURCES))

$(TESTBINARY) : $(TESTOBJECTS) $(COMMONOBJECTS) $(CLIENTLIBOBJECTS) $(SERVERLIBOBJECTS)
	$(CXX) -o $@ $(TESTOBJECTS) -lgtest $(CPPFLAGS) $(CLIENTLIBOBJECTS) $(COMMONOBJECTS) $(SERVERLIBOBJECTS) -pthread
	@(cd $(TESTDIR); ln -sf ../$(DATADIR) .; ./runtests $(DATADIR))

.PHONY: clean cleanall
clean:
	$(RM) */*.d $(SERVERBINARY) $(CLIENTBINARY) gmon.out */gmon.out $(TESTBINARY) *.gcov *.gcno *.gcda $(OBJDIR)/* $(TESTDIR)/data *~ */*~

cleanall : clean
	$(RM) $(COMMONDIR)/*.gch $(COMMONDIR)/*.pch
