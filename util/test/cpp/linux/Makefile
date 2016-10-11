# To build the image test use the target!!!!!!!

CPP = g++

NOMADS_HOME=../../../..
ACI_HOME=$(NOMADS_HOME)/aci

CPPFLAGS = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM  \
		   -I$(NOMADS_HOME)/util/cpp \
                   -I$(NOMADS_HOME)/util/cpp/net \
                   -I$(NOMADS_HOME)/util/cpp/graph \
                   -I$(NOMADS_HOME)/util/cpp/comm \
                   -I$(NOMADS_HOME)/externals/jpeg-6b \
                   -I$(NOMADS_HOME)/../../PIM/CMVision2.1/cmvision \
                   -I$(NOMADS_HOME)/../../PIM/CMVision2.1/shared 

LD_FLAGS = -lnsl -pthread -ldl

SOURCES = $(wildcard ../*.cpp)
OBJECTS = $(SOURCES:../%.cpp=%.o)

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

all: multicast_echo wildNetIFs netIFs multicast_receiver multicast_sender \
	networkMessageSenderTest networkMessageReceiverTest networkMessageBigDataSenderTest \
	networkMessageBigDataReceiverTest imageFromIpCamera TestTypes ProxyDatagramSocketTest \
	FIFOBufferTest netmsgsvc MCastRelayer

RangeDLListTestTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../RangeDLListTest.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o RangeDLListTestTest

SetUniquePtrLListTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../SetUniquePtrLListTest.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o SetUniquePtrLListTest

loggerListener: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../LoggerListener.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o llistener

three_string_hashtable: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../three_string_hashtable.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o three_string_hashtable

multicast_echo: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../multicast_echo.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o multicast_echo

netIFs: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../netIFs.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o netIFs

wildNetIFs: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../wildNetIFs.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o wildNetIFs

multicast_receiver: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../multicast_receiver.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o multicast_receiver

multicast_sender: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../multicast_sender.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o multicast_sender

networkMessageTests: NetworkMessageSenderTest NetworkMessageReceiverTest networkMessageBigDataSenderTest networkMessageBigDataReceiverTest

networkMessageSenderTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../NetworkMessageSenderTest.cpp \
		../TestListener.h ../TestListener.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o NetworkMessageSenderTest

networkMessageReceiverTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../NetworkMessageReceiverTest.cpp \
		../TestListener.h ../TestListener.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o NetworkMessageReceiverTest

networkMessageBigDataSenderTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../networkMessageBigDataSenderTest.cpp \
		../TestListener.h ../networkMessageBigDataTestListener.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o NetworkMessageBigDataSenderTest

networkMessageBigDataReceiverTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../networkMessageBigDataReceiverTest.cpp \
		../TestListener.h ../networkMessageBigDataTestListener.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o NetworkMessageBigDataReceiverTest		

imageFromUsbCamera: libutil_v4l2.a libjpegall.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../imageFromUsbCamera.cpp \
	$(NOMADS_HOME)/externals/jpeg-6b/libjpegall.a \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o imageFromUsbCamera

imageFromIpCamera: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../imageFromIpCamera.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o imageFromIpCamera

Graph: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../GraphTest.cpp \
	$(NOMADS_HOME)/util/cpp/graph/*.h \
        $(NOMADS_HOME)/util/cpp/graph/*.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o GraphTest

TestTypes : TestTypes.o
	$(CPP) -o TestTypes TestTypes.o $(LD_FLAGS)

TestCompass : libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../TestCompass.cpp \
        $(NOMADS_HOME)/util/cpp/comm/*.h \
        $(NOMADS_HOME)/util/cpp/comm/*.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o TestCompass

ProxyDatagramSocketTest : libutil.a
		$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../ProxyDatagramSocketTest.cpp \
		$(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-o ProxyDatagramSocketTest

netmsgsvc: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../NetworkMessageServiceTest.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o netmsgsvc

MCastRelayer : libutil.a ../MCastRelayer.cpp
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../MCastRelayer.cpp \
        $(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o MCastRelayer

FIFOBufferTest : libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../FIFOBufferTest.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o FIFOBufferTest

BoundedPtrLListTest : libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../BoundedPtrLListTest.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o BoundedPtrLListTest

SerializationTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../SerializationTest.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
        -o SerializationTest

SAckTSNRangeHandlerTest: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
	../SAckTSNRangeHandlerTest.cpp \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	-o SAckTSNRangeHandlerTest

libutil.a :
	(cd $(NOMADS_HOME)/util/cpp/linux; make)

libutil_v4l2.a :
	(cd $(NOMADS_HOME)/util/cpp/linux; make -f Makefile-v4l2)

libjpegall.a :
	(cd $(NOMADS_HOME)/externals/jpeg-6b; make all-lib)

clean :
	(cd $(NOMADS_HOME)/util/cpp/linux; make clean)
	rm -rf *.o *.a multicast_echo wildNetIFs netIFs multicast_receiver multicast_sender netmsgsvc BoundedPtrLListTest \
	SAckTSNRangeHandlerTest SetUniquePtrLListTest imageFromIpCamera NetworkMessageBigDataReceiverTest NetworkMessageReceiverTest \
	ProxyDatagramSocketTest  SerializationTest FIFOBufferTest llistener MCastRelayer NetworkMessageBigDataSenderTest \
	NetworkMessageSenderTest RangeDLListTestTest TestTypes
