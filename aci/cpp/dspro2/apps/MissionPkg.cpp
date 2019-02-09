#include "MissionPkg.h"

#include "Defs.h"
#include "DSProImpl.h"
#include "MetaData.h"
#include "MetadataHelper.h"

#include "MimeUtils.h"

#include "Base64Transcoders.h"
#include "BufferReader.h"
#include "CompressedReader.h"
#include "FileWriter.h"
#include "File.h"
#include "FileUtils.h"
#include "Logger.h"
#include "Json.h"
#include "ZipFileReader.h"

#include "tinyxml.h"

#include <stdio.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace MISSION_PACKAGE
{
    void setMimeType (const String &filename, String &mimeType)
    {
        File innerFile (filename);
        String extension (innerFile.getExtension());
        if (extension == "doc") {
            mimeType = "application/msword";
        }
        else if (extension == "docx") {
            mimeType = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        }
        else if (extension == "xls") {
            mimeType = "application/vnd.ms-excel";
        }
        else if (extension == "xlsx") {
            mimeType = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
        }
        else if (extension == "ppt") {
            mimeType = "application/vnd.ms-powerpoint";
        }
        else if (extension == "pptx") {
            mimeType = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
        }
        else if (extension == "cot") {
            mimeType = "x-dspro/x-phoenix-cot";
        }
        else if (extension == "xml") {
            mimeType = "application/xml";
        }
        else {
            mimeType = IHMC_MISC::MimeUtils::toMimeType (IHMC_MISC::MimeUtils::toType (extension));
        }
        if (mimeType.length () <= 0) {
            mimeType = "application/octet-stream";
            checkAndLogMsg ("MissionPackage::setMimeType", Logger::L_Warning,
                            "could not guess MIME type for file %s.\n", filename.c_str());
        }
    }

    bool isZip (const String &mimeType)
    {
        static const char* const ZIP_MIME_TYPES[] = {
            "application/x-zip-compressed", "application/zip", "application/x-zip", nullptr
        };
        for (uint8 i = 0; ZIP_MIME_TYPES[i] != nullptr; i++) {
            if (mimeType ^= ZIP_MIME_TYPES[i]) {
                return true;
            }
        }
        return false;
    }

    /*
     <?xml version="1.0"?>
     <MissionPackageManifest version="2">
         <Configuration>
             <Parameter value="1d704a6b-8e9f-4b24-90c6-fe619a543657" name="uid"/>
             <Parameter value="DP-THOR" name="name"/>
         </Configuration>
         <Contents>
             <Content zipEntry="PathA1/PathA1.cot" ignore="false">
                 <Parameter value="PathA1" name="uid"/>
             </Content>
         </Contents>
     </MissionPackageManifest>
    */
    String modifyManifest (const String &manifest)
    {
        TiXmlDocument doc;
        doc.Parse (manifest);
        TiXmlElement *pContents = doc.FirstChildElement ("Contents");
        if (pContents != nullptr) {
            doc.RemoveChild (pContents);
            // Also remove the parameters from configuration
        }
        TiXmlElement *pParameter = new TiXmlElement ("Parameter");
        pParameter->SetAttribute ("value", "metaContent");

        TiXmlElement *pContent = new TiXmlElement ("Content");
        pContent->SetAttribute ("zipEntry", "metaContent/metaContent.json");
        pContent->SetAttribute ("ignore", "false");
        pContent->LinkEndChild (pParameter);

        pContents = new TiXmlElement ("Contents");
        pContents->LinkEndChild (pContent);

        doc.LinkEndChild (pContents);

        // To string
        TiXmlPrinter printer;
        doc.Accept (&printer);
        return String (printer.CStr());
    }

    class Archive
    {
        virtual int addAsChunkedMissionPkg (DSProImpl *pDSPro, const char *pszGroupName,
            const char *pszObjectId, const char *pszInstanceId,
            MetaData *pMetadata, int64 i64ExpirationTime,
            char **ppszId) = 0;
    };

    class ZipArchive : public Archive
    {
        public:
            ZipArchive (const void *pData, uint32 ui32Len, const char *pszDataMimeType);
            ~ZipArchive (void);

            int addAsChunkedMissionPkg (DSProImpl *pDSPro, const char *pszGroupName,
                                        const char *pszObjectId, const char *pszInstanceId,
                                        MetaData *pMetadata, int64 i64ExpirationTime,
                                        char **ppszId);

        private:
            const void *_pData;
            const uint32 _ui32Len;
            const NOMADSUtil::String _mimeType;
    };
}

