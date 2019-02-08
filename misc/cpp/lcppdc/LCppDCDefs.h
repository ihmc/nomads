/*
 * LCppDCDefs.h
 *
 * This file is part of the IHMC Database Connectivity Library.
 * Copyright (c) 1993-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#ifndef INCL_LITTLE_CPP_DATABASE_CONNECTIVITY_DEFINITIIONS_H
#define INCL_LITTLE_CPP_DATABASE_CONNECTIVITY_DEFINITIIONS_H

namespace IHMC_MISC
{
    #define checkAndLogMsg if (pLogger) pLogger->logMsg
    #define memoryExhausted Logger::L_Warning, "Memory exhausted.\n"

    #define bindingError Logger::L_SevereError, "could not bind param for column %u (%s)\n"
    #define successfulSqlStmt Logger::L_LowDetailDebug, "the SQL statement run succesfully.\n [%s]\n"
    #define failureSqlStmt Logger::L_SevereError, "the SQL statement failed: %s\n [%s]\n%s\n"
    #define emptySqlStmt Logger::L_SevereError, "the SQL is null\n"

    #define columnIdxOutOfBound Logger::L_SevereError, "column index out of bound\n"

    #define TRUE_INT_VALUE 1
    #define FALSE_INT_VALUE 0
}

#endif  // INCL_LITTLE_CPP_DATABASE_CONNECTIVITY_DEFINITIIONS_H


