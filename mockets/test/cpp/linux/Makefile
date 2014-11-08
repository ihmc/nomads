MOCKETSDIR = ../../../cpp
UTILDIR = ../../../../util/cpp
CPP = g++

ifdef USE_X64_LIBS
  EXTERNALS_LIB = ../../../../externals/lib/linux-x86_64
else
  EXTERNALS_LIB = ../../../../externals/lib/linux-x86
endif

ifdef USE_SYSTEM_LIBS
  OPENSSL_LIBS = -lcrypto -lssl
else
  OPENSSL_LIBS = $(EXTERNALS_LIB)/libcrypto.a $(EXTERNALS_LIB)/libssl.a
endif

INCLUDES = -I$(MOCKETSDIR)/ -I$(UTILDIR)/
CPPFLAGS = -g -c -fPIC -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING \
	   -DLITTLE_ENDIAN_SYSTEM

LD_FLAGS = ${UTILDIR}/linux/libutil.a -ldl -lpthread

sources = ../ARLTestCase.cpp ../BasicTest.cpp \
          ../BioEnvMonitoringStation.cpp \
          ../BlockedWriterTest.cpp ../BufferEndlessRecv.cpp \
          ../BufferEndlessSend.cpp ../DataSendReceive.cpp \
          ../DeleteMessageTest.cpp ../FileRecv.cpp \
          ../FileSend.cpp ../FreezeDefrost.cpp ../FreezeDefrostServerSide.cpp \
          ../GatherSendTest.cpp ../IntDataTest.cpp ../IntDataTestUnrelUnseq.cpp \
          ../MessageReplaceTest.cpp ../MigrationFileRec.cpp \
          ../MocketStatusMonitorTest.cpp \
          ../MultipleFreezeDefrost.cpp MultipleFreezeDefrostServerSide.cpp \
          ../OneProcessTest.cpp ../Qed.cpp \
          ../QedClient.cpp ../QedClientTest2.cpp \
          ../QedClientTest3.cpp ../QedServer.cpp \
          ../QedServerTest2.cpp ../QedServerTest3.cpp \
          ../QedTest2.cpp ../QedTest3.cpp \
          ../RecvCongestion.cpp ../ReEstablishConnection.cpp \
          ../RemoteStatsTest.cpp ../RetryTimeoutTest.cpp \
          ../RTTClientServerTest.cpp ../RTTEstimator.cpp \
          ../SendCongestion.cpp \
          ../SimultaneousFreezeDefrost.cpp ../SimultaneousFreezeDefrostServerSide.cpp \
          ../TestClient.cpp ../TestServer.cpp \
          ../UnreliableIntDataTest.cpp ../UnreliableSequencedReassemblyTest.cpp 
            
objects = $(sources:../%.cpp=%.o)

tests = ARLTestCase BasicTest BioEnvMonitoringStation BlockedWriterTest \
        BufferEndlessRecv BufferEndlessSend ClientServerShell DataSendReceive \
        DeleteMessageTest FileRecv FileSend FreezeDefrost FreezeDefrostServerSide GatherSendTest IntDataTest IntDataTestUnrelUnseq \
        MessageReplaceTest MigrationFileRec MocketStatusMonitorTest \
        MultipleFreezeDefrost MultipleFreezeDefrostServerSide OneProcessTest \
        Qed QedClient QedClientTest2 QedClientTest3 QedServer QedServerTest2 \
        QedServerTest3 QedTest2 QedTest3 RecvCongestion ReEstablishConnection \
        RemoteStatsTest RetryTimeoutTest \
        RTTClientServerTest RTTEstimator SendCongestion \
        SimultaneousFreezeDefrost SimultaneousFreezeDefrostServerSide TestClient \
        TestServer UnreliableIntDataTest UnreliableSequencedReassemblyTest UnreliableSequencedTest

all : $(tests)

ARLTestCase : ARLTestCase.o libmockets.a libutil.a libsecurity.a
	$(CPP) -o ARLTestCase ARLTestCase.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