using namespace MISSION_PACKAGE;

ZipArchive::ZipArchive (const void *pData, uint32 ui32Len, const char *pszDataMimeType)
    : _pData (pData),
      _ui32Len (ui32Len),
      _mimeType (pszDataMimeType)
{
}

ZipArchive::~ZipArchive (void)
{
}

int ZipArchive::addAsChunkedMissionPkg (DSProImpl *pDSPro, const char *pszGroupName,
                                        const char *pszObjectId, const char *pszInstanceId,
                                        MetaData *pMetadata, int64 i64ExpirationTime,
                                        char **ppszId)
{
    const char *pszMethodName = "ZipArchive::addAsChunkedMissionPkg";
    FILE *pZipFile = nullptr;
#ifdef ANDROID
    if (!FileUtils::directoryExists ("/sdcard/ihmc/tmp")) {
        FileUtils::createDirectory ("/sdcard/ihmc/tmp");
    }
    static const char templ[] = "/sdcard/ihmc/tmp/zipXXXXXX";
    char fname[2048];
    strcpy (fname, templ);
    int fd = mkstemp (fname);
    if (fd > 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "trying to create temporary file: %s.\n", fname);
        pZipFile = fdopen (fd, "w+");
    }
    else {
        char err[1024];
        err[0] = '\0';
        strerror_r (fd, err, 1024);
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "couldn't generate temporary file name: %s. "
                        "Error: %s.\n", fname, err);
    }
#else
    pZipFile = tmpfile();
