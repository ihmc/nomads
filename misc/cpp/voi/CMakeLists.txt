cmake_minimum_required(VERSION 3.6)
project(voi)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        InformationObject.cpp
        MetadataInterface.cpp
        MataDataRanker.cpp
        MetadataRankerConfiguration.cpp
        MetadataRankerLocalConfiguration.cpp
        NodeContext.cpp
        NodePath.cpp
        Rank.cpp
        Score.cpp
        Voi.cpp
        VoiLauncher.cpp
        core/Cache.cpp
        core/Comparator.cpp
        core/Match.cpp
        core/MatchMakingFilters.cpp
        core/MatchMakingPolicies.cpp
        core/MetadataImpl.cpp
        core/VoiImpl.cpp
        ctxt/AreaOfInterest.cpp
        ctxt/Path.cpp
        ctxt/Pedigree.cpp
        util/BoundingBox.cpp
        util/C45Utils.cpp
        util/NodeIdSet.cpp
        util/RankByTargetMap.cpp
        util/RankFactory.cpp)

add_executable(voi ${SOURCE_FILES})

set(EXEC voilauncher)
add_executable(${EXEC} ${SOURCE_FILES})