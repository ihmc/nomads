# Makefile.inc
#
# User: Giacomo Benincasa      (gbenincasa@ihmc.us)
# Time: Aug 12, 2009 1:30:56 PM
#

cpp = g++

cppflags = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM -fPIC -I.. -I../.. -I../../../externals/include 

sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)

netsources = $(wildcard ../net/*.cpp)
netobjects = $(netsources:../net/%.cpp=%.o)

securitysources = $(wildcard ../security/*.cpp)
securityobjects = $(securitysources:../security/%.cpp=%.o)

graphsources = $(wildcard ../graph/*.cpp)
graphobjects = $(graphsources:../graph/%.cpp=%.o)

commsources = $(wildcard ../comm/*.cpp)
commobjects = $(commsources:../comm/%.cpp=%.o)

mediasources = $(wildcard ../media/*.cpp)
mediaobjects = $(mediasources:../media/%.cpp=%.o)
