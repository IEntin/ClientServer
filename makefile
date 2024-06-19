#
# Copyright (C) 2021 Ilya Entin
#

# sudo sysctl vm.mmap_rnd_bits=28

#LD_PRELOAD=$LD_PRELOAD:$HOME/jemalloc/lib/libjemalloc.so ./serverX
#LD_PRELOAD=$LD_PRELOAD:$HOME/jemalloc/lib/libjemalloc.so ./clientX
#use clang++ (CMPLR=clang++) for sanitized build if use jemalloc
#gprof -b ./serverX gmon.out > profile.txt
#	valgrind
# to use valgrind rebuild with -gdwarf-4
# make -j$NUMBER_CORES GDWARF=-gdwarf-4
# valgrind --tool=massif ./serverX
# ms_print massif.out.*
# valgrind --leak-check=yes ./serverX
# valgrind --tool=callgrind ./serverX
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

all: serverX clientX testbin runtests

ifeq ($(CMPLR),)
  CXX := clang++
else
  CXX := $(CMPLR)
endif

# workaround for LLVM17 bug affecting sanitized builds
# https://github.com/google/sanitizers/issues/1716
# temporary.
# alternatively run this command elsewhere after every
# reboot
# this workaround is not necessary if clang19 is installed
# and compiler is clang++
# but is still needed for g++ compiler.

# start with original configuration
ORIGINALINT=32
$(info ORIGINALINT=$(ORIGINALINT))
ORIGINAL := $(shell sudo sysctl vm.mmap_rnd_bits=$(ORIGINALINT))
$(info ORIGINAL=$(ORIGINAL))

ifeq ($(CMPLR),g++)
ifneq ($(SANITIZE),)
  NEW=$(shell sudo sysctl vm.mmap_rnd_bits=28)
  $(info NEW=$(NEW))
  NEWINT=$(filter-out vm.mmap_rnd_bits = ,$(NEW))
  $(info NEWINT=$(NEWINT))
endif
endif

RM := rm -f

BUSINESSDIR := business
CLIENTSRCDIR := client_src
COMMONDIR := common
TESTSRCDIR := test_src
LIBLZ4 := /snap/lz4/4/usr/local/lib/liblz4.a
CRYPTOPPRELEASE := cryptopp890.zip
CRYPTOLIBDIR:=/usr/local/lib/cryptopp
CRYPTOLIB := $(CRYPTOLIBDIR)/libcryptopp.a

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

BOOST_INCLUDES := /usr/local/boost_1_85_0

INCLUDES := -I. -I$(COMMONDIR) -I$(BUSINESSDIR) -I$(BOOST_INCLUDES) -I$(CLIENTSRCDIR) \
-I$(TESTSRCDIR)

CPPFLAGS := -g $(INCLUDE_PRECOMPILED) $(GDWARF) -std=c++2b -pipe -MMD -MP $(WARNINGS) \
$(OPTIMIZATION) $(SANBLD) $(PROFBLD)

BUILDDIR := build

vpath %.cpp $(BUSINESSDIR) $(COMMONDIR) $(CLIENTSRCDIR) $(TESTSRCDIR)

$(PCH) : $(ALLH) $(CRYPTOLIB)
	$(CXX) -g -x c++-header $(CPPFLAGS) -I$(BOOST_INCLUDES) $(ALLH) -o $@

-include $(BUILDDIR)/*.d

$(BUILDDIR)/%.o : %.cpp $(PCH) $(CRYPTOLIB)
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

BUSINESSSRC := $(wildcard $(BUSINESSDIR)/*.cpp)
BUSINESSOBJ := $(patsubst $(BUSINESSDIR)/%.cpp, $(BUILDDIR)/%.o, $(BUSINESSSRC))

COMMONSRC := $(wildcard $(COMMONDIR)/*.cpp)
COMMONOBJ := $(patsubst $(COMMONDIR)/%.cpp, $(BUILDDIR)/%.o, $(COMMONSRC)) \

SERVERSRC := $(wildcard *.cpp)
SERVEROBJ := $(patsubst %.cpp, $(BUILDDIR)/%.o, $(SERVERSRC))
SERVERFILTEREDOBJ := $(filter-out $(BUILDDIR)/ServerMain.o, $(SERVEROBJ))

serverX : $(COMMONOBJ) $(BUSINESSOBJ) $(SERVEROBJ) $(CRYPTOLIB)
	$(CXX) -o $(SERVERBIN) $(SERVEROBJ) $(COMMONOBJ) $(BUSINESSOBJ) \
$(CPPFLAGS) -pthread $(CRYPTOLIB) $(LIBLZ4)

CLIENTSRC := $(wildcard $(CLIENTSRCDIR)/*.cpp)
CLIENTOBJ := $(patsubst $(CLIENTSRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(CLIENTSRC))
CLIENTFILTEREDOBJ := $(filter-out $(BUILDDIR)/ClientMain.o, $(CLIENTOBJ))

$(CLIENTBIN) : $(COMMONOBJ) $(CLIENTOBJ) $(CRYPTOLIB)
	$(CXX) -o $@ $(CLIENTOBJ) $(COMMONOBJ) $(CPPFLAGS) -pthread $(CRYPTOLIB) $(LIBLZ4)

TESTSRC := $(wildcard $(TESTSRCDIR)/*.cpp)
TESTOBJ := $(patsubst $(TESTSRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(TESTSRC))

$(TESTBIN) : $(COMMONOBJ) $(BUSINESSOBJ) $(SERVERFILTEREDOBJ) $(CLIENTFILTEREDOBJ) $(TESTOBJ) $(CRYPTOLIB)
	$(CXX) -o $@ $(TESTOBJ) -lgtest $(COMMONOBJ) $(BUSINESSOBJ) $(SERVERFILTEREDOBJ) \
$(CLIENTFILTEREDOBJ) $(CPPFLAGS) -pthread $(CRYPTOLIB) $(LIBLZ4)

RUNTESTSPSEUDOTARGET := runtests

$(RUNTESTSPSEUDOTARGET) : $(TESTBIN)
	./$(TESTBIN)
	@touch $(RUNTESTSPSEUDOTARGET)

$(CRYPTOLIB) : scripts/makeCrypto.sh
	sudo scripts/makeCrypto.sh $(CRYPTOPPRELEASE)

.PHONY: clean cleanall

clean:
	$(RM) build/* $(SERVERBIN) $(CLIENTBIN) $(TESTBIN) \
gmon.out */gmon.out *.gcov *.gcno *.gcda *~ */*~

cleanall : clean
	$(RM) *.gch */*.gch *.pch */*.pch .*.sec