BandwidthControl : BandwidthControl.o libmockets.a
	$(CPP) -o BandwidthControl BandwidthControl.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

BasicTest : BasicTest.o libmockets.a
	$(CPP) -o BasicTest BasicTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

BioEnvMonitoringStation : BioEnvMonitoringStation.o libmockets.a
	$(CPP) -o BioEnvMonitoringStation BioEnvMonitoringStation.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

BlockedWriterTest : BlockedWriterTest.o libmockets.a
	$(CPP) -o BlockedWriterTest BlockedWriterTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)
	
BufferEndlessRecv: BufferEndlessRecv.o libmockets.a
	$(CPP) -o BufferEndlessRecv BufferEndlessRecv.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

BufferEndlessSend: BufferEndlessSend.o libmockets.a
	$(CPP) -o BufferEndlessSend BufferEndlessSend.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)


ClientServerShell : ClientServerShell.o libmockets.a
	$(CPP) -o ClientServerShell ClientServerShell.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

DataSendReceive : DataSendReceive.o libmockets.a
	$(CPP) -o DataSendReceive DataSendReceive.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

DeleteMessageTest : DeleteMessageTest.o libmockets.a
	$(CPP) -o DeleteMessageTest DeleteMessageTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

FileRecv : FileRecv.o libmockets.a
	$(CPP) -o FileRecv FileRecv.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

FileSend : FileSend.o libmockets.a
	$(CPP) -o FileSend FileSend.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

FreezeDefrost : FreezeDefrost.o libmockets.a libutil.a libsecurity.a
	$(CPP) -o FreezeDefrost FreezeDefrost.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

FreezeDefrostServerSide : FreezeDefrostServerSide.o libmockets.a libutil.a libsecurity.a
	$(CPP) -o FreezeDefrostServerSide FreezeDefrostServerSide.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)


GatherSendTest : GatherSendTest.o libmockets.a
	$(CPP) -o GatherSendTest GatherSendTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

IntDataTest : IntDataTest.o libmockets.a
	$(CPP) -g -Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas -DUNIX -DLINUX -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM $(INCLUDES) \
	../IntDataTest.cpp \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	-ldl -lpthread \
	-o IntDataTest 

IntDataTestUnrelUnseq : IntDataTestUnrelUnseq.o libmockets.a
	$(CPP) -o IntDataTestUnrelUnseq IntDataTestUnrelUnseq.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

MessageReplaceTest : MessageReplaceTest.o libmockets.a
	$(CPP) -o MessageReplaceTest MessageReplaceTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

MigrationFileRec : MigrationFileRec.o libmockets.a
	$(CPP) -o MigrationFileRec MigrationFileRec.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

MigrationFileSend : MigrationFileSend.o libmockets.a
	make -C $(MOCKETSDIR)/linux clean 
	make -f Makefile_debug_migration -C $(MOCKETSDIR)/linux libmockets.a
	$(CPP) -o MigrationFileSend MigrationFileSend.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

MocketStatusMonitorTest : MocketStatusMonitorTest.o libmockets.a
	$(CPP) -o MocketStatusMonitorTest MocketStatusMonitorTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(LD_FLAGS)

MultipleFreezeDefrost : MultipleFreezeDefrost.o libmockets.a libutil.a libsecurity.a
	$(CPP) -o MultipleFreezeDefrost MultipleFreezeDefrost.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

MultipleFreezeDefrostServerSide : MultipleFreezeDefrostServerSide.o libmockets.a libutil.a libsecurity.a
	$(CPP) -o MultipleFreezeDefrostServerSide MultipleFreezeDefrostServerSide.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

OneProcessTest : OneProcessTest.o libmockets.a
	$(CPP) -o OneProcessTest OneProcessTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

Qed : Qed.o libmockets.a
	$(CPP) -o Qed Qed.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

QedClient : QedClient.o libmockets.a
	$(CPP) -o QedClient QedClient.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

