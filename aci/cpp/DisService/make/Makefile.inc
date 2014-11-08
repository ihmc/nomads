#
# Disservice Include Makefile
#
# author Giacomo Benincasa (gbenincasa@ihmc.us)
#

#Paths
PROJECT_ROOT = ../../../../..
NOMADS_HOME = ../../../..
EXTERNALS = $(NOMADS_HOME)/externals

SQLITE_HOME = $(EXTERNALS)/SQLite
SQLITE_LIB = $(SQLITE_HOME)/linux/.libs
TINYXPATH_HOME = $(NOMADS_HOME)/externals/TinyXPath
LCPPDC_HOME= $(NOMADS_HOME)/misc/cpp/lcppdc
CHUNKING_HOME= $(NOMADS_HOME)/misc/cpp/chunking
JPEG_HOME=$(NOMADS_HOME)/externals/libjpeg
JASPER_HOME=$(NOMADS_HOME)/externals/jasper
PNG_HOME=$(NOMADS_HOME)/externals/lpng1513
MSGPACK_HOME = $(EXTERNALS)/msgpack

#Compiler
CPP = g++

LD_FLAGS = -lpthread

#Environment
ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
include Makefile.$(ARCH)

CPPFLAGS = -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unknown-pragmas \
	-DUNIX $(BUILD_FLAGS) -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
	-I.. \
	-isystem $(EXTERNALS)/include \
	-I$(NOMADS_HOME)/util/cpp \
	-I$(NOMADS_HOME)/util/cpp/net \
	-I$(NOMADS_HOME)/util/cpp/graph \
	-I$(SQLITE_HOME)/ \
	-I$(LCPPDC_HOME)/ \
	-I$(CHUNKING_HOME)/ \
	-isystem $(TINYXPATH_HOME)/ \
	-I$(MOCKETS_HOME)/cpp \
	-I$(NOMADS_HOME)/externals/JDK \
        -I$(NOMADS_HOME)/externals/JDK/$(LIB_FOLDER)

SOURCES = ../AckController.cpp ../Analytics.cpp \
	../BandwidthSensitiveController.cpp ../BandwidthSharing.cpp \
	../BasicWorldState.cpp ../ChunkingAdaptor.cpp \
	../ChunkRetrievalController.cpp ../ConfigFileReader.cpp \
	../ConnectivityHistory.cpp ../Controllable.cpp \
	../ControllerFactory.cpp ../DataCache.cpp \
	../DataCacheExpirationController.cpp ../DataCacheInterface.cpp \
	../DataCacheReplicationController.cpp ../DataRequestHandler.cpp \
	../DefaultDataCacheExpirationController.cpp \
	../DefaultDataCacheReplicationController.cpp \
	../DefaultForwardingController.cpp ../DisseminationService.cpp \
	../SearchController.cpp ../DefaultSearchController.cpp \
	../DisseminationServiceProxyAdaptor.cpp \
	../DisseminationServiceProxyCallbackHandler.cpp \
	../DisseminationServiceProxy.cpp \
	../DisseminationServiceProxyServer.cpp \
	../DisServiceCommandProcessor.cpp ../DisServiceDataCacheQuery.cpp \
	../DisServiceLauncher.cpp ../DisServiceMessageInjector.cpp \
	../DisServiceMsg.cpp ../DisServiceMsgHelper.cpp \
	../DisServiceStats.cpp ../DisServiceStatusMonitor.cpp \
	../DisServiceStatusNotifier.cpp ../DSSFLib.cpp \
	../ForwardingController.cpp ../History.cpp ../HistoryFactory.cpp \
	../Listener.cpp ../ListenerNotifier.cpp ../Message.cpp ../MessageInfo.cpp \
	../MessageReassembler.cpp ../MessageRequestScheduler.cpp ../MessageId.cpp \
	../NetworkTrafficMemory.cpp ../NodeId.cpp ../NodeInfo.cpp ../PeerState.cpp \
	../PersistentDataCache.cpp ../PullReplicationController.cpp \
	../PushReplicationController.cpp \
	../PushToConvoyDataCacheReplicationController.cpp \
	../PropertyStoreInterface.cpp \
	../RateEstimator.cpp ../ReceivedMessages.cpp ../ReceivedMessagesInterface.cpp \
	../RequestsState.cpp ../Searches.cpp ../Services.cpp \
	../SQLMessageHeaderStorage.cpp ../SQLMessageStorage.cpp \
	../SQLTransmissionHistory.cpp ../SQLPropertyStore.cpp \
	../StorageInterface.cpp \
	../Subscription.cpp ../SubscriptionFactory.cpp \
	../SubscriptionAdvTable.cpp ../SubscriptionForwardingController.cpp \
	../SubscriptionList.cpp ../SubscriptionStateCapped.cpp \
	../SubscriptionState.cpp ../TargetBasedReplicationController.cpp \
	../TopologyWorldState.cpp ../TopologyForwardingController.cpp \
	../TransmissionHistoryInterface.cpp ../TransmissionService.cpp \
	../TransmissionServiceListener.cpp ../Utils.cpp ../WorldState.cpp \
	../WorldStateForwardingController.cpp 

OBJECTS = $(SOURCES:../%.cpp=%.o)

ifdef USE_SYSTEM_LIBS
  EXTRA_LIB = -lcrypto -lz
else
  HAS_CRYPTO_LIB = $(shell g++ -lcrypto 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
  EXTRA_LIB += $(shell if [ ${HAS_CRYPTO_LIB} -gt 0 ]; then echo "$(EXTERNALS_LIB)/libcrypto.a"; else  echo "-lcrypto"; fi)

  HAS_Z_LIB = $(shell g++ -lz 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
  EXTRA_LIB += $(shell if [ ${HAS_Z_LIB} -gt 0 ]; then echo "$(EXTERNALS_LIB)/libz.a"; else  echo "-lz"; fi)
endif

LIB_LIST = $(LCPPDC_HOME)/$(LIB_FOLDER)/liblcppdc.a \
        $(CHUNKING_HOME)/$(LIB_FOLDER)/libchunking.a \
        $(SQLITE_HOME)/$(LIB_FOLDER)/libsqlite3.a \
        $(TINYXPATH_HOME)/$(LIB_FOLDER)/libtinyxpath.a \
        $(NOMADS_HOME)/util/cpp/$(LIB_FOLDER)/libsecurity.a \
        $(NOMADS_HOME)/util/cpp/$(LIB_FOLDER)/libutil.a \
        $(JASPER_HOME)/$(LIB_FOLDER)/libjasper.a \
        $(JPEG_HOME)/$(LIB_FOLDER)/libjpeg.a \
        $(PNG_HOME)/$(LIB_FOLDER)/libpng.a \
        $(MSGPACK_HOME)/$(LIB_FOLDER)/libmsgpack.a \
        $(EXTRA_LIB)

SOURCESPROXY = DisseminationServiceProxyAdaptor.cpp \
	DisseminationServiceProxyCallbackHandler.cpp \
	DisseminationServiceProxy.cpp \
	DisseminationServiceProxyServer.cpp
OBJECTSPROXY = DisseminationServiceProxyAdaptor.o \
	DisseminationServiceProxyCallbackHandler.o \
	DisseminationServiceProxy.o \
	DisseminationServiceProxyServer.o

