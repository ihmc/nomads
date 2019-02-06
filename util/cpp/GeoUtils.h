/*
 * GeoUtils.h
 *
 * This file is part of the IHMC Util Library
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
 * Defines a set of functions and constants for geographical computation.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#ifndef INCL_GEO_UTILS_H
#define INCL_GEO_UTILS_H

namespace NOMADSUtil
{
    static const double EARTH_MEAN_RADIUS = 6371010.0;  // in meters
    static const float MAX_LATITUDE = 90.0f;
    static const float MIN_LATITUDE = -90.0f;
    static const float MAX_LONGITUDE = 180.0f;
    static const float MIN_LONGITUDE = -180.0f;
    static const float DISTANCE_UNSET = -1.0f;

    bool isValidLatitude (float fLatitude);
    bool isValidLongitude (float fLongitude);

    /**
     * Calculates whether the point (fPointLat, fPointLong) is contained
     * inside a coordinate box, identified by (fLeftUpperLatitude,
     * fLeftUpperLongitude, fRightLowerLatitude, fRightLowerLongitude)
     */
    bool pointContained (float fPointLat, float fPointLong,
                         float fLeftUpperLatitude, float fLeftUpperLongitude,
                         float fRightLowerLatitude, float fRightLowerLongitude);

    /**
     * Calculates whether the segment, identified by (fSegLatA, fSegLongA,
     * fSegLatB, fSegLongB) is contained inside a coordinate box,
     * identified by (fLeftUpperLatitude, fLeftUpperLongitude,
     * fRightLowerLatitude, fRightLowerLongitude)
     */
    bool segmentContained (float fSegLatA, float fSegLongA, float fSegLatB, float fSegLongB,
                           float fLeftUpperLatitude, float fLeftUpperLongitude,
                           float fRightLowerLatitude, float fRightLowerLongitude);

    /**
     * Calculates if the is an intersection between segment (latitude1,
     * longitude1), (latitude2, longitude2) and area identified by
     * (fLeftUpperLatitude, fLeftUpperLongitude, fRightLowerLatitude,
     * fRightLowerLongitude) which are the latitude and longitide of the
     * point C and E.
     *
     *        Segment           |             Area
     *                          |
     *              B           |      C                 D
     *            /             |       +---------------+
     *           /              |       |               |
     *          /               |       +---------------+
     *        A                 |      F                 E
     */
    bool segmentAreaIntersection (float fSegLatA, float fSegLongA,
                                  float fSegLatB, float fsegLongB,
                                  float fLeftUpperLatitude, float fLeftUpperLongitude,
                                  float fRightLowerLatitude, float fRightLowerLongitude);

    /**
     * Calculates if the is an intersection between segment (latitude1,
     * longitude1), (latitude2, longitude2) and segment (latitude3,
     * longitude3), (latitude4, longitude4).
     */
    bool segmentIntersection (float latitude1, float longitude1, float latitude2, float longitude2,
                              float latitude3, float longitude3, float latitude4, float longitude4);

    float minDistancePointArea (float fPointLat, float fPointLong,
                                float fLeftUpperLatitude, float fLeftUpperLongitude,
                                float fRightLowerLatitude, float fRightLowerLongitude);

    /**
     * Returns the minimum of the distances between segment AB (fSegLatA,
     * fSegLongA), (fSegLatB, fsegLongB) and the 4 corners of area CDEF
     * point C and E.
     * The CDEF area is identified only by the coordinate of the upper
     * left cornet (C) and the lower right corner (E).
     *
     * NOTE: returns a negative number in case of error!
     *
     *        Segment           |             Area
     *                          |
     *              B           |      C                 D
     *            /             |       +---------------+
     *           /              |       |               |
     *          /               |       +---------------+
     *        A                 |      F                 E
     */
    float minDistanceSegmentArea (float fSegLatA, float fSegLongA,
                                  float fSegLatB, float fsegLongB,
                                  float fLeftUpperLatitude, float fLeftUpperLongitude,
                                  float fRightLowerLatitude, float fRightLowerLongitude);

    /**
     * Returns the distance between segment (latitude1, longitude1),
     * (latitude2, longitude2) and point (latitude3, longitude3),
     * given that they are points on the earth surface.
     *
     * The second version returns the coordinates of the perpendicular
     * projection of the point (pointLat, pointLong) on the segment AB,
     * if this point is internal to AB. If the perpendicular projection
     * is on the backward or forward extension of AB, the coordinates
     * of the nearer ... are returned.
     *
     * NOTE: returns a negative number in case of error!
     */
    float distancePointSegment (float segLatA, float segLongA,
                                float segLatB, float segLongB,
                                float pointLat, float pointLong);
    float distancePointSegment (float segLatA, float segLongA,
                                float segLatB, float segLongB,
                                float pointLat, float pointLong,
                                float &projectionLat, float &projectionLong);

    /**
     * Return the distance (in meters) between two points on the earth
     * surface. The distance is calculated using the great circle
     * formula.
     *
     * Latitudes and longitudes are expressed in decimal degree.
     */
    float greatCircleDistance (float latitude1, float longitude1, float latitude2, float longitude2);

    /**
     * Returns the displacement in decimal degrees given the displacement
     * in meters and the latitude of the origin of the displacement.
     * The approximation can be significant for large distances (above
     * a few kilometers).
     */
    int metersToLatitudeDegrees (double dLat, double dDisplacementInMeters,
                                 double &dLatDegrees, double &dLonDegrees);

    /**
     * Compute a bounding-box of fPadding meters for the (fLat, fLong)
     * point.
     *
     * Latitudes and longitudes are expressed in decimal degrees.
     * fDistance is expressed in meters.
     */
    void getBoundingBox (float fLat, float fLong, float fDistance,
                         float &fMaxLat, float &fMinLat, float &fMaxLong, float &fMinLong);

    /**
     * Apply fPadding meters of padding to the area identified by
     * (fMaxLat, fMinLat, fMaxLong fMinLong)
     *
     * Latitudes and longitudes are expressed in decimal degrees.
     * fPadding is expressed in meters.
     */
    void addPaddingToBoudingBox (float fMaxLat, float fMinLat, float fMaxLong, float fMinLong, float fPadding,
                                 float &fNewMaxLat, float &fNewMinLat, float &fNewMaxLong, float &fNewMinLong);

    class Point
    {
        public:
            Point (const Point &point);
            Point (float fLatitude, float fLongitude);
            ~Point (void);

         public:
             const float _fLatitude;
             const float _fLongitude;
    };

    class BoundingBox
    {
        public:
            BoundingBox (void);
            BoundingBox (float leftUpperLatitude, float leftUpperLongitude,
                         float rightLowerLatitude, float rightLowerLongitude);
            BoundingBox (const BoundingBox &bbox);
            ~BoundingBox (void);

            float getArea (void) const;
            Point getBaricenter (void) const;
            float getHeigth (void) const;
            bool isValid (void) const;

            // Return the radius in meters
            float getRadius (void) const;

            float getWidth (void) const;

            // Create a squared bouding box centered in (fLatitude, float fLongitude) and with
            // each side of approximately 2 * fRange meters.
            static BoundingBox getBoundingBox (Point &baricenter, float fRange);
            static BoundingBox getBoundingBox (float fLatitude, float fLongitude, float fRange);

            BoundingBox getIntersection (const BoundingBox &rgBoundingBox) const;

            const bool _bEmpty;
            const float _leftUpperLatitude;
            const float _leftUpperLongitude;
            const float _rightLowerLatitude;
            const float _rightLowerLongitude;
    };
}

#endif    // INCL_GEO_UTILS_H
