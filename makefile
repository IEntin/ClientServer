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
#~/ClientServer/test_bin$ ./runtests --gtest_repeat=10 --gtest_break_on_failure

#make PROFILE=[  | 1]
#make SANITIZE=[  | aul | thread ]
#make CMPLR=[ g++ | clang++ ]

all: server client buildtests runtests

ifeq ($(CMPLR),)
  CXX := g++
else
  CXX := $(CMPLR)
endif

RM := rm -f

CLIENTSRCDIR := client_src
CLIENTBINDIR := client_bin
CLIENTPSEUDOTARGET := client
COMMONDIR := common
LZ4DIR := lz4
TESTDIR := test_src
DATADIR := data

SERVERBIN := server
CLIENTBIN := $(CLIENTBINDIR)/client
TESTBINDIR := test_bin
TESTBIN := $(TESTBINDIR)/runtests

# precompiled headers

ENABLEPCH =
ifeq ($(ENABLEPCH),)
  PCHENABLED = 1
else ifeq ($(ENABLEPCH),0)
  PCHENABLED = 0
else ifeq ($(ENABLEPCH),1)
  PCHENABLED = 1
endif

ifeq ($(PCHENABLED),1)
  ALLH := $(COMMONDIR)/all.h

  ifeq ($(CXX),clang++)
    PCH := $(ALLH).pch
  else ifeq ($(CXX),g++)
    PCH := $(ALLH).gch
  endif

  INCLUDE_PRECOMPILED := -include $(ALLH)
endif

OPTIMIZE =
ifeq ($(OPTIMIZE),)
  OPTIMIZATION := -O3
else
  OPTIMIZATION := $(OPTIMIZE)
endif

ifeq ($(SANITIZE), aul)
  SANBLD := -fsanitize=address,undefined,leak
else ifeq ($(SANITIZE), thread)
  SANBLD := -fsanitize=thread
endif

ifeq ($(PROFILE), 1)
  PROFBLD := -pg
endif

WARNINGS := -Wall -pedantic-errors

MACROS := -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) -DENABLEPCH=$(ENABLEPCH)

INCLUDES := -I. -I$(COMMONDIR) -I$(CLIENTSRCDIR) -I$(TESTDIR) -I$(LZ4DIR)

CPPFLAGS := -g $(INCLUDE_PRECOMPILED) -std=c++2a -MMD -MP $(WARNINGS) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) $(MACROS)

OBJDIR := obj

vpath %.cpp $(COMMONDIR) $(CLIENTSRCDIR) $(TESTDIR) $(LZ4DIR)

$(OBJDIR)/%.o : %.cpp $(PCH)
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

$(PCH) : $(ALLH)
	$(CXX) -g -x c++-header $(CPPFLAGS) $(ALLH) -o $@

-include $(OBJDIR)/*.d

LZ4SRC := $(LZ4DIR)/lz4.cpp
COMMONSRC := $(wildcard $(COMMONDIR)/*.cpp)
COMMONOBJ := $(patsubst $(COMMONDIR)/%.cpp, $(OBJDIR)/%.o, $(COMMONSRC)) $(patsubst $(LZ4DIR)/%.cpp, $(OBJDIR)/%.o, $(LZ4SRC))

SERVERSRC := $(wildcard *.cpp)
SERVEROBJ := $(patsubst %.cpp, $(OBJDIR)/%.o, $(SERVERSRC))
SERVERFILTEREDOBJ := $(filter-out $(OBJDIR)/ServerMain.o, $(SERVEROBJ))

server : $(COMMONOBJ) $(SERVEROBJ)
	$(CXX) -o $(SERVERBIN) $(SERVEROBJ) $(COMMONOBJ) $(CPPFLAGS) -pthread

CLIENTSRC := $(wildcard $(CLIENTSRCDIR)/*.cpp)
CLIENTOBJ := $(patsubst $(CLIENTSRCDIR)/%.cpp, $(OBJDIR)/%.o, $(CLIENTSRC))
CLIENTFILTEREDOBJ := $(filter-out $(OBJDIR)/ClientMain.o, $(CLIENTOBJ))

$(CLIENTPSEUDOTARGET) : $(CLIENTBIN)
	(cd $(CLIENTBINDIR); ln -sf ../$(DATADIR))
	touch $(CLIENTPSEUDOTARGET)

$(CLIENTBIN) : $(COMMONOBJ) $(CLIENTOBJ)
	$(CXX) -o $@ $(CLIENTOBJ) $(COMMONOBJ) $(CPPFLAGS) -pthread

TESTSRC = $(wildcard $(TESTDIR)/*.cpp)
TESTOBJ = $(patsubst $(TESTDIR)/%.cpp, $(OBJDIR)/%.o, $(TESTSRC))
BUILDTESTPSEUDOTARGET := buildtests

$(BUILDTESTPSEUDOTARGET) : $(TESTBIN)
	(cd $(TESTBINDIR); ln -sf ../$(DATADIR))
	touch $(BUILDTESTPSEUDOTARGET)

$(TESTBIN) : $(TESTOBJ) $(COMMONOBJ) $(CLIENTFILTEREDOBJ) $(SERVERFILTEREDOBJ)
	$(CXX) -o $(TESTBIN) $(TESTOBJ) -lgtest $(CLIENTFILTEREDOBJ) $(COMMONOBJ) $(SERVERFILTEREDOBJ) $(CPPFLAGS) -pthread

RUNTESTSPSEUDOTARGET := runtests

$(RUNTESTSPSEUDOTARGET) : $(BUILDTESTPSEUDOTARGET)
	@(cd $(TESTBINDIR); ./runtests)
	touch $(RUNTESTSPSEUDOTARGET)

.PHONY: clean cleanall

clean:
	$(RM) *.o */*.o *.d */*.d $(SERVERBIN) $(CLIENTBIN) $(CLIENTBINDIR)/data gmon.out */gmon.out $(TESTBINDIR)/* *.gcov *.gcno *.gcda *~ */*~

cleanall : clean
	$(RM) *.gch */*.gch *.pch */*.pch
