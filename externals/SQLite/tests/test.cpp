#include "sqlite3.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

sqlite3 *pDB;

int dbOpen (const char *pszFileName)
{
    return sqlite3_open (pszFileName, &pDB);
}

int prepare (const char *pszSql, sqlite3_stmt *&pPrepStmt)
{
    int rc = sqlite3_prepare_v2 (pDB, pszSql, strlen (pszSql), &pPrepStmt, NULL);
    if (rc < 0) {
        printf ("prepare() returned %d\n", rc);
    }
    return rc;
}

int bind (sqlite3_stmt *pPrepStmt, unsigned short usColumnIdx, void *pBuf, int iLen)
{
    if (pPrepStmt == NULL) {
        printf ("bind (blob) - empty sql statement\n");
        return -1; 
    }
    if (pBuf == NULL) {
        if (iLen > 0) {
            printf ("bind (blob) - mismatching buffer and buffer length\n");
        }
    }
    else if (iLen == 0) {
        printf ("bind (blob) - mismatching buffer and buffer length\n");
    }

    int rc = SQLITE_ERROR;
    if (iLen == 0) {
        rc = sqlite3_bind_null (pPrepStmt, (int) usColumnIdx);
    }
    else {
        rc = sqlite3_bind_blob (pPrepStmt, (int) usColumnIdx, pBuf,
                                iLen, SQLITE_STATIC);
    }

    if (rc != SQLITE_OK) {
        printf ("bind (blob) - binding error: %d\n", rc);
        return -usColumnIdx;
    }

    return 0;
}

int getValue (sqlite3_stmt *pPrepStmt,
              unsigned short usColumnIdx, void **ppBuf,
              int &iLen)
{
    iLen = 0;
    if (pPrepStmt == NULL) {
        printf ("getValue (blob) - empty slq statement\n");
        return -1;
    }

    iLen = sqlite3_column_bytes (pPrepStmt, usColumnIdx);
    if (iLen < 0) {
        iLen = 0;
        return -1;
    }
    if (iLen == 0) {
        ppBuf = NULL;
        iLen = 0;
        return -1;
    }

    *ppBuf = malloc (iLen);
    if (ppBuf == NULL) {
        printf ("getValue (blob) - memory exhausted\n");
        iLen = 0;
        return -2;
    }

    const void *pBlob = sqlite3_column_blob (pPrepStmt, usColumnIdx);
    if (pBlob == NULL) {
        free (*ppBuf);
        iLen = 0;
        return -3;
    }

    memcpy (*ppBuf, pBlob, iLen);
    return 0;
}

int createTable()
{
    const char *pszSql = "CREATE TABLE IF NOT EXISTS test (data BLOB);";
    char *pszErrMsg = NULL;
    int rc = sqlite3_exec (pDB, pszSql, NULL, NULL, &pszErrMsg);
    if (rc < 0) {
        printf ("createTable() returned %d %s\n",
                rc, (pszErrMsg != NULL ? pszErrMsg : ""));
    }
    return rc;
}

int addData (void *pBuf, int iLen)
{
    const char *pszSql = "INSERT INTO test VALUES (?1);";
    sqlite3_stmt *pPrepStmt = NULL;
    assert (prepare (pszSql, pPrepStmt) == 0);
    assert (bind (pPrepStmt, (unsigned short) 1, pBuf, iLen) == 0);
    int rc = sqlite3_step (pPrepStmt);
    assert (rc == SQLITE_OK || rc == SQLITE_DONE);
    sqlite3_finalize (pPrepStmt);

    return 0;
}

void * getData (int &iLen)
{
    const char *pszSql = "SELECT * FROM test;";
    sqlite3_stmt *pPrepStmt = NULL;
    assert (prepare (pszSql, pPrepStmt) == 0);
    int rc = sqlite3_step (pPrepStmt);
    assert (rc == SQLITE_ROW);
    void *pBuf = NULL;
    iLen = 0;
    assert (getValue (pPrepStmt, 0, &pBuf, iLen) == 0);
    sqlite3_reset (pPrepStmt);
    return pBuf;
}

bool beginTransaction()
{
    char *pszErrMsg = NULL;
    int rc = sqlite3_exec (pDB, "BEGIN;", NULL, NULL, &pszErrMsg);
    if (rc != 0)
        printf ("Error code: %d - %s\n", rc, pszErrMsg != NULL ? pszErrMsg : "");
    return rc == 0;
}

bool endTransaction()
{
    char *pszErrMsg = NULL;
    int rc = sqlite3_exec (pDB, "COMMIT;", NULL, NULL, &pszErrMsg);
    if (rc != 0)
        printf ("Error code: %d - %s\n", rc, pszErrMsg != NULL ? pszErrMsg  : "");
    return rc == 0;
}

int main (int argc, char**ppszArgv)
{
    int iLen = 50000000;
    char *pszFileName = ":memory:";  // Op the database in memory by default
    if (argc > 1) {
        pszFileName = ppszArgv[1];
    }
    printf ("Opening database in %s\n", pszFileName);

    char *pBuf = (char*) malloc (iLen);
    memset (pBuf, 'a', iLen);

    assert (dbOpen (pszFileName) == 0);
    assert (createTable() == 0);
    assert (addData (pBuf, iLen) == 0);

    // Check if the cache is persistent
    memset (pBuf, 'b', iLen);
    free (pBuf);
    char *pUselessBuf = (char*) malloc (iLen);
    memset (pUselessBuf, 'c', iLen);
    pBuf = (char*) malloc (iLen);
    memset (pBuf, 'a', iLen);
    free (pUselessBuf);

    int iRetLen = 0;
    assert (beginTransaction());
    char *pRetBuf = (char*) getData (iRetLen);
    assert (endTransaction());

    assert (iLen == iRetLen);
    int rc = 0;
    for (unsigned int i = 0; i < iLen; i++) {
        if (pBuf[i] != pRetBuf[i]) {
            printf ("Chars at index %d differ: %c %c\n", i, pBuf[i], pRetBuf[i]);
            rc = -1;
        }
    }
    return rc;
}

