# To build the image test use the target!!!!!!!

CPP = g++

NOMADS_HOME=../../../..

CPPFLAGS = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM  \
	   -I$(NOMADS_HOME)/nms/cpp \
	   -I$(NOMADS_HOME)/util/cpp \
           -I$(NOMADS_HOME)/util/cpp/net

LD_FLAGS = -lnsl -pthread -ldl

SOURCES = $(wildcard ../*.cpp)
OBJECTS = $(SOURCES:../%.cpp=%.o)

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

all: NetworkMessageSenderTest NetworkMessageReceiverTest networkMessageBigDataSenderTest networkMessageBigDataReceiverTest

networkMessageSenderTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../NetworkMessageSenderTest.cpp \
		../TestListener.h ../TestListener.cpp \
		$(NOMADS_HOME)/nms/cpp/linux/nms.a \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o NetworkMessageSenderTest

networkMessageReceiverTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../NetworkMessageReceiverTest.cpp \
		../TestListener.h ../TestListener.cpp \
		$(NOMADS_HOME)/nms/cpp/linux/nms.a \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o NetworkMessageReceiverTest

networkMessageBigDataSenderTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../networkMessageBigDataSenderTest.cpp \
		../TestListener.h ../networkMessageBigDataTestListener.cpp \
		$(NOMADS_HOME)/nms/cpp/linux/nms.a \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o NetworkMessageBigDataSenderTest

networkMessageBigDataReceiverTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../networkMessageBigDataReceiverTest.cpp \
		../TestListener.h ../networkMessageBigDataTestListener.cpp \
		$(NOMADS_HOME)/nms/cpp/linux/nms.a \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o NetworkMessageBigDataReceiverTest		

libutil.a :
	(cd $(NOMADS_HOME)/util/cpp/linux; make)

libutil_v4l2.a :
	(cd $(NOMADS_HOME)/util/cpp/linux; make -f Makefile-v4l2)

libjpegall.a :
	(cd $(NOMADS_HOME)/externals/jpeg-6b; make all-lib)

clean :
	(cd $(NOMADS_HOME)/util/cpp/linux; make clean)
	rm -rf *.o *.a NetworkMessageSenderTest NetworkMessageReceiverTest NetworkMessageBigDataSenderTest NetworkMessageBigDataReceiverTest
