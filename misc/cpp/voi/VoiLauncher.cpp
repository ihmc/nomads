#include "Voi.h"

#include "Comparator.h"
#include "MetadataImpl.h"
#include "MetaDataRanker.h"

#include "File.h"
#include "FileUtils.h"
#include "Json.h"
#include "StrClass.h"

#define checkIfFileExists(f) if (!FileUtils::fileExists (f)) return -1

using namespace NOMADSUtil;
using namespace IHMC_VOI;

namespace IHMC_VOI_LAUNCHER
{
    struct Compare
    {
        bool isSet (void)
        {
            return (_old.length () > 0) && (_new.length () > 0);
        }

        NOMADSUtil::String _old;
        NOMADSUtil::String _new;
    };

    struct Args
    {
        NOMADSUtil::String _sessionid;
        NOMADSUtil::String _metadata;
        NOMADSUtil::String _data;
        NOMADSUtil::String _nodeCtxt;
        Compare _compare;
    };

    String guessMetadataInternal (const String &data)
    {
        String guess;
        guess = data + ".dpmd";
        if (FileUtils::fileExists (guess)) {
            return guess;
        }
        guess = data + ".json";
        if (FileUtils::fileExists (guess)) {
            return guess;
        }
        return String ();
    }

    String guessMetadata (const String &data)
    {
        String guess (guessMetadataInternal (data));
        if (guess.length () <= 0) {
            File file (data);
            String baseFileName (file.getName (true));
            guess = guessMetadataInternal (baseFileName);
        }
        return guess;
    }

    bool isSet (const String &s)
    {
        return s.length () > 0;
    }

    int parse (int argc, char *argv[], Args &args)
    {
        for (int i = 0; i < argc; i++) {
            String opt (argv[i]);
            if ((opt ^= "-s") || (opt ^= "--session")) {
                checkIfFileExists (argv[++i]);
                args._sessionid = argv[i];
            }
            else if ((opt ^= "-m") || (opt ^= "--metadata")) {
                checkIfFileExists (argv[++i]);
                args._metadata = argv[i];
            }
            else if ((opt ^= "-d") || (opt ^= "--data")) {
                checkIfFileExists (argv[++i]);
                args._data = argv[i];
            }
            if ((opt ^= "-c") || (opt ^= "--context")) {
                checkIfFileExists (argv[++i]);
                args._nodeCtxt = argv[i];
            }
            if ((opt ^= "-n") || (opt ^= "--novelty")) {
                checkIfFileExists (argv[++i]);
                args._compare._old = argv[i];
                checkIfFileExists (argv[++i]);
                args._compare._new = argv[i];
            }
        }

        if (args._compare.isSet()) {
            return 0;
        }

        if (isSet (args._data)) {
            if (!isSet (args._metadata)) {
                // Try to guess metadata file
                args._metadata = guessMetadata (args._data);
            }
        }
        else if (!isSet (args._metadata)) {
            // Data and metadata are not set
            return -2;
        }

        if (!isSet (args._nodeCtxt)) {
            return -3;
        }

        return 0;
    }

    template<class T>
    T * parseFromJson (JsonObject &json)
    {
        T *pMD = new T ();
        if (pMD->fromJson (&json) < 0) {
            delete pMD;
            return NULL;
        }
        return pMD;
    }

    String readAsString (String filename)
    {
        int64 i64Size = 0;
        void *pString = FileUtils::readFile (filename, &i64Size);
        String s ((const char *)pString, (unsigned short)i64Size);
        free (pString);
        return s;
    }

    template<class T>
    T * parseMetadata (String filename)
    {
        const String sJson (readAsString (filename));
        JsonObject json (sJson);
        return parseFromJson<T> (json);
    }

    void print (Score *pScore)
    {
        // TODO: implement this
    }

    void printUsage (void)
    {
        // TODO: implement this
    }
}

namespace IHMC_VOI
{
    class NodeContextImpl : public NodeContext
    {
    public:
        NodeContextImpl (void) {};
        ~NodeContextImpl (void) {};

        int fromJson (const NOMADSUtil::JsonObject *pJson) { return 0; };
        NOMADSUtil::JsonObject * toJson (void) { return NULL; }
        NOMADSUtil::String getNodeId () const { return String (); };
        NOMADSUtil::String getTeamId () const { return String (); };
        NOMADSUtil::String getMissionId () const { return String (); };
        NOMADSUtil::String getRole () const { return String (); };
        unsigned getBatteryLevel () const { return 0; };
        unsigned getMemoryAvailable () const { return 0; };
        bool getLimitToLocalMatchmakingOnly () const { return false; };
        float getMatchmakingThreshold () const { return 0.0f; };
        IHMC_C45::Classifier* getClassifier () const { return NULL; };
        int getCurrentWayPointInPath () const { return 0; };
        NodeStatus getStatus () const { return ON_WAY_POINT; };
        float getClosestPointOnPathLatitude () const { return 0.0f; };
        float getClosestPointOnPathLongitude () const { return 0.0f; };
        int getCurrentLatitude (float& latitude) { return 0; };
        int getCurrentLongitude (float& longitude) { return 0; };
        int getCurrentTimestamp (uint64& timestamp) { return 0; };
        int getCurrentPosition (float& latitude, float& longitude, float& altitude, const char*& pszLocation, const char*& pszNote, uint64& timeStamp) { return 0; };
        uint32 getRangeOfInfluence (const char* pszNodeType) { return 0U; };
        MetadataRankerConfiguration* getMetaDataRankerConfiguration () const { return NULL; };
        NodePath* getPath () { return NULL; };
        AreaOfInterestList * getAreasOfInterest (void) const { return NULL; };
        AdjustedPathWrapper* getAdjustedPath () { return NULL; };
        PositionApproximationMode getPathAdjustingMode () const { return GO_TO_NEXT_WAY_POINT; };
        NOMADSUtil::PtrLList<CustomPolicy>* getCustomPolicies () { return NULL; };
        uint32 getMaximumUsefulDistance () { return 0U; };
        uint32 getUsefulDistance (const char* pszInformationMIMEType) const { return 0U; };
        uint32 getMaximumRangeOfInfluence () { return 0u; };
        bool hasUserId (const char* pszUserId) { return false; };
        bool isPeerActive () { return false; };
    };
}

using namespace IHMC_VOI_LAUNCHER;

void computeNovelty (const Args &args)
{
    String sOld = readAsString (args._compare._old);
    String sNew = readAsString (args._compare._new);

    float changePerc = COMPARATOR::compareLogstats (sOld, sNew) * 100;
    printf ("LogStat changed %f%%.\n", changePerc);
}

void computeVoi (const Args &args)
{
    MetadataInterface *pMetadata = parseMetadata<MetadataImpl> (args._metadata);
    NodeContext *pNodeCtxt = parseMetadata<NodeContextImpl> (args._nodeCtxt);

    Voi voi (args._sessionid);
    Score *pScore = voi.getVoi (pMetadata, pNodeCtxt);
    print (pScore);
    delete pScore;
}

int main (int argc, char *argv[])
{
    if (argc < 3) {
        printUsage ();
        return -1;
    }

    Args args;
    if (parse (argc, argv, args) < 0) {
        printUsage ();
        return -2;
    }

    if (args._compare.isSet()) {
        computeNovelty (args);
    }
    else {
        computeVoi (args);
    }

    return 0;
}

