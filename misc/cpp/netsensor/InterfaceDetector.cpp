#include "InterfaceDetector.h"

using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{

    InterfaceDetector::InterfaceDetector(void)
    {
        _pAddresses = nullptr;
    }

    InterfaceDetector::~InterfaceDetector(void)
    {
#if defined (WIN32)
        free(_pAddresses);
        free(_pInterfaceInfo);
        // This is put specifically in the Windows directive because
        // it will fail in Linux. Without this, the names stored in the
        // list have their pointers deleted, but not the memory that's being pointed at
        for (int i = 0; i < _llistFriendlyNames.getCount(); i++) {
            delete[] _llistFriendlyNames.getNext();
    }
#elif defined (UNIX)
        freeifaddrs(_pAddresses);
#endif
        _llistFriendlyNames.removeAll();
    }

    void InterfaceDetector::init(void)
    {
#if defined (WIN32)
        _pAddresses = nullptr;
        _pInterfaceInfo = nullptr;

        // Initialize the windows iphlpapi structures
        initAdapterAddresses();
        initInterfaceInfo();

#elif defined (UNIX)
        _pAddresses = nullptr;
        if (getifaddrs(&_pAddresses) == -1) {
            printf("InterfaceDetector::Error getting network interfaces\n");
            return;
        }
#endif

        storeFriendlyNames();
    }

#if defined (WIN32)
    char* InterfaceDetector::getCharFriendlyName(IP_ADAPTER_ADDRESSES *pCurrAddr)
    {
        // Convert from wchar to char
        size_t len = wcslen(pCurrAddr->FriendlyName) * 2 + 1;
        char *pszFriendlyName = new char[len];
        pszFriendlyName[0] = 0;
        wcstombs(pszFriendlyName, pCurrAddr->FriendlyName, len);

        return pszFriendlyName;
    }

    char* InterfaceDetector::getRawDeviceName(const char *pszUserFriendlyName)
    {
        if (_pAddresses == nullptr) {
            printf("InterfaceDetector::Call init() before calling getRawDeviceName()\n");
            return nullptr;
        }
        char *pszAdapterName = nullptr;
        IP_ADAPTER_ADDRESSES *pCurrAddr = _pAddresses;
        char *pszFriendlyName = nullptr;

        // Loop through network interfaces
        while (pCurrAddr != nullptr) {
            pszFriendlyName = getCharFriendlyName(pCurrAddr);
            if (!strcmp(pszFriendlyName, pszUserFriendlyName)) {
                pszAdapterName = pCurrAddr->AdapterName;

                delete[] pszFriendlyName;
                return pszAdapterName;
            }
            pCurrAddr = pCurrAddr->Next;
        }
        delete[] pszFriendlyName;

        return pszAdapterName;
    }

    char* InterfaceDetector::getUserFriendlyName(const char *pszAdapterName)
    {
        if (_pAddresses == nullptr) {
            printf("InterfaceDetector::Call init() before calling getUserFriendlyName()\n");
            return nullptr;
        }
        IP_ADAPTER_ADDRESSES *pCurrAddr = _pAddresses;
        while (pCurrAddr != nullptr) {
            if (!strcmp(pCurrAddr->AdapterName, pszAdapterName)) {
                return getCharFriendlyName(pCurrAddr);
            }
        }
        return nullptr;
    }

    void InterfaceDetector::initAdapterAddresses(void)
    {
        // Init adapterAddresses structure
        DWORD dwRetVal;
        unsigned long ulOutBufLen = 16384;

        _pAddresses = nullptr;
        _pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(ulOutBufLen);
        memset(_pAddresses, 0, ulOutBufLen);

        dwRetVal = GetAdaptersAddresses(0, 0, nullptr, _pAddresses, &ulOutBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {

            free(_pAddresses);

            ulOutBufLen += 2048;
            _pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(ulOutBufLen);
            memset(_pAddresses, 0, ulOutBufLen);
            dwRetVal = GetAdaptersAddresses(0, 0, nullptr, _pAddresses, &ulOutBufLen);
        }

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            printf("InterfaceDetector::initAdapterAddresses:Buffer size not large enough\n");
        }
    }

    void InterfaceDetector::initInterfaceInfo(void)
    {
        // Init adapterAddresses structure
        DWORD dwRetVal;
        unsigned long ulOutBufLen = 16384;

        // Init the interfaceInfo structure
        dwRetVal = GetInterfaceInfo(nullptr, &ulOutBufLen);

        if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
            _pInterfaceInfo = (IP_INTERFACE_INFO *)malloc(ulOutBufLen);
            if (_pInterfaceInfo == nullptr) {
                printf("Unable to allocate memory needed to call GetInterfaceInfo\n");
            }
        }
        // Make a second call to GetInterfaceInfo to get
        // the actual data
        dwRetVal = GetInterfaceInfo(_pInterfaceInfo, &ulOutBufLen);

        if (dwRetVal == ERROR_NO_DATA) {
            printf("InterfaceDetector::initInterfaceInfo():No network adapters with IPv4 enabled on local system\n");
        }
        else if (dwRetVal != NO_ERROR) {
            printf("InterfaceDetector::initInterfaceInfo():GetInterfaceInfo failed with error: %d\n", dwRetVal);
        }

    }


    bool InterfaceDetector::isValidInterface(IP_ADAPTER_ADDRESSES *pCurrAddr)
    {
        if (_pInterfaceInfo == nullptr) {
            printf("InterfaceDetector::Call init() first\n");
        }

        IP_INTERFACE_INFO *pCurrInterface = _pInterfaceInfo;

        for (int i = 0; i < _pInterfaceInfo->NumAdapters; i++) {
            if (_pInterfaceInfo->Adapter[i].Index == pCurrAddr->IfIndex) {
                return true;
            }
        }

        return false;
    }

#endif

    // Returns the next user-friendly adapter name
    int InterfaceDetector::getNext(char **ppNextEl)
    {

        if (*ppNextEl = _llistFriendlyNames.getNext()) {
            return 1;
        }
        else {
            _llistFriendlyNames.resetGet();
        }

        return 0;
    }

    // Prints the list of user-friendly adapter names
    void InterfaceDetector::printUserFriendlyNames(void)
    {
        char *pNextAdapter = nullptr;
        int counter = 1;
        while (getNext(&pNextAdapter)) {
            printf("%d. %s\n", counter, pNextAdapter);
            counter++;
        }
    }

    void InterfaceDetector::storeFriendlyNames(void)
    {
#if defined (WIN32)
        IP_ADAPTER_ADDRESSES *pCurrAddr = _pAddresses;
        char *pNewName = nullptr;

        while (pCurrAddr != nullptr) {
            if (isValidInterface(pCurrAddr)) {
                pNewName = getCharFriendlyName(pCurrAddr);
                _llistFriendlyNames.append(pNewName);
            }
            pCurrAddr = pCurrAddr->Next;
        }
#elif defined (UNIX)
        ifaddrs *pCurrAddr = _pAddresses;
        while (pCurrAddr != nullptr) {
            if (pCurrAddr->ifa_addr->sa_family == AF_INET) {
                _llistFriendlyNames.append(pCurrAddr->ifa_name);
            }
            pCurrAddr = pCurrAddr->ifa_next;
        }
#endif
    }
}