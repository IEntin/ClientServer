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

SERVERBIN = server
CLIENTBIN = $(CLIENTBINDIR)/client
TESTBIN = $(TESTDIR)/runtests

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

all: $(SERVERBIN) $(CLIENTBIN) $(TESTBIN)

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

LZ4SRC = $(LZ4DIR)/lz4.cpp
COMMONSRC = $(wildcard $(COMMONDIR)/*.cpp)
COMMONHDR = $(COMMONDIR)/*.h $(LZ4DIR)/*.h
COMMONOBJ = $(patsubst $(COMMONDIR)/%.cpp, $(OBJDIR)/%.o, $(COMMONSRC)) $(patsubst $(LZ4DIR)/%.cpp, $(OBJDIR)/%.o, $(LZ4SRC))

$(COMMONOBJ) : $(COMMONSRC) $(COMMONHDR)

SERVERSRC = $(wildcard *.cpp)
SERVERHDR = *.h
SERVEROBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SERVERSRC))
SERVERFILTEREDOBJ = $(filter-out $(OBJDIR)/ServerMain.o, $(SERVEROBJ))

$(SERVEROBJ) : $(SERVERSRC) $(SERVERHDR)

$(SERVERBIN) : $(COMMONOBJ) $(SERVEROBJ)
	$(CXX) -o $@ $(SERVEROBJ) $(CPPFLAGS) $(COMMONOBJ) -pthread

CLIENTSRC = $(wildcard $(CLIENTSRCDIR)/*.cpp)
CLIENTHDR = $(CLIENTSRCDIR)/*.h
CLIENTOBJ = $(patsubst $(CLIENTSRCDIR)/%.cpp, $(OBJDIR)/%.o, $(CLIENTSRC))
CLIENTFILTEREDOBJ = $(filter-out $(OBJDIR)/ClientMain.o, $(CLIENTOBJ))

$(CLIENTOBJ) : $(CLIENTSRC) $(CLIENTHDR)

$(CLIENTBIN) : $(COMMONOBJ) $(CLIENTOBJ)
	$(CXX) -o $@ $(CLIENTOBJ) $(CPPFLAGS) $(COMMONOBJ) -pthread
	@(cd $(CLIENTBINDIR); ln -sf ../$(DATADIR))

TESTSRC = $(wildcard $(TESTDIR)/*.cpp)
TESTHDR = $(TESTDIR)/*.h
TESTOBJ = $(patsubst $(TESTDIR)/%.cpp, $(OBJDIR)/%.o, $(TESTSRC))

$(TESTOBJ) : $(TESTSRC) $(TESTHDR)

$(TESTBIN) : $(TESTOBJ) $(COMMONOBJ) $(CLIENTFILTEREDOBJ) $(SERVERFILTEREDOBJ)
	$(CXX) -o $@ $(TESTOBJ) -lgtest $(CPPFLAGS) $(CLIENTFILTEREDOBJ) $(COMMONOBJ) $(SERVERFILTEREDOBJ) -pthread
	@(cd $(TESTDIR); ln -sf ../$(DATADIR) .; ./runtests $(DATADIR))

.PHONY: clean cleanall
clean:
	$(RM) */*.d $(SERVERBIN) $(CLIENTBIN) $(CLIENTBINDIR)/data gmon.out */gmon.out $(TESTBIN) *.gcov *.gcno *.gcda $(OBJDIR)/* $(TESTDIR)/data *~ */*~

cleanall : clean
	$(RM) $(COMMONDIR)/*.gch $(COMMONDIR)/*.pch
