#
# Makefile OpenWrt (MIPS)
# author: Alessandro Morelli	(amorelli@ihmc.us)
#

include Makefile.inc

%.o : ../%.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o : ../%.c
	$(CXX) $(CXXFLAGS) -c $<

%.o : ../net/%.cpp
	$(CXX) $(CXXFLAGS) -c $<

all: libutil.a

libutil.a : $(objects) $(netobjects)
	ar -rcv libutil.a $(objects) $(netobjects)

clean :
	rm -rf *.o *.a
