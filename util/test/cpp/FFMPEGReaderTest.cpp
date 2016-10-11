extern "C"
{
    #include "libavcodec/avcodec.h"
};

#include "FFMPEGReader.h"
#include "FFMPEGWriter.h"
#include "File.h"
#include "FileReader.h"
#include "FileWriter.h"
#include "FTypes.h"
#include "Logger.h"
#include "NLFLib.h"
#include "PPMImage.h"
#include "StrClass.h"

using namespace NOMADSUtil;

#define Images DArray2<ExtractedImage>
#define fail(loc,rc) pLogger->logMsg(loc, NOMADSUtil::Logger::L_SevereError, "function retured %d.\n", rc)

struct ExtractedImage
{
    unsigned int uiFrameIdx;
    PPMImage img;
};

String getOutFileName (const String &filename, uint32 ui32FrameIdx, const char *pszDescription, const char *pszExtension)
{
    const File file (filename);
    String out (file.getParent());
    if (!out.endsWith (getPathSepCharAsString())) {
        out += getPathSepChar();
    }
    out += file.getName (true);
    out += "_frame_";
    out += ui32FrameIdx;
    out += '_';
    out += pszDescription;
    if (pszExtension[0] != '.') {
        out += '.';
    }
    out += pszExtension;
    return out;
}

int encodeFile (const String &filename, const FFMPEGUtil::VideoEncodingProfile &profile,
                const FFMPEGUtil::VideoFormat &format, Images &images)
{
    int rc;
    const char *pszFuncName = "encodeFile";
    if ((rc = images.size()) == 0) {
        fail (pszFuncName, rc);
        return -1;
    }
    unsigned int uiQuality = 0;
    bool bConvertAudio = false;
    FFMPEGUtil::Audio *pAudio = NULL;
    FFMPEGUtil::VideoEncodingProfile tmp (profile);
    if (!FFMPEGUtil::supportsEncoder (profile._videoCodec)) {
        tmp._videoCodec = FFMPEGUtil::getDefaultVideoCodec();
    }
    const String outFilename (getOutFileName (filename, images.size (), "reencoded", profile._videoContainerExtension));
    FFMPEGWriter writer;
    if ((rc = writer.createFile (outFilename, &tmp, &format, uiQuality, bConvertAudio, pAudio)) < 0) {
        fail (pszFuncName, rc);
        return -2;
    }
    for (unsigned int i = 0; i < images.size(); i++) {
        const RGBImage *pRGB = images[i].img.getImage();
        if ((rc = writer.encodeImage (*pRGB, images[i].uiFrameIdx)) < 0) {
            fail (pszFuncName, rc);
            return -3;
        }
    }
    if ((rc = writer.close()) < 0) {
        fail (pszFuncName, rc);
        return -4;
    }
    return 0;
}

int extractFrame (FFMPEGReader &reader, uint32 ui32FrameIdx, const String &out, Images &images)
{
    int rc;
    const char *pszFuncName = "extractFrame";
    const RGBImage *pRGB = reader.getFrameAtTime (ui32FrameIdx);
    if (pRGB == NULL) {
        return -1;
    }
    const unsigned int uiIdx = images.firstFree();
    images[uiIdx].img = *pRGB;
    pRGB = images[uiIdx].img.getImage();
    images[uiIdx].uiFrameIdx = ui32FrameIdx;
    FileWriter fw (out, "wb");
    if ((rc = images[uiIdx].img.writeHeaderAndImage (&fw)) < 0) {
        fail (pszFuncName, rc);
        return -2;
    }
    return 0;
}

int extractFrames (FFMPEGReader &reader, const String &filename, uint32 ui32FrameIdx,
                   const char *pszDescription, Images &images)
{
    for (unsigned int i = 1; i < 50; i++) {
        uint32 idx = ui32FrameIdx * i;
        const String out (getOutFileName (filename, idx, pszDescription, ".ppm"));
        if (extractFrame (reader, idx, out, images) < 0) {
            return -1;
        }
    }
    return 0;
}

int decodeFromBuf (const String &filename, uint32 ui32FrameIdx)
{
    File file (filename);
    const int64 i64 = file.getFileSize();
    if (i64 < 0) {
        return -1;
    }
    if (i64 > 0xFFFFFFFF) {
        return - 2;
    }
    const uint32 ui32BufLen = static_cast<uint32>(i64);
    if (ui32BufLen == 0) {
        return -3;
    }
    void *pBuf = malloc (ui32BufLen);
    if (pBuf == NULL) {
        return -4;
    }
    FileReader fr (filename, "rb");
    if (fr.readBytes (pBuf, ui32BufLen) < 0) {
        free (pBuf);
        return -5;
    }
    FFMPEGReader reader;
    if (reader.read (pBuf, ui32BufLen) < 0) {
        free (pBuf);
        return -6;
    }
    FFMPEGUtil::VideoFormat format (*reader.getVideoFormat());
    Images images;
    if (extractFrames (reader, filename, ui32FrameIdx, "from_buf", images) < 0) {
        free (pBuf);
        return -7;
    }
    free (pBuf);
    return 0;
}

int decodeFromFile (const String &filename, uint32 ui32FrameIdx)
{
    const char *pszFuncName = "decodeFromFile";
    int rc;
    FFMPEGReader reader;
    if ((rc = reader.openFile (filename)) < 0) {
        fail (pszFuncName, rc);
        return -1;
    }
    const FFMPEGUtil::VideoFormat *pFormat = reader.getVideoFormat();
    const FFMPEGUtil::VideoEncodingProfile *pProfile = reader.getVideoEncProfile();
    Images images;
    if ((rc = extractFrames (reader, filename, ui32FrameIdx, "from_file", images)) < 0) {
        fail (pszFuncName, rc);
        return -2;
    }
    if ((rc = encodeFile (filename, *pProfile, *pFormat, images)) < 0) {
        fail (pszFuncName, rc);
        return -3;
    }

    return 0;
}

int main (int argc, const char **ppszArgv)
{
    // Enable detailed logging
    pLogger = new Logger();
    pLogger->setDebugLevel (Logger::L_HighDetailDebug);
    pLogger->enableScreenOutput();

    // Parameter sanity check
    const String filename (ppszArgv[1]);
    const File file (filename);
    if (!file.exists()) {
        pLogger->logMsg ("main", Logger::L_SevereError, "file <%s> does not exist.\n", filename.c_str());
        return -1;
    }

    // Test
    int rc;
    const uint32 ui32FrameIdx = 1000;
    if ((rc = decodeFromFile (filename, ui32FrameIdx)) < 0) {
        fail ("main", rc);
        return -2;
    }
    /*if ((rc = decodeFromBuf (filename, ui32FrameIdx)) < 0) {
        fail ("main", rc);
        return -3;
    }*/
    return 0;
}

