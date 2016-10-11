#include "VideoCodec.h"

using namespace IHMC_MISC;

bool VideoCodec::supports (Chunker::Type type)
{
    switch (type) {
        case Chunker::AVI:
        case Chunker::MOV:
        case Chunker::V_MPEG:
            return true;

        default:
            return false;
    }
}

