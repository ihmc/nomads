#include "DataGenerator.h"

#include "C45AVList.h"

#include "Logger.h"
#include "StrClass.h"

#include <sys/stat.h>
#include <stdlib.h>

using namespace IHMC_C45;
using namespace NOMADSUtil;

bool fileExists (const char *pszPath);

int main (int argc, char *argv[])
{
    if(!pLogger){
        pLogger = new Logger();
        pLogger->enableScreenOutput();
        //pLogger->initLogFile("generateXmlDataset.log",false);
        //pLogger->enableFileOutput();
        pLogger->setDebugLevel(Logger::L_LowDetailDebug);
    }
    // Read Configuration
    if (argc > 4) {
        printf ("Usage: ./GenerateXmlDataset <initFile> <dataset length> <dataset file name>\n");
        exit (1);
    }
    DataGenerator *pDataGenerator;
    int datasetLength, retValue;
    if (argv[1]) {
        if (fileExists(argv[1])) {
            pDataGenerator = new DataGenerator();
            retValue = pDataGenerator->initializeAttributes((const char *) argv[1]);
            if (retValue < 0) {
                printf ("Error in reading init file <%s>. returned code %d", (const char *) argv[1], retValue);
                exit (1);
            }
        }
        else {
            printf ("The file at location: %s does not exist!", (const char *) argv[1]);
            exit (1);
        }
    }
    if (argv[2]) {
        datasetLength = atoi((const char *) argv[2]);
    }
    else {
        printf ("Usage: ./GenerateXmlDataset <initFile> <dataset length>\n");
        exit (1);
    }
    // Initialize the DataGenerator
    IHMC_C45::C45AVList **rules = new IHMC_C45::C45AVList * [4];
    IHMC_C45::C45AVList *ranges = new IHMC_C45::C45AVList(4);
    // initialize rules
    rules[0] = new IHMC_C45::C45AVList(4);
    rules[0]->addPair(rules[0]->_CLASS.c_str(), "Useful");
    rules[0]->addPair("Classification", "Secret");
    rules[0]->addPair("Data_Format", "TXT");
    rules[0]->addPair("Data_Format", "XML");
        
    rules[1] = new IHMC_C45::C45AVList(3);
    rules[1]->addPair(rules[1]->_CLASS, "Useful");
    rules[1]->addPair("Data_Content", "Map");
    rules[1]->addPair("Classification", "Unclassified");
        
    rules[2] = new IHMC_C45::C45AVList(4);
    rules[2]->addPair(rules[2]->_CLASS, "Useful");
    rules[2]->addPair("Classification", "FOUO");
    rules[2]->addPair("Classification", "ITAR");
    rules[2]->addPair("Data_Content", "Sensor_Data_Update");
    rules[2]->addPair("Data_Format", "JPG");
        
    rules[3] = new IHMC_C45::C45AVList(1);
    rules[3]->addPair(rules[4]->_DEFAULT_CLASS, "Not_Useful");
    //initialize ranges
    ranges->addPair("Left_Upper_Latitude", "1.0, 2.0");
    ranges->addPair("Left_Upper_Longitude", "50.0, 51.0");
    ranges->addPair("Right_Lower_Latitude", "3.0, 4.0");
    ranges->addPair("Right_Lower_Longitude", "52.0, 53.0");
    retValue = pDataGenerator->configureGenerator(rules, 4, ranges, 0.05);
    if (retValue < 0) {
        printf ("Error in reading the rules. returned code %d", retValue);
        exit (1);
    }
    // Generate the dataset
    retValue = pDataGenerator->generateDataset(datasetLength, (const char *) argv[3]);
    if (retValue < 0) {
        printf ("Error in generating the dataset. returned code %d", retValue);
        exit (1);
    }
    printf("process finished\n");
}

bool fileExists (const char *pszPath)
{
    struct stat statinfo;
    if ((stat (pszPath, &statinfo) == -1) || ((statinfo.st_mode & S_IFMT) != S_IFREG)) {
        return false;
    }

    return true;
}


