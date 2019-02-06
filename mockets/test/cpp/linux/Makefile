CPP = g++

NOMADS_HOME = ../../../..
ACI_HOME = $(NOMADS_HOME)/aci
MOCKETS_HOME = $(NOMADS_HOME)/mockets
UTIL_HOME = $(NOMADS_HOME)/util
EXTERNALS = $(NOMADS_HOME)/externals
EXTERNALS_LIB = $(EXTERNALS)/lib/$(ARCH)
OPENSSL_HOME = $(EXTERNALS)/openssl/1.0.2h/$(ARCH)
MSGPACK_HOME = $(EXTERNALS)/msgpack
LIBUDT_HOME = $(EXTERNALS)/udt4/src
LIBZ_HOME = $(EXTERNALS)/zlib
LZMA_HOME = $(EXTERNALS)/xz
LZMA_LIB = $(LZMA_HOME)/src/liblzma/api

LIB_FOLDER = linux
MAKEFILE_FOLDER = linux
ARCH = $(shell sh $(UTIL_HOME)/scripts/guessArch.sh)

CPPFLAGS = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
			-I$(MOCKETS_HOME)/cpp \
			-I$(UTIL_HOME)/cpp \
			-I$(EXTERNALS)/include \
			-I$(LIBUDT_HOME)

LIB_LIST = $(MOCKETS_HOME)/cpp/$(LIB_FOLDER)/libmockets.a \
			$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
			$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
			$(MSGPACK_HOME)/$(LIB_FOLDER)/libmsgpack.a \
			$(LIBZ_HOME)/$(LIB_FOLDER)/libz.a \
			$(LZMA_HOME)/$(LIB_FOLDER)/liblzma.a \
			-L$(LIBUDT_HOME) \
			-L$(OPENSSL_HOME)/lib

LD_FLAGS = -lpcap -lssl -lcrypto -ludt -lpthread -ldl