#endif // ANDROID

    if (pZipFile == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not create temporary file.\n");
        return -1;
    }
    FileWriter fw (pZipFile, true);  // fw closes the file, no need to call fclose() on pZipFile!
    if ((fw.writeBytes (_pData, _ui32Len) < 0) || (fw.flush() < 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not write zip file on disk.\n");
        return -2;
    }
    rewind (pZipFile);
    ZipFileReader zr (false);
    if (zr.init (pZipFile) != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not init zip file reader for %s.\n");
        return -3;
    }
    String missionPkgFileName;
    pMetadata->getFieldValue (MetaData::DATA_CONTENT, missionPkgFileName);
    JsonObject json;
    JsonArray dsproIdsArray;
    const int iInnerFilesCount = zr.getFileCount();
    for (int i = 0; i < iInnerFilesCount; i++) {
        // Read each file in the zip archive
        ZipFileReader::Entry *pEntry = zr.getEntry (i);
        if (pEntry == nullptr) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "the %d -th entry is null.\n", i);
            return -4;
        }
        const String innerFileFullName (pEntry->pszName);
        File fInnerFileFullName (innerFileFullName, '/', true);
        const String innerFileName (fInnerFileFullName.getName (false));
        /*if ((innerFileName ^= "manifest.xml") || (innerFileName.startsWith ("MANIFEST") == 1)) {
            // Skip manifest
            continue;
        }*/
        const bool bIsManifest = innerFileName.endsWith ("manifest.xml");
        if ((pEntry->pBuf == nullptr) || (pEntry->ui32CompSize == 0U)) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not read empty file %s.\n", pEntry->pszName);
            delete pEntry;
            continue;
        }
        // Uncompress it
        BufferReader br (pEntry->pBuf, pEntry->ui32CompSize + 1);
        CompressedReader cr (&br, false, true);
        void *pUnzippedFile = malloc (pEntry->ui32UncompSize);
        if (pUnzippedFile == nullptr) {
            delete pEntry;
            return -6;
        }
        if (cr.readBytes (pUnzippedFile, pEntry->ui32UncompSize) < 0) {
            free (pUnzippedFile);
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not read file %s.\n", pEntry->pszName);
            delete pEntry;
            return -7;
        }
        // Create object id for the inner object
        String objectId (pszObjectId);
        if (objectId.length() > 0) {
            objectId += '-';
        }
        // Chunk inner unzipped object
        String mimeType;
        MISSION_PACKAGE::setMimeType (innerFileName, mimeType);
        objectId += innerFileName;
        char *pszInnerObjMsgId = nullptr;
        MetaData *pInnerObjMetadata = pMetadata->clone();
        if (pInnerObjMetadata == nullptr) {
            free (pUnzippedFile);
            delete pEntry;
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not clone metadata.\n");
            return -8;
        }
        pInnerObjMetadata->setFieldValue (MetaData::DATA_CONTENT, "innerFileName");
        const bool bStoreInInfoStore = false;
        String group (pszGroupName);
        group += bIsManifest ? ".manifest" : ".part";
        int rc = pDSPro->chunkAndAddMessage (group, objectId, pszInstanceId,
                                             pInnerObjMetadata, pUnzippedFile, pEntry->ui32UncompSize,
                                             mimeType, i64ExpirationTime, &pszInnerObjMsgId,
                                             bStoreInInfoStore);
        free (pUnzippedFile);
        delete pEntry;
        if ((pszInnerObjMsgId == nullptr) || (rc != 0)) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not add file %s.\n", innerFileFullName.c_str());
            if (pszInnerObjMsgId != nullptr) {
                free (pszInnerObjMsgId);
            }
            return -9;
        }
        else if (bIsManifest) {
            String referredMessageId;
            pInnerObjMetadata->getReferredDataMsgId (referredMessageId);
            json.setString ("manifest", referredMessageId);
        }
        else if (pInnerObjMetadata != nullptr) {
            JsonObject innerObject;
            String referredMessageId;
            pInnerObjMetadata->getReferredDataMsgId (referredMessageId);
            if (referredMessageId == (MetaData::NO_REFERRED_OBJECT)) {
                innerObject.setString ("referredMessageId", pszInnerObjMsgId);
            }
            else {
                innerObject.setString ("referredMessageId", referredMessageId);
            }
            innerObject.setString ("mimeType", mimeType);
            innerObject.setString ("fileName", innerFileFullName);
            dsproIdsArray.addObject (&innerObject);
        }
        delete pInnerObjMetadata;
        free (pszInnerObjMsgId);
    }
    json.setString ("fileName", missionPkgFileName);
    json.setObject ("content", &dsproIdsArray);
    String sJson (json.toString (true));
    char *pszEncodedJson = Base64Transcoders::encode (sJson, sJson.length());
    if (pszEncodedJson == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not base64-encode json mission package.\n");
        return -10;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "created mission package <%s>.\n", sJson.c_str());
    pMetadata->setFieldValue (MetaData::DATA_FORMAT, "application/vnd.mission-package");
    pMetadata->setFieldValue (MetaData::APPLICATION_METADATA, pszEncodedJson);
    free (pszEncodedJson);
    // pMetadata->setFieldValue (MetaData::APPLICATION_METADATA_FORMAT, "application/json-base64");
    pMetadata->setFieldValue (MetaData::APPLICATION_METADATA_FORMAT, "Soigen2_Json_Base64");
    int rc = pDSPro->addMessage (pszGroupName, pszObjectId, pszInstanceId,
                                 pMetadata, nullptr, 0, i64ExpirationTime, ppszId);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "could not store mission package.\n");
        return -11;
    }
    return 0;
}

MissionPackage::MissionPackage (void)
{
}

MissionPackage::~MissionPackage (void)
{
}

bool MissionPackage::isMissionPackage (const char *pszMimeType)
{
    const String mimeType (pszMimeType);
    return isZip (mimeType);
}

int MissionPackage::addAsChunkedMissionPkg (DSProImpl *pDSPro, const char *pszGroupName,
                                            const char *pszObjectId, const char *pszInstanceId,
                                            MetaData *pMetadata, const void *pData, uint32 ui32DataLen,
                                            const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "MissionPackage::addAsChunkedMissionPkg";
    if ((pData == nullptr) || (ui32DataLen == 0U)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "data is nullptr.\n");
        return -1;
    }
    const String mimeType (pszDataMimeType);
    if (isZip (mimeType)) {
        ZipArchive archive (pData, ui32DataLen, pszDataMimeType);
        return archive.addAsChunkedMissionPkg (pDSPro, pszGroupName, pszObjectId, pszInstanceId,
                                               pMetadata, i64ExpirationTime, ppszId);
    }
    checkAndLogMsg (pszMethodName, Logger::L_Warning, "data MIME type not supported: <%s>.\n",
                    (pszDataMimeType == nullptr ? "" : pszDataMimeType));
    return -2;
}

