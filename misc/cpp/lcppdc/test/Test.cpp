#include "Database.h"
#include "PreparedStatement.h"
#include "Result.h"

#include "Logger.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace IHMC_MISC;
using namespace NOMADSUtil;

void printBuffer (void *pBuf, int iLen)
{
    char *psBuf = (char *)pBuf;
    for (int i = 0; i < iLen; i++) {
        printf ("%c", psBuf[i]);
    }
    printf ("\n");
}

bool checkResult (Table *pTable, unsigned int uiExpectedNRows)
{
    Row *pRow = pTable->getRow();
    unsigned int uiNRows = 0;
    for (; pTable->next (pRow); uiNRows++) {
        char *pszString;
        int integer;
        uint64 ui64BigInteger;
        bool boolean;
        void *pBuf; int iLen = 0;

        pRow->getValue ((unsigned short) 0, &pszString);
        pRow->getValue ((unsigned short) 1, integer);
        pRow->getValue ((unsigned short) 2, ui64BigInteger);
        pRow->getValue ((unsigned short) 3, boolean);
        pRow->getValue ((unsigned short) 4, &pBuf, iLen);

        printf ("ROW: %s %d %llu %u\n", pszString, integer, ui64BigInteger, boolean);
        if (iLen > 0) {
            printBuffer (pBuf, iLen);
        }
    }

    assert (uiNRows == uiExpectedNRows);

    delete pRow;
    return true;
}

const char * CREATE_DB = "CREATE TABLE test ("
                              "test_string TEXT, "
                              "test_int INT, "
                              "test_int64 INT, "
                              "test_bool INT, "
                              "test_blob BLOB, "
                              "PRIMARY KEY (test_string)"
                          ")";

const char * INSERT_1 = "INSERT INTO test (test_string, test_int, test_int64, test_bool) "
                         "VALUES ('mystring', 1, 18446744073709551615, 0)";

const char * INSERT_2 = "INSERT INTO test (test_string, test_int, test_int64, test_bool, test_blob) "
                         "VALUES (?, ?, ?, ?, ?)";

const char * SELECT_ALL = "SELECT * FROM test";

bool testWithoutPreparedStatements (Database::Type type)
{
    /* Create DB */
    Database *pDB = Database::getDatabase (type);
    assert (pDB != NULL);
    assert (pDB->open (NULL) == 0);

    /* Create Table */
    assert (pDB->execute (CREATE_DB) == 0);

    /* Query Table */
    Table *pTable = pDB->getTable();
    assert (pTable != NULL);
    assert (pDB->execute (SELECT_ALL, pTable) == 0);
    checkResult (pTable, 0);
    delete pTable;

    /* Insert element */
    assert (pDB->execute (INSERT_1) == 0);

    /* Query Table */
    pTable = pDB->getTable();
    assert (pTable != NULL);
    assert (pDB->execute (SELECT_ALL, pTable) == 0);
    checkResult (pTable, 1);
    delete pTable;

    delete pDB;

    return true;
}

bool testWithPreparedStatements (Database::Type type)
{
    /* Create DB */
    Database *pDB = Database::getDatabase (type);
    assert (pDB != NULL);
    assert (pDB->open (NULL) == 0);

    /* Create Table */
    PreparedStatement *pPrepStmt = pDB->prepare (CREATE_DB);
    assert (pPrepStmt != NULL);
    pPrepStmt->update();
    delete pPrepStmt;

    /* Query Table */
    PreparedStatement *pQueryPrepStmt = pDB->prepare (SELECT_ALL);
    assert (pQueryPrepStmt != NULL);
    assert (checkResult (pQueryPrepStmt, 0));

    /* Insert element */
    pPrepStmt = pDB->prepare (INSERT_1);
    assert (pPrepStmt != NULL);
    pPrepStmt->update();
    delete pPrepStmt;

    /* Query Table */
    pQueryPrepStmt->reset();
    assert (checkResult (pQueryPrepStmt, 1));

    /* Insert element */
    pPrepStmt = pDB->prepare (INSERT_2);
    assert (pPrepStmt != NULL);
    assert (pPrepStmt->bind ((unsigned int) 1, "mystring2") == 0);
    int i = -23;
    assert (pPrepStmt->bind ((unsigned int) 2, i) == 0);
    uint64 ui64 = 18446744073709551615;
    printf ("\n\n%llu\n\n", ui64);
    assert (pPrepStmt->bind ((unsigned int) 3, ui64) == 0);
    assert (pPrepStmt->bind ((unsigned int) 4, true) == 0);
    void *pBuf = malloc (35);
    assert (pPrepStmt->bind ((unsigned int) 5, pBuf, 35) == 0);
    printBuffer (pBuf, (unsigned int) 35);
    pPrepStmt->update();
    delete pPrepStmt;

    /* Query Table */
    pQueryPrepStmt->reset();
    assert (checkResult (pQueryPrepStmt, 2));
    delete pQueryPrepStmt;

    delete pDB;

    return true;
}

bool test (Database::Type type)
{
    assert (testWithoutPreparedStatements (type));
    assert (testWithPreparedStatements (type));

    return true;
}

int main (int argc, char **ppArgs)
{
    if (!pLogger) {              
        pLogger = new Logger();
        pLogger->enableScreenOutput();
        pLogger->setDebugLevel(Logger::L_LowDetailDebug);
    }
    assert (test (Database::SQLite));

    return 0;
}


