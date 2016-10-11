#include "NetworkMessageServiceProxyServer.h"

#include "NMSCommandProcessor.h"
#include "NMSProperties.h"

#include "ConfigManager.h"
#include "FileUtils.h"
#include "StringTokenizer.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

/*
int NMSProxyShell::processCmd (const void *pToken, char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    const char *pszCmd = st.getNextToken();
    if (pszCmd == NULL) {
        // Should not happen, but return 0 anyways
        return 0;
    }
    else if (0 == stricmp (pszCmd, "load")) {
        StringTokenizer st (pszCmdLine);
        st.getNextToken();  // This is the command itself - discard
        const char *pszBatchFileName = st.getNextToken();
        FILE *pBatchFile = fopen (pszBatchFileName, "r");
        if (pBatchFile == NULL) {
            print (pToken, "can't load file <%s>\n", pszBatchFileName);
            return -2;
        }
        FileReader fr (pBatchFile, true);
        LineOrientedReader lr (&fr, false);
        char buf[1024];
        int rc = 0;
        while ((rc = lr.readLine (buf, 1024)) >= 0) {
            String line (buf, rc);
            if (line.startsWith ("#") == 0) {
                // Not a comment
                if ((rc = processCmd (pToken, line)) != 0) {
                    return rc;
                }
            }
        }
    }
    else if (0 == stricmp (pszCmd, "repeat")) {
        StringTokenizer st (pszCmdLine);
        st.getNextToken();  // This is the command itself - discard
        const uint32 ui32NCommand = atoui32 (st.getNextToken());
        String cmdLine;
        for (const char *pszCurrToken = st.getNextToken(); pszCurrToken != NULL; pszCurrToken = st.getNextToken()) {
            cmdLine += pszCurrToken;
            cmdLine += " ";
        }
        cmdLine.trim();
        char *pszInnerCmdLine = cmdLine.r_str();
        if (pszInnerCmdLine != NULL) {
            for (uint32 i = 0; i < ui32NCommand; i++) {
                processCmd (pToken, pszInnerCmdLine);
            }
            free (pszInnerCmdLine);
        }
    }
    else if (0 == stricmp (pszCmd, "help")) {
        const char *pszHelpCmd = st.getNextToken();
        if (pszHelpCmd == NULL) {
            displayGeneralHelpMsg (pToken);
        }
        else {
            displayHelpMsgForCmd (pToken, pszHelpCmd);
        }
    }
    else if (0 == stricmp (pszCmd, "makeAvailable")) {
        handleMakeAvailableCmd (pToken, pszCmdLine);
    }
}
*/

int main (int argc, const char **ppszArgv)
{
    if (!pLogger) {
        pLogger = new Logger();
        pLogger->enableScreenOutput();
        pLogger->initLogFile ("networkmessage.log", false);
        pLogger->enableFileOutput();
        pLogger->setDebugLevel (Logger::L_Info);
    }

    if (argc > 2) {
        printf ("Usage: %s <configFile>\n", ppszArgv[0]);
        return -1;
    }

    ConfigManager cfgMgr;
    cfgMgr.init();
    if (argc == 2) {
        if (!FileUtils::fileExists (ppszArgv[1])) {
            printf ("Could not find the specified configuration file: %s.\n", ppszArgv[1]);
            return -2;
        }
        if (cfgMgr.readConfigFile (ppszArgv[1], true) < 0) {
            printf ("Could not parse the specified configuration file: %s.\n", ppszArgv[1]);
            return -3;
        }
    }

    NetworkMessageServiceProxyServer *pProxySrv = NetworkMessageService::getProxySvrInstance (&cfgMgr);
    if (pProxySrv == NULL) {
        checkAndLogMsg ("main", Logger::L_SevereError, "could not instantiate NetworkMessageServiceProxyServer correctly.\n");
        return -4;
    }

    checkAndLogMsg ("main", Logger::L_Info, "starting proxy server.\n");
    NetworkMessageService *pNMS = pProxySrv->getSvcInstace();
    if (pNMS == NULL) {
        checkAndLogMsg ("main", Logger::L_SevereError, "NetworkMessageServiceProxyServer was not correctly configured: the NetworkMessageService is null.\n");
        return -5;
    }
    NMSCommandProcessor *pCmdProc = pNMS->getCmdProcessor();
    if (pCmdProc == NULL) {
        checkAndLogMsg ("main", Logger::L_SevereError, "NetworkMessageService was not correctly configured: the NMSCommandProcessor is null.\n");
        return -6;
    }
    pNMS->start();
    pProxySrv->start();
    pCmdProc->setPrompt ("NMS");
    pCmdProc->run();

    return 0;
}

