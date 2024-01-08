#
# Copyright (C) 2021 Ilya Entin
#

#LD_PRELOAD=$LD_PRELOAD:$HOME/jemalloc/lib/libjemalloc.so ./server
#LD_PRELOAD=$LD_PRELOAD:$HOME/jemalloc/lib/libjemalloc.so ./client
#use clang++ (CMPLR=clang++) for sanitized build if use jemalloc
#gprof -b ./server gmon.out > profile.txt
#	valgrind
# to use valgrind rebuild with -gdwarf-4
# make -j4 GDWARF=-gdwarf-4
# valgrind --tool=massif ./server
# ms_print massif.out.*
# valgrind --leak-check=yes ./server
# valgrind --tool=callgrind ./server
# kcachegrind callgrind.out.*

#ps huH p <pid> | wc -l
# cores in
# /var/lib/systemd/coredump
# latest core:
# coredumpctl gdb
# sudo du -a | sort -n -r | head -n 5
# find ./ -type f -exec sed -i 's/old_string/new_string/g' {} \;
#make PROFILE=[  | 1]
#make SANITIZE=[  | aul | thread ]
#make CMPLR=[ g++ | clang++ ]

all: server client testbin runtests

ifeq ($(CMPLR),)
  CXX := clang++
endif

RM := rm -f

BUSINESSDIR := business
CLIENTSRCDIR := client_src
COMMONDIR := common
TESTSRCDIR := test_src
LIBLZ4 := /snap/lz4/4/usr/local/lib/liblz4.a
CRYPTOLIBDIR:=/usr/local/lib/cryptopp
CRYPTOLIB := -lcryptoppgcc
DATADIR := data

SERVERBIN := server
CLIENTBIN := client
TESTBIN := testbin

# precompiled headers

PCHENABLED = 1
ALLH := $(COMMONDIR)/all.h
PCH := $(ALLH).pch
ifeq ($(CXX),g++)
  PCH := $(ALLH).gch
endif

INCLUDE_PRECOMPILED := -include $(ALLH)

ifeq ($(ENABLEPCH),0)
  PCHENABLED = 0
  ALLH :=
  PCH :=
  INCLUDE_PRECOMPILED :=
endif

# e.g. make -j4 OPTIMIZE=-O0
# for no optimization, useful for debugging
ifeq ($(OPTIMIZE),)
  OPTIMIZATION := -O3
endif

ifeq ($(SANITIZE), aul)
  SANBLD := -fsanitize=address,undefined,leak
else ifeq ($(SANITIZE), thread)
  SANBLD := -fsanitize=thread
endif

ifeq ($(PROFILE), 1)
  PROFBLD := -pg
  GDWARF := -gdwarf-4
endif

WARNINGS := -Wall -Wextra -pedantic-errors

MACROS := -DSANITIZE=$(SANITIZE) -DPROFILE=$(PROFILE) -DOPTIMIZE=$(OPTIMIZE) \
-DENABLEPCH=$(ENABLEPCH) -DGDWARF=$(GDWARF)

BOOST_INCLUDES := /usr/local/boost_1_84_0

INCLUDES := -I. -I$(COMMONDIR) -I$(BUSINESSDIR) -I$(BOOST_INCLUDES) -I$(CLIENTSRCDIR) \
-I$(TESTSRCDIR)

CPPFLAGS := -g $(INCLUDE_PRECOMPILED) $(GDWARF) -std=c++2a -pipe -MMD -MP $(WARNINGS) \
$(OPTIMIZATION) $(SANBLD) $(PROFBLD) $(MACROS)

BUILDDIR := build

vpath %.cpp $(BUSINESSDIR) $(COMMONDIR) $(CLIENTSRCDIR) $(TESTSRCDIR)

$(PCH) : $(ALLH)
	$(CXX) -g -x c++-header $(CPPFLAGS) -I$(BOOST_INCLUDES) $(ALLH) -o $@

-include $(BUILDDIR)/*.d

$(BUILDDIR)/%.o : %.cpp $(PCH)
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

BUSINESSSRC := $(wildcard $(BUSINESSDIR)/*.cpp)
BUSINESSOBJ := $(patsubst $(BUSINESSDIR)/%.cpp, $(BUILDDIR)/%.o, $(BUSINESSSRC))

COMMONSRC := $(wildcard $(COMMONDIR)/*.cpp)
COMMONOBJ := $(patsubst $(COMMONDIR)/%.cpp, $(BUILDDIR)/%.o, $(COMMONSRC)) \

SERVERSRC := $(wildcard *.cpp)
SERVEROBJ := $(patsubst %.cpp, $(BUILDDIR)/%.o, $(SERVERSRC))
SERVERFILTEREDOBJ := $(filter-out $(BUILDDIR)/ServerMain.o, $(SERVEROBJ))

server : $(COMMONOBJ) $(BUSINESSOBJ) $(SERVEROBJ)
	$(CXX) -o $(SERVERBIN) $(SERVEROBJ) $(COMMONOBJ) $(BUSINESSOBJ) \
$(CPPFLAGS) -pthread -L$(CRYPTOLIBDIR) $(CRYPTOLIB) $(LIBLZ4)

CLIENTSRC := $(wildcard $(CLIENTSRCDIR)/*.cpp)
CLIENTOBJ := $(patsubst $(CLIENTSRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(CLIENTSRC))
CLIENTFILTEREDOBJ := $(filter-out $(BUILDDIR)/ClientMain.o, $(CLIENTOBJ))

$(CLIENTBIN) : $(COMMONOBJ) $(CLIENTOBJ)
	$(CXX) -o $@ $(CLIENTOBJ) $(COMMONOBJ) $(CPPFLAGS) -pthread -L$(CRYPTOLIBDIR) $(CRYPTOLIB) $(LIBLZ4)

TESTSRC := $(wildcard $(TESTSRCDIR)/*.cpp)
TESTOBJ := $(patsubst $(TESTSRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(TESTSRC))

$(TESTBIN) : $(COMMONOBJ) $(BUSINESSOBJ) $(SERVERFILTEREDOBJ) $(CLIENTFILTEREDOBJ) $(TESTOBJ)
	$(CXX) -o $@ $(TESTOBJ) -lgtest $(COMMONOBJ) $(BUSINESSOBJ) $(SERVERFILTEREDOBJ) \
$(CLIENTFILTEREDOBJ) $(CPPFLAGS) -pthread -L$(CRYPTOLIBDIR) $(CRYPTOLIB) $(LIBLZ4)

RUNTESTSPSEUDOTARGET := runtests

$(RUNTESTSPSEUDOTARGET) : $(TESTBIN)
	./$(TESTBIN)
	@touch $(RUNTESTSPSEUDOTARGET)

.PHONY: clean cleanall

clean:
	$(RM) build/* $(SERVERBIN) $(CLIENTBIN) $(TESTBIN) \
gmon.out */gmon.out *.gcov *.gcno *.gcda *~ */*~

cleanall : clean
	$(RM) *.gch */*.gch *.pch */*.pch .*.sec
