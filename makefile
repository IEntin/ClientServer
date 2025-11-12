#
# Copyright (C) 2021 Ilya Entin
#

all: serverX clientX testbin runtests

ifeq ($(CMPLR),)
  CXX := clang++
else
  CXX := $(CMPLR)
endif

RM := rm -f

BUSINESSDIR := business
POLICYDIR := policy
CLIENTSRCDIR := client_src
COMMONDIR := common
TESTSRCDIR := test_src
# lz4 must be installed with
# 'sudo scripts/installLZ4.sh'
# google snappy must be installed
# 'sudo apt-get install libsnappy-dev'
# /usr/include/snappy.h
# libcrypto++-dev must be installed
# sudo apt-get install libcrypto++-dev libcrypto++-doc libcrypto++-utils
# libsodium-dev must be installed
# sudo apt install libsodium-dev

DATADIR := data

SERVERBIN := serverX
CLIENTBIN := clientX
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

# e.g. make -j$NUMBER_CORES OPTIMIZE=-O0
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

BOOST_INCLUDES := /usr/local/boost_1_89_0

INCLUDES := -I. -I$(COMMONDIR) -I$(BUSINESSDIR) -I$(POLICYDIR) -I$(BOOST_INCLUDES) -I$(CLIENTSRCDIR) \
-I$(TESTSRCDIR)

# to disable debug mode remove ' -D_DEBUG ' '-DCRYPTOTUPLE' possible replacement for '-DCRYPTOVARIANT' (not implemented yet)
CPPFLAGS := -g -D_DEBUG -DCRYPTOTUPLE $(INCLUDE_PRECOMPILED) $(GDWARF) -std=c++2b -fstack-protector-strong \
 -pipe -MMD -MP $(WARNINGS) \
$(OPTIMIZATION) $(SANBLD) $(PROFBLD)

BUILDDIR := build

vpath %.cpp $(BUSINESSDIR) $(POLICYDIR) $(COMMONDIR) $(CLIENTSRCDIR) $(TESTSRCDIR)

$(PCH) : $(ALLH)
	$(CXX) -g -x c++-header $(CPPFLAGS) -I$(BOOST_INCLUDES) $(ALLH) -o $@

-include $(BUILDDIR)/*.d

$(BUILDDIR)/%.o : %.cpp $(PCH)
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

BUSINESSSRC := $(wildcard $(BUSINESSDIR)/*.cpp)
BUSINESSOBJ := $(patsubst $(BUSINESSDIR)/%.cpp, $(BUILDDIR)/%.o, $(BUSINESSSRC))

POLICYSRC := $(wildcard $(POLICYDIR)/*.cpp)
POLICYOBJ := $(patsubst $(POLICYDIR)/%.cpp, $(BUILDDIR)/%.o, $(POLICYSRC))

COMMONSRC := $(wildcard $(COMMONDIR)/*.cpp)
COMMONOBJ := $(patsubst $(COMMONDIR)/%.cpp, $(BUILDDIR)/%.o, $(COMMONSRC)) \

SERVERSRC := $(wildcard *.cpp)
SERVEROBJ := $(patsubst %.cpp, $(BUILDDIR)/%.o, $(SERVERSRC))
SERVERFILTEREDOBJ := $(filter-out $(BUILDDIR)/ServerMain.o, $(SERVEROBJ))

serverX : $(COMMONOBJ) $(BUSINESSOBJ) $(POLICYOBJ) $(SERVEROBJ)
	$(CXX) -o $(SERVERBIN) $(SERVEROBJ) $(COMMONOBJ) $(BUSINESSOBJ) $(POLICYOBJ) \
$(CPPFLAGS) -pthread -lcryptopp -lsodium -llz4 -lsnappy -lzstd

CLIENTSRC := $(wildcard $(CLIENTSRCDIR)/*.cpp)
CLIENTOBJ := $(patsubst $(CLIENTSRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(CLIENTSRC))
CLIENTFILTEREDOBJ := $(filter-out $(BUILDDIR)/ClientMain.o, $(CLIENTOBJ))

$(CLIENTBIN) : $(COMMONOBJ) $(CLIENTOBJ)
	$(CXX) -o $@ $(CLIENTOBJ) $(COMMONOBJ)  $(CPPFLAGS) -pthread -lcryptopp -lsodium -llz4 -lsnappy -lzstd

TESTSRC := $(wildcard $(TESTSRCDIR)/*.cpp)
TESTOBJ := $(patsubst $(TESTSRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(TESTSRC))

$(TESTBIN) : $(COMMONOBJ) $(BUSINESSOBJ) $(POLICYOBJ) $(SERVERFILTEREDOBJ) $(CLIENTFILTEREDOBJ) $(TESTOBJ)
	$(CXX) -o $@ $(TESTOBJ) -lgtest $(COMMONOBJ) $(BUSINESSOBJ) $(POLICYOBJ) $(SERVERFILTEREDOBJ) \
$(CLIENTFILTEREDOBJ) $(CPPFLAGS) -pthread -lcryptopp -lsodium -llz4 -lsnappy -lzstd

RUNTESTSPSEUDOTARGET := runtests

$(RUNTESTSPSEUDOTARGET) : $(TESTBIN)
	./$(TESTBIN)
	@touch $(RUNTESTSPSEUDOTARGET)

.PHONY: clean cleanall

clean:
	$(RM) build/* $(SERVERBIN) $(CLIENTBIN) $(TESTBIN) \
gmon.out */gmon.out *.gcov *.gcno *.gcda *~ */*~ */*.d

cleanall : clean
	$(RM) *.gch */*.gch *.pch */*.pch \
debugTests.txt debugServer.txt debugClient.txt
