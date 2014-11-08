include Makefile.inc

NOMADS_HOME = ../../../../..
CPPSOURCES = $(wildcard ../*.cpp)
CPPOBJECTS = $(CPPSOURCES:../%.cpp=%.o)

LD_FLAGS = -ldl -pthread -lnsl

all: test SQLiteBenchmark

%.o : ../%.cpp
	$(cpp) $(cppflags) -c $<

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/linux/ libutil.a

liblcppdc.a : libutil.a
	make -C $(NOMADS_HOME)/misc/cpp/lcppdc/linux/ liblcppdc.a

test:	$(objects) liblcppdc.a
		$(cpp) $(cppflags) \
		../Test.cpp \
		$(NOMADS_HOME)/misc/cpp/lcppdc/linux/liblcppdc.a \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		$(SQLITE_HOME)/linux/libsqlite3.a \
        $(LD_FLAGS) \
		-o Test

SQLiteBenchmark: $(objects) liblcppdc.a
		$(cpp) $(cppflags) \
		../SQLiteBenchmark.cpp \
		$(NOMADS_HOME)/misc/cpp/lcppdc/linux/liblcppdc.a \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		$(SQLITE_HOME)/linux/libsqlite3.a \
        $(LD_FLAGS) \
		-o SQLiteBenchmark

clean :
	rm -rf *.o *.a *.gch ../*.gch Test
	make -C $(NOMADS_HOME)/misc/cpp/lcppdc/linux/ liblcppdc.a clean