sources = $(wildcard /*.cpp)
objects = $(sources:../%.cpp=%.o)

tests = ARLTestCase BasicTest BioEnvMonitoringStation BlockedWriterTest \
        BufferEndlessRecv BufferEndlessSend ClientServerShell DataSendReceive \
        DeleteMessageTest FileRecv FileSend FreezeDefrost FreezeDefrostServerSide \
		GatherSendTest IntDataTest IntDataTestUnrelUnseq MessageReplaceTest \
        MigrationFileRec MocketStatusMonitorTest MultipleFreezeDefrost \
        MultipleFreezeDefrostServerSide OneProcessTest Qed QedClient \
        QedClientTest2 QedClientTest3 QedServer QedServerTest2 QedServerTest3 \
        QedTest2 QedTest3 RecvCongestion ReEstablishConnection RemoteStatsTest \
        RetryTimeoutTest RTTClientServerTest RTTEstimator SendCongestion \
        SimultaneousFreezeDefrost SimultaneousFreezeDefrostServerSide TestClient \
        TestServer UnreliableIntDataTest UnreliableSequencedReassemblyTest \
		UnreliableSequencedTest

all : $(tests)

libmockets.a :
	make -C $(MOCKETS_HOME)/cpp/linux/ libmockets.a

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/linux/ libutil.a

libmsgpack.a :
	make -C $(NOMADS_HOME)/externals/msgpack/linux/ libmsgpack.a

libz.a :
	make -C $(NOMADS_HOME)/externals/zlib/linux/ libz.a

liblzma.a :
	make -C $(NOMADS_HOME)/externals/xz/linux/ liblzma.a

libudt.a :
	make -C $(EXTERNALS)/udt4/src/ libudt.a

libsecurity.a :
	make -C $(UTIL_HOME)/cpp/linux libsecurity.a
	

IntDataTest : IntDataTest.o libmockets.a liblzma.a libz.a libmsgpack.a libudt.a
	$(CPP) $(CPPFLAGS) -Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas \
	../IntDataTest.cpp \
	$(LIB_LIST) $(LD_FLAGS) \
	-ldl -ludt -lpthread \
	-o IntDataTest 
	
ARLTestCase : ARLTestCase.o libmockets.a libutil.a libsecurity.a liblzma.a libz.a libmsgpack.a libudt.a
	$(CPP) $(CPPFLAGS) -o ARLTestCase ARLTestCase.o \
	$(LIB_LIST) $(LD_FLAGS) 

BasicTest : BasicTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o BasicTest BasicTest.o \
	$(LIB_LIST) $(LD_FLAGS)

BioEnvMonitoringStation : BioEnvMonitoringStation.o libmockets.a
	$(CPP) $(CPPFLAGS) -o BioEnvMonitoringStation BioEnvMonitoringStation.o \
	$(LIB_LIST) $(LD_FLAGS)

BlockedWriterTest : BlockedWriterTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o BlockedWriterTest BlockedWriterTest.o \
	$(LIB_LIST) $(LD_FLAGS)
	
BufferEndlessRecv: BufferEndlessRecv.o libmockets.a
	$(CPP) $(CPPFLAGS) -o BufferEndlessRecv BufferEndlessRecv.o \
	$(LIB_LIST) $(LD_FLAGS)

BufferEndlessSend: BufferEndlessSend.o libmockets.a
	$(CPP) $(CPPFLAGS) -o BufferEndlessSend BufferEndlessSend.o \
	$(LIB_LIST) $(LD_FLAGS)


ClientServerShell : ClientServerShell.o libmockets.a
	$(CPP) $(CPPFLAGS) -o ClientServerShell ClientServerShell.o \
	$(LIB_LIST) $(LD_FLAGS)
	
	
DataSendReceive : DataSendReceive.o libmockets.a
	$(CPP) $(CPPFLAGS) -o DataSendReceive DataSendReceive.o \
	$(LIB_LIST) $(LD_FLAGS)
	
DeleteMessageTest : DeleteMessageTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o DeleteMessageTest DeleteMessageTest.o \
	$(LIB_LIST) $(LD_FLAGS)

FileRecv : FileRecv.o libmockets.a
	$(CPP) $(CPPFLAGS) -o FileRecv FileRecv.o \
	$(LIB_LIST) $(LD_FLAGS)

FileSend : FileSend.o libmockets.a
	$(CPP) $(CPPFLAGS) -o FileSend FileSend.o \
	$(LIB_LIST) $(LD_FLAGS)

FreezeDefrost : FreezeDefrost.o libmockets.a libutil.a libsecurity.a
	$(CPP) $(CPPFLAGS) -o FreezeDefrost FreezeDefrost.o \
	$(LIB_LIST) $(LD_FLAGS)
	
FreezeDefrostServerSide : FreezeDefrostServerSide.o libmockets.a libutil.a libsecurity.a
	$(CPP) $(CPPFLAGS) -o FreezeDefrostServerSide FreezeDefrostServerSide.o \
	$(LIB_LIST) $(LD_FLAGS)


GatherSendTest : GatherSendTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o GatherSendTest GatherSendTest.o \
	$(LIB_LIST) $(LD_FLAGS)


IntDataTest2 : IntDataTest2.o libmockets.a libudt.a
	$(CPP) $(CPPFLAGS) -g -Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas -DUNIX -DLINUX -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
	../IntDataTest2.cpp \
	$(LIB_LIST) $(LD_FLAGS) \
	-ldl -ludt -lpthread \
	-o IntDataTest2 

IntDataTestUnrelUnseq : IntDataTestUnrelUnseq.o libmockets.a
	$(CPP) -o IntDataTestUnrelUnseq IntDataTestUnrelUnseq.o \
	$(LIB_LIST) $(LD_FLAGS)

MessageReplaceTest : MessageReplaceTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o MessageReplaceTest MessageReplaceTest.o \
	$(LIB_LIST) $(LD_FLAGS)
	
	
MigrationFileRec : MigrationFileRec.o libmockets.a
	$(CPP) $(CPPFLAGS) -o MigrationFileRec MigrationFileRec.o \
	$(LIB_LIST) $(LD_FLAGS)

MigrationFileSend : MigrationFileSend.o libmockets.a
	$(CPP) $(CPPFLAGS) -o MigrationFileSend MigrationFileSend.o \
	$(LIB_LIST) $(LD_FLAGS)
	
MocketStatusMonitorTest : MocketStatusMonitorTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o MocketStatusMonitorTest MocketStatusMonitorTest.o \
	$(LIB_LIST) $(LD_FLAGS)

MultipleFreezeDefrost : MultipleFreezeDefrost.o libmockets.a libutil.a libsecurity.a
	$(CPP) $(CPPFLAGS) -o MultipleFreezeDefrost MultipleFreezeDefrost.o \
	$(LIB_LIST) $(LD_FLAGS)

MultipleFreezeDefrostServerSide : MultipleFreezeDefrostServerSide.o libmockets.a libutil.a libsecurity.a
	$(CPP) $(CPPFLAGS) -o MultipleFreezeDefrostServerSide MultipleFreezeDefrostServerSide.o \
	$(LIB_LIST) $(LD_FLAGS)

OneProcessTest : OneProcessTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o OneProcessTest OneProcessTest.o \
	$(LIB_LIST) $(LD_FLAGS)

Qed : Qed.o libmockets.a
	$(CPP) $(CPPFLAGS) -o Qed Qed.o \
	$(LIB_LIST) $(LD_FLAGS)

QedClient : QedClient.o libmockets.a
	$(CPP) $(CPPFLAGS) -o QedClient QedClient.o \
	$(LIB_LIST) $(LD_FLAGS)

QedClientTest2 : QedClientTest2.o libmockets.a
	$(CPP) $(CPPFLAGS) -o QedClientTest2 QedClientTest2.o \
	$(LIB_LIST) $(LD_FLAGS)

QedClientTest3 : QedClientTest3.o libmockets.a
	$(CPP) $(CPPFLAGS) -o QedClientTest3 QedClientTest3.o \
	$(LIB_LIST) $(LD_FLAGS)

QedServer : QedServer.o libmockets.a
	$(CPP) $(CPPFLAGS) -o QedServer QedServer.o \
	$(LIB_LIST) $(LD_FLAGS)

QedServerTest2 : QedServerTest2.o libmockets.a
	$(CPP) $(CPPFLAGS) -o QedServerTest2 QedServerTest2.o \
	$(LIB_LIST) $(LD_FLAGS)

QedServerTest3 : QedServerTest3.o libmockets.a
	$(CPP) $(CPPFLAGS) -o QedServerTest3 QedServerTest3.o \
	$(LIB_LIST) $(LD_FLAGS)

QedTest2 : QedTest2.o libmockets.a
	$(CPP) $(CPPFLAGS) -o QedTest2 QedTest2.o \
	$(LIB_LIST) $(LD_FLAGS)

QedTest3 : QedTest3.o libmockets.a
	$(CPP) $(CPPFLAGS) -o QedTest3 QedTest3.o \
	$(LIB_LIST) $(LD_FLAGS)

RecvCongestion : RecvCongestion.o libmockets.a
	$(CPP) $(CPPFLAGS) -o RecvCongestion RecvCongestion.o \
	$(LIB_LIST) $(LD_FLAGS)
ReEstablishConnection : ReEstablishConnection.o libmockets.a
	$(CPP) $(CPPFLAGS) -o ReEstablishConnection ReEstablishConnection.o \
	$(LIB_LIST) $(LD_FLAGS)

RemoteStatsTest : RemoteStatsTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o RemoteStatsTest RemoteStatsTest.o \
	$(LIB_LIST) $(LD_FLAGS)

RetryTimeoutTest: RetryTimeoutTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o RetryTimeoutTest RetryTimeoutTest.o \
	$(LIB_LIST) $(LD_FLAGS)

RTTClientServerTest : RTTClientServerTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o RTTClientServerTest RTTClientServerTest.o \
	$(LIB_LIST) $(LD_FLAGS)

RTTEstimator : RTTEstimator.o
	$(CPP) $(CPPFLAGS) -o RTTEstimator RTTEstimator.o \
	$(LIB_LIST) $(LD_FLAGS)

SendCongestion : SendCongestion.o libmockets.a
	$(CPP) $(CPPFLAGS) -o SendCongestion SendCongestion.o \
	$(LIB_LIST) $(LD_FLAGS)

SimultaneousFreezeDefrost : SimultaneousFreezeDefrost.o libmockets.a libutil.a libsecurity.a
	$(CPP) $(CPPFLAGS) -o SimultaneousFreezeDefrost SimultaneousFreezeDefrost.o \
	$(LIB_LIST) $(LD_FLAGS)

SimultaneousFreezeDefrostServerSide : SimultaneousFreezeDefrostServerSide.o libmockets.a libutil.a libsecurity.a
	$(CPP) $(CPPFLAGS) -o SimultaneousFreezeDefrostServerSide SimultaneousFreezeDefrostServerSide.o \
	$(LIB_LIST) $(LD_FLAGS)

TestClient : TestClient.o libmockets.a
	$(CPP) $(CPPFLAGS) -o TestClient TestClient.o \
	$(LIB_LIST) $(LD_FLAGS)

TestServer : TestServer.o libmockets.a
	$(CPP) $(CPPFLAGS) -o TestServer TestServer.o \
	$(LIB_LIST) $(LD_FLAGS)

UnreliableIntDataTest : UnreliableIntDataTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o UnreliableIntDataTest UnreliableIntDataTest.o \
	$(LIB_LIST) $(LD_FLAGS)

UnreliableSequencedTest : UnreliableSequencedTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o UnreliableSequencedTest UnreliableSequencedTest.o \
	$(LIB_LIST) $(LD_FLAGS)

UnreliableSequencedReassemblyTest : UnreliableSequencedReassemblyTest.o libmockets.a
	$(CPP) $(CPPFLAGS) -o UnreliableSequencedReassemblyTest UnreliableSequencedReassemblyTest.o \
	$(LIB_LIST) $(LD_FLAGS)

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

clean :
	rm -rf *.o $(tests)

cleanall : clean
	(make -C $(MOCKETS_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(MSGPACK_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(LIBUDT_HOME)/ clean)
	(make -C $(LZMA_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER) clean)
