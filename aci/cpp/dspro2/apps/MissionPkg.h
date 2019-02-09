#include "FTypes.h"

namespace IHMC_ACI
{
    class DSProImpl;
    class MetaData;


    class MissionPackage
    {
        public:
            MissionPackage (void);
            ~MissionPackage (void);

            bool isMissionPackage (const char *pszMimeType);
            int addAsChunkedMissionPkg (DSProImpl *pDSPro, const char *pszGroupName,
                                        const char *pszObjectId, const char *pszInstanceId,
                                        MetaData *pMetadata, const void *pData, uint32 ui32DataLen,
                                        const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId);
    };
}

