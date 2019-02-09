#include "FileUtils.h"
#include "Logger.h"
#include "NLFLib.h"

#include <sys/stat.h>
int startDSPro(uint16 ui16Port, const char *pszStorageDir, const char *pszVersion);
int startDisService(uint16 ui16Port, const char *pszStorageDir, const char *pszVersion);
int reloadTransmissionService();
int reloadCommAdaptors();
int stopDisServicePro();
