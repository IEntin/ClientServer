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

INCLUDES := -I. -Icommon -Iclient_src -Itests -Ilz4

CPPFLAGS := -g $(INCLUDE_PRECOMPILED) -std=c++2a $(WARNINGS) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) $(MACROS) -pthread

OBJDIR = obj
LIBDIR = lib

LZ4LIBA = $(LIBDIR)/lz4.a
LZ4SOURCES = lz4/lz4.cpp
LZ4OBJECTS = $(OBJDIR)/lz4.o
LZ4LINK = -L$(LIBDIR) -llz4

$(LZ4LIBA) : lz4/*.h
	$(CXX) -g -c $(WARNINGS) $(OPTIMIZATION) $(SANBLD) $(PROFBLD) -o $(LZ4OBJECTS) $(LZ4SOURCES)
	ar r $(LZ4LIBA) $(LZ4OBJECTS)

$(PCH) : *.h */*.h $(ALLH)
	@echo -n precompile start:
	@date
	$(CXX) -g -x c++-header $(CPPFLAGS) $(ALLH) -o $@
	@echo -n precompile end:
	@date

COMMONLIBA = $(LIBDIR)/common.a
COMMONSOURCES = $(wildcard common/*.cpp)
COMMONOBJECTS = $(patsubst common/%.cpp, $(OBJDIR)/%.o, $(COMMONSOURCES))

$(OBJDIR)/%.o: common/%.cpp
	$(CXX) -c -o $@ $< $(CPPFLAGS)

$(COMMONLIBA) : $(PCH) $(COMMONOBJECTS) common/*h
	@echo -n common start:
	@date
	ar r $(COMMONLIBA) $(COMMONOBJECTS)
	@echo -n common end:
	@date

SERVERLIBA = $(LIBDIR)/server.a
SERVERLIBSOURCES = $(filter-out ServerMain.cpp, $(wildcard *.cpp))
SERVERLIBOBJECTS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SERVERLIBSOURCES))

$(OBJDIR)/%.o: %.cpp
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

$(SERVERLIBA) : $(PCH) $(SERVERLIBOBJECTS)
	@echo -n serverlib start:
	@date
	ar r $(SERVERLIBA) $(SERVERLIBOBJECTS)
	@echo -n serverlib end:
	@date

SERVERSOURCES = ServerMain.cpp

server : $(PCH) $(COMMONLIBA) $(SERVERLIBA) $(LZ4LIBA) *.h
	@echo -n server start:
	@date
	$(CXX) $(CPPFLAGS) -Icommon $(SERVERSOURCES) $(SERVERLIBA) $(COMMONLIBA) $(LZ4LINK) -o $@
	@echo -n server end:
	@date

CLIENTLIBA = $(LIBDIR)/client.a
CLIENTLIBSOURCES = $(filter-out client_src/ClientMain.cpp, $(wildcard client_src/*.cpp))
CLIENTLIBOBJECTS = $(patsubst client_src/%.cpp, $(OBJDIR)/%.o, $(CLIENTLIBSOURCES))

$(OBJDIR)/%.o: client_src/%.cpp
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

$(CLIENTLIBA) : $(PCH) $(CLIENTLIBOBJECTS)
	@echo -n clientlib start:
	@date
	ar r $(CLIENTLIBA) $(CLIENTLIBOBJECTS)
	@echo -n clientlib end:
	@date

CLIENTSOURCES = client_src/ClientMain.cpp

$(CLIENTBINDIR)/client : $(PCH) $(COMMONLIBA) $(CLIENTLIBA) $(LZ4LIBA) client_src/*.h
	@echo -n client start:
	@date
	$(CXX) $(CPPFLAGS) -Icommon $(CLIENTSOURCES) $(CLIENTLIBA) $(COMMONLIBA) $(LZ4LINK) -o $@
	@echo -n client end:
	@date

TESTSOURCES = $(wildcard tests/*.cpp)

$(TESTDIR)/runtests : $(PCH) $(COMMONLIBA) $(CLIENTLIBA) $(SERVERLIBA) $(LZ4LIBA) client_src/*.h $(TESTDIR)/*.h
	@echo -n tests start:
	@date
	$(CXX) $(CPPFLAGS) $(INCLUDES) $(TESTSOURCES) -lgtest -lgtest_main $(CLIENTLIBA) $(COMMONLIBA) $(SERVERLIBA) $(LZ4LINK) -o $@
	@echo -n tests end:
	@date
	@cd $(TESTDIR); ln -sf ../$(CLIENTBINDIR)/requests.log .; ln -sf ../$(CLIENTBINDIR)/outputD.txt .; ln -sf ../$(CLIENTBINDIR)/outputND.txt .; ln -sf ../ads.txt .;./runtests

.PHONY: clean
clean:
	rm -f */*.d server $(CLIENTBINDIR)/client gmon.out */gmon.out $(TESTDIR)/runtests *.gcov *.gcno *.gcda $(TESTDIR)/requests.log $(TESTDIR)/outputD.txt $(TESTDIR)/outputND.txt $(TESTDIR)/ads.txt $(LIBDIR)/* $(PCH) $(COMMONLIBA) $(OBJDIR)/*  *~ */*~
