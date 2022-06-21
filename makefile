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
# cores in
# /var/lib/systemd/coredump
# latest core:
# coredumpctl gdb -1
# sudo du -a | sort -n -r | head -n 5

#make PROFILE=[  | 1]
#make SANITIZE=[  | aul | thread ]
#make CMPLR=[ g++ | clang++ ]

all: server client testbin runtests

ifeq ($(CMPLR),)
  CXX := clang++
else
  CXX := $(CMPLR)
endif

RM := rm -f

CLIENTSRCDIR := client_src
COMMONDIR := common
LZ4DIR := lz4
TESTSRCDIR := test_src
DATADIR := data

SERVERBIN := server
CLIENTBIN := client
TESTBIN := testbin

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

WARNINGS := -Wall -Wextra -pedantic-errors

MACROS := -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) -DENABLEPCH=$(ENABLEPCH)

BOOST_INCLUDES := /usr/local/boost_1_79_0

INCLUDES := -I$(COMMONDIR) -I. -I$(BOOST_INCLUDES) -I$(CLIENTSRCDIR) -I$(TESTSRCDIR) -I$(LZ4DIR)

CPPFLAGS := -g $(INCLUDE_PRECOMPILED) -std=c++2a -MMD -MP $(WARNINGS) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) $(MACROS)

BUILDDIR := build

vpath %.cpp $(COMMONDIR) $(CLIENTSRCDIR) $(TESTSRCDIR) $(LZ4DIR)

$(BUILDDIR)/%.o : %.cpp $(PCH)
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

$(PCH) : $(ALLH)
	$(CXX) -g -x c++-header $(CPPFLAGS) -I$(BOOST_INCLUDES) $(ALLH) -o $@

-include $(BUILDDIR)/*.d

LZ4SRC := $(LZ4DIR)/lz4.cpp
COMMONSRC := $(wildcard $(COMMONDIR)/*.cpp)
COMMONOBJ := $(patsubst $(COMMONDIR)/%.cpp, $(BUILDDIR)/%.o, $(COMMONSRC)) $(patsubst $(LZ4DIR)/%.cpp, $(BUILDDIR)/%.o, $(LZ4SRC))

SERVERSRC := $(wildcard *.cpp)
SERVEROBJ := $(patsubst %.cpp, $(BUILDDIR)/%.o, $(SERVERSRC))
SERVERFILTEREDOBJ := $(filter-out $(BUILDDIR)/ServerMain.o, $(SERVEROBJ))

server : $(COMMONOBJ) $(SERVEROBJ)
	$(CXX) -o $(SERVERBIN) $(SERVEROBJ) $(COMMONOBJ) $(CPPFLAGS) -pthread

CLIENTSRC := $(wildcard $(CLIENTSRCDIR)/*.cpp)
CLIENTOBJ := $(patsubst $(CLIENTSRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(CLIENTSRC))
CLIENTFILTEREDOBJ := $(filter-out $(BUILDDIR)/ClientMain.o, $(CLIENTOBJ))

$(CLIENTBIN) : $(COMMONOBJ) $(CLIENTOBJ)
	$(CXX) -o $@ $(CLIENTOBJ) $(COMMONOBJ) $(CPPFLAGS) -pthread

TESTSRC = $(wildcard $(TESTSRCDIR)/*.cpp)
TESTOBJ = $(patsubst $(TESTSRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(TESTSRC))

$(TESTBIN) : client server $(TESTOBJ)
	$(CXX) -o $@ $(TESTOBJ) -lgtest $(CLIENTFILTEREDOBJ) $(COMMONOBJ) $(SERVERFILTEREDOBJ) $(CPPFLAGS) -pthread

RUNTESTSPSEUDOTARGET := runtests

$(RUNTESTSPSEUDOTARGET) : $(TESTBIN)
	./$(TESTBIN)
	@touch $(RUNTESTSPSEUDOTARGET)

.PHONY: clean cleanall

clean:
	$(RM) *.o */*.o *.d */*.d $(SERVERBIN) $(CLIENTBIN) $(TESTBIN) gmon.out */gmon.out *.gcov *.gcno *.gcda *~ */*~

cleanall : clean
	$(RM) *.gch */*.gch *.pch */*.pch