QedClientTest2 : QedClientTest2.o libmockets.a
	$(CPP) -o QedClientTest2 QedClientTest2.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

QedClientTest3 : QedClientTest3.o libmockets.a
	$(CPP) -o QedClientTest3 QedClientTest3.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

QedServer : QedServer.o libmockets.a
	$(CPP) -o QedServer QedServer.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

QedServerTest2 : QedServerTest2.o libmockets.a
	$(CPP) -o QedServerTest2 QedServerTest2.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

QedServerTest3 : QedServerTest3.o libmockets.a
	$(CPP) -o QedServerTest3 QedServerTest3.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

QedTest2 : QedTest2.o libmockets.a
	$(CPP) -o QedTest2 QedTest2.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

QedTest3 : QedTest3.o libmockets.a
	$(CPP) -o QedTest3 QedTest3.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

RecvCongestion : RecvCongestion.o libmockets.a
	$(CPP) -o RecvCongestion RecvCongestion.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

ReEstablishConnection : ReEstablishConnection.o libmockets.a
	$(CPP) -o ReEstablishConnection ReEstablishConnection.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

RemoteStatsTest : RemoteStatsTest.o libmockets.a
	$(CPP) -o RemoteStatsTest RemoteStatsTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libutil.a \
	$(LD_FLAGS)

RetryTimeoutTest: RetryTimeoutTest.o libmockets.a
	$(CPP) -o RetryTimeoutTest RetryTimeoutTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

RTTClientServerTest : RTTClientServerTest.o libmockets.a
	$(CPP) -o RTTClientServerTest RTTClientServerTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

RTTEstimator : RTTEstimator.o
	$(CPP) -o RTTEstimator RTTEstimator.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

SendCongestion : SendCongestion.o libmockets.a
	$(CPP) -o SendCongestion SendCongestion.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

SimultaneousFreezeDefrost : SimultaneousFreezeDefrost.o libmockets.a libutil.a libsecurity.a
	$(CPP) -o SimultaneousFreezeDefrost SimultaneousFreezeDefrost.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

SimultaneousFreezeDefrostServerSide : SimultaneousFreezeDefrostServerSide.o libmockets.a libutil.a libsecurity.a
	$(CPP) -o SimultaneousFreezeDefrostServerSide SimultaneousFreezeDefrostServerSide.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)


TestClient : TestClient.o libmockets.a
	$(CPP) -o TestClient TestClient.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

TestServer : TestServer.o libmockets.a
	$(CPP) -o TestServer TestServer.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

UnreliableIntDataTest : UnreliableIntDataTest.o libmockets.a
	$(CPP) -o UnreliableIntDataTest UnreliableIntDataTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/Socket.cpp \
	$(UTILDIR)/TCPSocket.cpp \
	$(UTILDIR)/StrClass.cpp \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

UnreliableSequencedTest : UnreliableSequencedTest.o libmockets.a
	$(CPP) -o UnreliableSequencedTest UnreliableSequencedTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

UnreliableSequencedReassemblyTest : UnreliableSequencedReassemblyTest.o libmockets.a
	$(CPP) -o UnreliableSequencedReassemblyTest UnreliableSequencedReassemblyTest.o \
	$(MOCKETSDIR)/linux/libmockets.a \
	$(UTILDIR)/linux/libsecurity.a \
	$(UTILDIR)/linux/libutil.a \
	$(OPENSSL_LIBS) \
	$(LD_FLAGS)

libmockets.a :
	make -C $(MOCKETSDIR)/linux libmockets.a

libutil.a :
	make -C $(UTILDIR)/linux libutil.a

libsecurity.a :
	make -C $(UTILDIR)/linux libsecurity.a

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $(INCLUDES) $<

clean :
	rm -rf *.o $(tests) MigrationFileSend
	make -C $(MOCKETSDIR)/linux clean
	#make -C $(UTILDIR)/linux clean
	

