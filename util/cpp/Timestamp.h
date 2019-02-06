/*
 * File:   Timestamp.h
 * Author: gbenincasa
 *
 * Created on April 28, 2015, 5:03 PM
 */

#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <time.h>

#include "StrClass.h"

namespace NOMADSUtil
{
    class Timestamp
    {
        public:
            /**
             * Set the time to the current time.
             */
            Timestamp (void);

            /**
             * Set to the specified timestamp in milliseconds.
             */
            Timestamp (int64 i64TimestampInMillis);
            virtual ~Timestamp (void);

            String toString (void);

        private:
            time_t _time;
    };
}

#endif    /* TIMESTAMP_H */

