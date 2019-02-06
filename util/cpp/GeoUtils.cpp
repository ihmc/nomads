/*
 * GeoUtils.cpp
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
 */

#include "GeoUtils.h"

#if defined (WIN32)
    #define _USE_MATH_DEFINES    // For M_PI - see math.h
#endif

#include "Logger.h"
#include "NLFLib.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define invalidCoordinates Logger::L_Warning, "Invalid coordinates. %f %f %f %f %f %f %f %f\n"

using namespace NOMADSUtil;

static const float FLOAT_EQUALITY_TOLERANCE = 0.000001f;

bool NOMADSUtil::isValidLatitude (float fLatitude)
{
    if (fLatitude < MIN_LATITUDE) {
        return false;
    }
    if (fLatitude > MAX_LATITUDE) {
        return false;
    }
    return true;
}

bool NOMADSUtil::isValidLongitude (float fLongitude)
{
    if (fLongitude < MIN_LONGITUDE) {
        return false;
    }
    if (fLongitude > MAX_LONGITUDE) {
        return false;
    }
    return true;
}

bool NOMADSUtil::pointContained (float fPointLat, float fPointLong,
                                 float fLeftUpperLatitude, float fLeftUpperLongitude,
                                 float fRightLowerLatitude, float fRightLowerLongitude)
{
    if ((fPointLat < MIN_LATITUDE) || (fPointLat > MAX_LATITUDE) ||
        (fPointLong < MIN_LONGITUDE) || (fPointLong > MAX_LONGITUDE) ||
        (fLeftUpperLatitude < MIN_LATITUDE) || (fLeftUpperLatitude > MAX_LATITUDE) ||
        (fLeftUpperLongitude < MIN_LONGITUDE) || (fLeftUpperLongitude > MAX_LONGITUDE) ||
        (fRightLowerLatitude < MIN_LATITUDE) || (fRightLowerLatitude > MAX_LATITUDE) ||
        (fRightLowerLongitude < MIN_LONGITUDE) || (fRightLowerLongitude > MAX_LONGITUDE)) {
        checkAndLogMsg ("MathUtils::pointContained", invalidCoordinates,
                        fPointLat, fPointLong, fLeftUpperLatitude, fLeftUpperLongitude,
                        fRightLowerLatitude, fRightLowerLongitude, 0.0f, 0.0f);
        return false;
    }
    if (fLeftUpperLatitude < fRightLowerLatitude) {
        checkAndLogMsg ("MathUtils::pointContained", Logger::L_Warning,
                        "Left-Upper Latitude is less than Right-Lower Latitude - inverting\n");
        float fTemp = fLeftUpperLatitude;
        fLeftUpperLatitude = fRightLowerLatitude;
        fRightLowerLatitude = fTemp;
    }
    if (fLeftUpperLongitude > fRightLowerLongitude) {
        checkAndLogMsg ("MathUtils::pointContained", Logger::L_Warning,
                        "Left-Upper Longitude is greater than Right-Lower Longitude - inverting\n");
        float fTemp = fRightLowerLongitude;
        fRightLowerLongitude = fLeftUpperLongitude;
        fLeftUpperLongitude = fTemp;
    }
    if (fPointLat > fLeftUpperLatitude) {
        // One of the points exceeds the top of the bounding box
        return false;
    }
    if (fPointLat < fRightLowerLatitude) {
        // One of the points exceeds the bottom of the bounding box
        return false;
    }
    if (fPointLong < fLeftUpperLongitude) {
        // One of the points exceeds the left of the bounding box
        return false;
    }
    if (fPointLong > fRightLowerLongitude) {
        // One of the points exceeds the right of the bounding box
        return false;
    }
    return true;
}

bool NOMADSUtil::segmentContained (float fSegLatA, float fSegLongA, float fSegLatB, float fSegLongB,
                                   float fLeftUpperLatitude, float fLeftUpperLongitude,
                                   float fRightLowerLatitude, float fRightLowerLongitude)
{

    return (pointContained (fSegLatA, fSegLongA,
                            fLeftUpperLatitude, fLeftUpperLongitude,
                            fRightLowerLatitude, fRightLowerLongitude)
                                  &&
            pointContained (fSegLatB, fSegLongB,
                            fLeftUpperLatitude, fLeftUpperLongitude,
                            fRightLowerLatitude, fRightLowerLongitude));
}

bool NOMADSUtil::segmentAreaIntersection (float fSegLatA, float fSegLongA,
                                          float fSegLatB, float fsegLongB,
                                          float fLeftUpperLatitude, float fLeftUpperLongitude,
                                          float fRightLowerLatitude, float fRightLowerLongitude)
{
    if (// Check if the path segment intersects with the upper side
        segmentIntersection (fSegLatA, fSegLongA, fSegLatB, fsegLongB,
                             fLeftUpperLatitude, fLeftUpperLongitude,
                             fRightLowerLatitude, fLeftUpperLongitude) ||
        // Check if the path segment intersects with the right side
        segmentIntersection (fSegLatA, fSegLongA, fSegLatB, fsegLongB,
                             fRightLowerLatitude, fLeftUpperLongitude,
                             fRightLowerLatitude, fRightLowerLongitude) ||
        // Check if the path segment intersects with the bottom side
        segmentIntersection (fSegLatA, fSegLongA, fSegLatB, fsegLongB,
                             fRightLowerLatitude, fRightLowerLongitude,
                             fLeftUpperLatitude, fRightLowerLongitude) ||
        // Check if the path segment intersects with the left side
        segmentIntersection (fSegLatA, fSegLongA, fSegLatB, fsegLongB,
                             fLeftUpperLatitude, fRightLowerLongitude,
                             fLeftUpperLatitude, fLeftUpperLongitude)) {
        return true;
    }
    return false;
}

bool NOMADSUtil::segmentIntersection (float fSegALat1, float fSegALong1, float fSegALat2, float fSegALong2,
                                      float fSegBLat1, float fSegBLong1, float fSegBLat2, float fSegBLong2)
{
    if ((fSegALat1 < MIN_LATITUDE) || (fSegALat1 > MAX_LATITUDE) ||
        (fSegALong1 < MIN_LONGITUDE) || (fSegALong1 > MAX_LONGITUDE) ||
        (fSegALat2 < MIN_LATITUDE) || (fSegALat2 > MAX_LATITUDE) ||
        (fSegALong2 < MIN_LONGITUDE) || (fSegALong2 > MAX_LONGITUDE) ||
        (fSegBLat1 < MIN_LATITUDE) || (fSegBLat1 > MAX_LATITUDE) ||
        (fSegBLong1 < MIN_LONGITUDE) || (fSegBLong1 > MAX_LONGITUDE) ||
        (fSegBLat2 < MIN_LATITUDE) || (fSegBLat2 > MAX_LATITUDE) ||
        (fSegBLong2 < MIN_LONGITUDE) || (fSegBLong2 > MAX_LONGITUDE)) {
        checkAndLogMsg ("MathUtils::segmentIntersection", invalidCoordinates,
                        fSegALat1, fSegALong1, fSegALat2, fSegALong2,
                        fSegBLat1, fSegBLong1, fSegBLat2, fSegBLong2);
        return false;
    }

    /**
     * Hp:  Pa is a point that lies on the line that contains the segment P1 P2
     *      Pb is a point that lien on the line that contains the segment P3 P4
     *
     * If we want to find the intersection between the lines AB and CD, we need
     * to find the point Px that lies on both, in other word we are looking for
     * a point Px for which    Pa = Pb
     *
     * Pa and Pb can be described by the following equations:
     *  Pa = P1 + ua (P2-P1)
     *  Pb = P3 + ub (P4-P3)
     *
     * ua and ub are the fractional values you can multiple the x and y legs of
     * the triangle formed by each line to find a point on the line.
     *
     * The two equations can be expanded to their x/y components:
     *  Pa.x = p1.x + ua(p2.x - p1.x)
     *  Pa.y = p1.y + ua(p2.y - p1.y)
     *
     *  Pb.x = p3.x + ub(p4.x - p3.x)
     *  Pb.y = p3.y + ub(p4.y - p3.y)
     *
     * When Pa.x == Pb.x and Pa.y == Pb.y the lines intersect so you can come
     * up with two equations (one for x and one for y):
     *
     * p1.x + ua(p2.x - p1.x) = p3.x + ub(p4.x - p3.x)
     * p1.y + ua(p2.y - p1.y) = p3.y + ub(p4.y - p3.y)
     *
     * ua and ub can then be individually solved for.  This results in the
     *  equations used in the following code.
     */

    float ua = (fSegBLong2 - fSegBLong1) * (fSegALat1 - fSegBLat1) -
               (fSegBLat2 - fSegBLat1) * (fSegALong1 - fSegBLong1);
    float ub = (fSegALong2 - fSegALong1) * (fSegALat1 - fSegBLat1) -
               (fSegALat2 - fSegALat1) * (fSegALong1 - fSegBLong1);
    float fDen = (fSegBLat2 - fSegBLat1) * (fSegALong2 - fSegALong1) -
                 (fSegBLong2 - fSegBLong1) * (fSegALat2 - fSegALat1);

    if (fEquals (fDen, 0.0f, FLOAT_EQUALITY_TOLERANCE)) {
        if (fEquals (ua, 0.0f, FLOAT_EQUALITY_TOLERANCE) &&
                fEquals (ub, 0.0f, FLOAT_EQUALITY_TOLERANCE)) {
            // The lines are coincident
            if (pointContained (fSegALat1, fSegALong1, fSegBLat1, fSegBLong1, fSegBLat2, fSegBLong2) ||
                pointContained (fSegALat2, fSegALong2, fSegBLat1, fSegBLong1, fSegBLat2, fSegBLong2)) {
                // we know that the point lies on the lines that contain the segments
                // (because the two lines are coincident).  Therefore it is only
                // necessary to check whether the two segments overlap.
                //
                // This test also handles the cases in which both segments degenerated
                // to points (the containment test returns false and so does this method)
                return true;
            }
            // The two segments are disjointed
            return false;
        }
        // The lines containing the segments are parallel, so the two segments
        // can not intersect
        return false;
    }

    ua = ua / fDen;
    ub = ub / fDen;
    if ((ua >= 0.0f) && (ua <= 1.0f) && (ub >= 0.0f) && (ub <= 1.0f)) {
        return true;
    }
    return false;
}

float NOMADSUtil::minDistancePointArea (float fPointLat, float fPointLong,
                                        float fLeftUpperLatitude, float fLeftUpperLongitude,
                                        float fRightLowerLatitude, float fRightLowerLongitude)
{
    // C                  D
    //   +---------------+
    //   |               |
    //   +---------------+
    // F                  E

    // Compute CD - P distance
    float fMin = distancePointSegment (fLeftUpperLatitude, fLeftUpperLongitude,
                                       fLeftUpperLatitude, fRightLowerLongitude,
                                       fPointLat, fPointLong);

    // Compute DE - P distance
    float fTmpMin = distancePointSegment (fLeftUpperLatitude, fRightLowerLongitude,
                                          fRightLowerLatitude, fRightLowerLongitude,
                                          fPointLat, fPointLong);
    if ((fMin > fTmpMin) || fMin < 0.0) {
        fMin = fTmpMin;
    }

    // Compute EF - P distance
    fTmpMin = distancePointSegment (fRightLowerLatitude, fRightLowerLongitude,
                                    fRightLowerLatitude, fLeftUpperLongitude,
                                    fPointLat, fPointLong);
    if ((fMin > fTmpMin) || fMin < 0.0) {
        fMin = fTmpMin;
    }

    // Compute FC - P distance
    fTmpMin = distancePointSegment (fRightLowerLatitude, fLeftUpperLongitude,
                                    fLeftUpperLatitude, fLeftUpperLongitude,
                                    fPointLat, fPointLong);
    if ((fMin > fTmpMin) || fMin < 0.0) {
        fMin = fTmpMin;
    }

    return ((fTmpMin < fMin) || fMin < 0.0 ? fTmpMin : fMin);
}

float NOMADSUtil::minDistanceSegmentArea (float fSegLatA, float fSegLongA,
                                          float fSegLatB, float fsegLongB,
                                          float fLeftUpperLatitude, float fLeftUpperLongitude,
                                          float fRightLowerLatitude, float fRightLowerLongitude)
{
    // C                  D
    //   +---------------+
    //   |               |
    //   +---------------+
    // F                  E

    // Compute distance to C
    float fMin, fTmpMin;
    fMin = distancePointSegment (fSegLatA, fSegLongA, fSegLatB, fsegLongB,
                                 fLeftUpperLatitude, fLeftUpperLongitude);
    // Compute distance to F
    fTmpMin = distancePointSegment (fSegLatA, fSegLongA, fSegLatB, fsegLongB,
                                    fRightLowerLatitude, fLeftUpperLongitude);
    if ((fTmpMin < fMin) || fMin < 0.0f) {
        fMin = fTmpMin;
    }
    // Compute distance to E
    fTmpMin = distancePointSegment (fSegLatA, fSegLongA, fSegLatB, fsegLongB,
                                    fRightLowerLatitude, fRightLowerLongitude);
    if ((fTmpMin < fMin) || fMin < 0.0f) {
        fMin = fTmpMin;
    }
    // Compute distance to D
    fTmpMin = distancePointSegment (fSegLatA, fSegLongA, fSegLatB, fsegLongB,
                                    fLeftUpperLatitude, fRightLowerLongitude);
    return ((fTmpMin < fMin) || fMin < 0.0f ? fTmpMin : fMin);
}

float NOMADSUtil::distancePointSegment (float segLatA, float segLongA,
                                        float segLatB, float segLongB,
                                        float pointLat, float pointLong)
{
    float projectionLat, projectionLong;
    return distancePointSegment (segLatA, segLongA, segLatB, segLongB,
                                 pointLat, pointLong,
                                 projectionLat, projectionLong);
}

float NOMADSUtil::distancePointSegment (float segLatA, float segLongA,
                                        float segLatB, float segLongB,
                                        float pointLat, float pointLong,
                                        float &projectionLat, float &projectionLong)
{
    if ((segLatA < MIN_LATITUDE) || (segLatA > MAX_LATITUDE) ||
        (segLongA < MIN_LONGITUDE) || (segLongA > MAX_LONGITUDE) ||
        (segLatB < MIN_LATITUDE) || (segLatB > MAX_LATITUDE) ||
        (segLongB < MIN_LONGITUDE) || (segLongB > MAX_LONGITUDE) ||
        (pointLat < MIN_LATITUDE) || (pointLat > MAX_LATITUDE) ||
        (pointLong < MIN_LONGITUDE) || (pointLong > MAX_LONGITUDE)) {
        checkAndLogMsg ("MathUtils::distancePointSegment", invalidCoordinates,
                        segLatA, segLongA, segLatB, segLongB,
                        pointLat, pointLong, 0.0f, 0.0f);
        return DISTANCE_UNSET;
    }

    if (fEquals (segLatA, segLatB, FLOAT_EQUALITY_TOLERANCE) &&
            fEquals (segLongA, segLongB, FLOAT_EQUALITY_TOLERANCE)) {
        // This, also avoids division by 0 in the code that follows.
        projectionLat = segLatA;
        projectionLong = segLongA;
        return greatCircleDistance (segLatA, segLongA, pointLat, pointLong);
    }

    // find the distance from the point (pointLong,pointLat) to the line
    // determined by the points (segLongA,segLatA) and (segLongB,segLatB).
    //
    // distanceSegment = distance from the point to the line segment
    //
    // Point-Line Distance:
    //
    // Let the point be C (pointLong, pointLat) and the line be AB
    // (segLongA, segLatA) to (segLongB, segLatB).
    // Let P be the point of perpendicular projection of C on AB.  The parameter
    // r, which indicates P's position along AB, is computed segLatB the dot
    // product of AC and AB divided segLatB the square of the length of AB:
    //
    // (1)    AC dot AB
    //    r = ---------
    //        ||AB||^2
    //
    // r has the following meaning:
    //    - r=0      P = A
    //    - r=1      P = B
    //    - r<0      P is on the backward extension of AB
    //    - r>1      P is on the forward extension of AB
    //    - 0<r<1    P is interior to AB
    //
    // The length of a line segment in d dimensions, AB is computed segLatB:
    //
    // L = sqrt( (segLongB-segLongA)^2 + (segLatB-segLatA)^2 + ... + (Bd-Ad)^2)
    // so in 2D:
    //
    // L = sqrt( (segLongB-segLongA)^2 + (segLatB-segLatA)^2 )
    //
    // and the dot product of two vectors in d dimensions, U dot V is computed:
    //
    // D = (Ux * Vx) + (Uy * Vy) + ... + (Ud * Vd)
    //
    // so in 2D:
    //
    // D = (Ux * Vx) + (Uy * Vy)
    //
    // So (1) expands to:
    //
    //    (pointLong-segLongA)(segLongB-segLongA) + (pointLat-segLatA)(segLatB-segLatA)
    //   r = -------------------------------
    //                  L^2
    //
    // The point P can then be found:
    //
    // Px = segLongA + r(segLongB-segLongA)
    // Py = segLatA + r(segLatB-segLatA)
    //
    // And the distance from A to P = r*L.
    //
    // Use another parameter s to indicate the location along PC, with the
    // following meaning:
    //  - s<0      C is left of AB
    //  - s>0      C is right of AB
    //  - s=0      C is on AB
    //
    // Compute s as follows:
    //
    //    (segLatA-pointLat)(segLongB-segLongA)-(segLongA-pointLong)(segLatB-segLatA)
    // s = -----------------------------
    //                L^2
    //
    // Then the distance from C to P = |s|*L.

    float deltaLongCA = pointLong - segLongA;
    float deltaLatCA = pointLat - segLatA;
    float deltaLongBA = segLongB - segLongA;
    float deltaLatBA = segLatB - segLatA;

    double r_numerator = (deltaLongCA) * (deltaLongBA) +
                         (deltaLatCA) * (deltaLatBA);
    double r_denomenator = pow (deltaLongBA, 2) + pow (deltaLatBA, 2);

    double r = r_numerator / r_denomenator;

    if (r < 0.0f) {
        // P in the backward extension of A
        projectionLat = segLatA;
        projectionLong = segLongA;
        return greatCircleDistance (segLatA, segLongA, pointLat, pointLong);
    }
    if (r > 1.0f) {
        // P in the forward extension of B
        projectionLat = segLatB;
        projectionLong = segLongB;
        if (r == 1) {
            return 1;
        }
        return greatCircleDistance (segLatB, segLongB, pointLat, pointLong);
    }
    if (fEquals ((float)r, 0.0f, FLOAT_EQUALITY_TOLERANCE)) {
        // P == A
        projectionLat = segLatA;
        projectionLong = segLongA;
        return 0;
    }
    if (fEquals ((float)r , 1.0f, FLOAT_EQUALITY_TOLERANCE)) {
        // P == B
        projectionLat = segLatB;
        projectionLong = segLongB;
        return 0;
    }

    // P is interior to AB
    projectionLat = (float) (segLatA + r * deltaLatBA);
    projectionLong = (float) (segLongA + r * deltaLongBA);

    return greatCircleDistance (projectionLat, projectionLong, pointLat, pointLong);
}

float NOMADSUtil::greatCircleDistance (float latitude1, float longitude1,
                                       float latitude2, float longitude2)
{
    // The great-circle distance is the shortest distance between any two points
    // on the surface of a sphere measured along a path on the surface of the
    // sphere (as opposed to going through the sphere's interior).
    // The formula, for coordinates EXPRESSED IN RADIANS is as follow
    // dist = arccos(sin(lat1) · sin(lat2) +
    //        cos(lat1) · cos(lat2) · cos(lon1 - lon2)) · R

    double latitude1rad = degToRad (latitude1);
    double latitude2rad = degToRad (latitude2);
    double dDeltaLongRad = degToRad (longitude1) - degToRad (longitude2);
    if (dDeltaLongRad < 0.0) {
        dDeltaLongRad = - dDeltaLongRad;
    }
    if (dDeltaLongRad > M_PI) {
        dDeltaLongRad = 2.0 * M_PI - dDeltaLongRad;
    }
    double num = sqrt (pow (cos (latitude2rad) * sin (dDeltaLongRad), 2) +
                       pow (cos (latitude1rad) * sin (latitude2rad) -
                       sin (latitude1rad) * cos (latitude2rad) * cos (dDeltaLongRad), 2));
    double den = sin (latitude1rad) * sin (latitude2rad) +
                 cos (latitude1rad) * cos (latitude2rad) * cos (dDeltaLongRad);
    double sad = atan2 (num, den);
    num = sad * EARTH_MEAN_RADIUS;
    return (float) num;
}

int NOMADSUtil::metersToLatitudeDegrees (double dLat, double dDisplacementInMeters,
                                         double &dLatDegrees, double &dLonDegrees)
{
    static const double R = 111111;
    dLatDegrees = dDisplacementInMeters / R;
    dLonDegrees = fabs (dDisplacementInMeters * cos (dLat) / R);
    return 0;
}

void NOMADSUtil::getBoundingBox (float fLat, float fLong, float fDistance,
                                 float &fMaxLat, float &fMinLat, float &fMaxLong, float &fMinLong)
{
    // Let's define r = distance/EARTH_MEAN_RADIUS as the angular radius of
    // the circle around fLat and fLong
    double r = fDistance/EARTH_MEAN_RADIUS;

    fMinLat = (float) radToDeg (degToRad (fLat) - r);
    fMaxLat = (float) radToDeg (degToRad (fLat) + r);

    if ((fMinLat > MIN_LATITUDE) && (fMaxLat < MAX_LATITUDE)) {
        double dDeltaLongRad = asin (sin(r)/cos (degToRad(fLat)));
        fMinLong = (float) radToDeg (degToRad (fLong) - dDeltaLongRad);
        if (fMinLong < MIN_LONGITUDE) {
            fMinLong += MAX_LONGITUDE;
        }
        fMaxLong = (float) radToDeg (degToRad (fLong) + dDeltaLongRad);
        if (fMaxLong > MAX_LONGITUDE) {
            fMaxLong -= MAX_LONGITUDE;
        }
    }
    else {
        // a pole is within the distance
        fMinLat = maximum (fMinLat, MIN_LATITUDE);
        fMaxLat = minimum (fMaxLat, MAX_LATITUDE);
        fMinLong = MIN_LONGITUDE;
        fMaxLong = MAX_LONGITUDE;
    }
}

void NOMADSUtil::addPaddingToBoudingBox (float fMaxLat, float fMinLat,
                                         float fMaxLong, float fMinLong, float fPadding,
                                         float &fNewMaxLat, float &fNewMinLat,
                                         float &fNewMaxLong, float &fNewMinLong)
{
    // Sanity check
    if (fMaxLat < fMinLat) {
        checkAndLogMsg ("MathUtils::getBoundingBox", Logger::L_Warning,
                        "fMinLat is greater than fMaxLat - inverting\n");
        float fTemp = fMaxLat;
        fMaxLat = fMinLat;
        fMinLat = fTemp;
    }
    if (fMaxLong < fMinLong) {
        checkAndLogMsg ("MathUtils::getBoundingBox", Logger::L_Warning,
                        "fMinLong is greater than fMaxLong - inverting\n");
        float fTemp = fMaxLong;
        fMaxLong = fMinLong;
        fMinLong = fTemp;
    }

    float tmpMaxLat, tmpMinLat, tmpMaxLong, tmpMinLong;

    // C                  D
    //   +---------------+
    //   |               |
    //   +---------------+
    // F                  E

    // Get bounding-box of radius fPadding for the C vertex
    getBoundingBox (fMaxLat, fMinLong, fPadding, tmpMaxLat, tmpMinLat, tmpMaxLong, tmpMinLong);
    fNewMaxLat = tmpMaxLat;
    fNewMinLong = tmpMinLong;

    // Get bounding-box of radius fPadding for the E vertex
    getBoundingBox (fMinLat, fMaxLong, fPadding, tmpMaxLat, tmpMinLat, tmpMaxLong, tmpMinLong);
    fNewMinLat = tmpMinLat;
    fNewMaxLong = tmpMaxLong;
}

namespace NOMADSUtil
{
    float validateLatitude (double fLatitude)
    {
        if (fLatitude > MAX_LATITUDE) {
            return MAX_LATITUDE;
        }
        if (fLatitude < MIN_LATITUDE) {
            return MIN_LATITUDE;
        }
        return (float) fLatitude;
    }

    float validateLongitude (float fLongitude)
    {
        if (fLongitude > MAX_LONGITUDE) {
            return MAX_LONGITUDE;
        }
        if (fLongitude < MIN_LONGITUDE) {
            return MIN_LONGITUDE;
        }
        return (float) fLongitude;
    }
}

Point::Point (const Point &point)
    : _fLatitude (point._fLatitude),
    _fLongitude (point._fLongitude)
{
}

Point::Point (float fLatitude, float fLongitude)
    : _fLatitude (fLatitude),
    _fLongitude (fLongitude)
{
}

Point::~Point (void)
{

}

BoundingBox::BoundingBox (void)
    : _bEmpty (true), _leftUpperLatitude (0U), _leftUpperLongitude (0U),
    _rightLowerLatitude (0U), _rightLowerLongitude (0U)
{
}

BoundingBox::BoundingBox (float leftUpperLatitude, float leftUpperLongitude, float rightLowerLatitude, float rightLowerLongitude)
    : _bEmpty (!isValidLatitude (leftUpperLatitude) || !isValidLongitude (leftUpperLongitude) ||
        !isValidLatitude (rightLowerLatitude) || !isValidLongitude (rightLowerLongitude)),
    _leftUpperLatitude (leftUpperLatitude), _leftUpperLongitude (leftUpperLongitude),
    _rightLowerLatitude (rightLowerLatitude), _rightLowerLongitude (rightLowerLongitude)
{
}

BoundingBox::BoundingBox (const BoundingBox &bbox)
    : _bEmpty (bbox._bEmpty), _leftUpperLatitude (bbox._leftUpperLatitude),
    _leftUpperLongitude (bbox._leftUpperLongitude), _rightLowerLatitude (bbox._rightLowerLatitude),
    _rightLowerLongitude (bbox._rightLowerLongitude)
{
}

BoundingBox::~BoundingBox (void)
{
}

float BoundingBox::getArea (void) const
{
    return fabsf ((_leftUpperLatitude - _rightLowerLatitude) *
        (_rightLowerLongitude - _leftUpperLongitude));
}

Point BoundingBox::getBaricenter (void) const
{
    float fLat = _rightLowerLatitude + (float)(fabs (_leftUpperLatitude - _rightLowerLatitude) / 2.0f);
    float fLon = _leftUpperLongitude + (float)(fabs (_rightLowerLongitude - _leftUpperLongitude) / 2.0f);
    return Point (fLat, fLon);
}

float BoundingBox::getHeigth (void) const
{
    return (float) fabs (_leftUpperLatitude - _rightLowerLatitude);
}

float BoundingBox::getRadius (void) const
{
    Point baricenter = getBaricenter();
    return greatCircleDistance (baricenter._fLatitude, baricenter._fLongitude,
                                _leftUpperLatitude, _leftUpperLongitude);
}

float BoundingBox::getWidth (void) const
{
    return (float) fabs (_leftUpperLongitude - _rightLowerLongitude);
}

bool BoundingBox::isValid (void) const
{
    return !_bEmpty;
}

BoundingBox BoundingBox::getBoundingBox (Point &baricenter, float fRange)
{
    return getBoundingBox (baricenter._fLatitude, baricenter._fLongitude, fRange);
}

BoundingBox BoundingBox::getBoundingBox (float fLatitude, float fLongitude, float fRange)
{
    if (!isValidLatitude (fLatitude) || !isValidLongitude (fLongitude)) {
        return BoundingBox ();
    }
    double dLatDegDisplacement = 0.0;
    double dLonDegDisplacement = 0.0;
    metersToLatitudeDegrees (fLatitude, fRange, dLatDegDisplacement, dLonDegDisplacement);
    float leftUpperLatitude = NOMADSUtil::minimum (MAX_LATITUDE, validateLatitude (fLatitude + dLatDegDisplacement));
    float leftUpperLongitude = NOMADSUtil::maximum (MIN_LONGITUDE, validateLongitude (fLongitude - dLonDegDisplacement));
    float rightLowerLatitude = NOMADSUtil::maximum (MIN_LATITUDE, validateLatitude (fLatitude - dLatDegDisplacement));
    float rightLowerLongitude = NOMADSUtil::minimum (MAX_LONGITUDE, validateLongitude (fLongitude + dLonDegDisplacement));
    return BoundingBox (leftUpperLatitude, leftUpperLongitude, rightLowerLatitude, rightLowerLongitude);
}

BoundingBox BoundingBox::getIntersection (const BoundingBox &rhsBoundingBox) const
{
    if (!isValid() || !rhsBoundingBox.isValid()) {
        return BoundingBox();
    }
    if ((_rightLowerLatitude > rhsBoundingBox._leftUpperLatitude) ||
        (_leftUpperLatitude < rhsBoundingBox._rightLowerLatitude)) {
        return BoundingBox ();
    }
    if ((_rightLowerLongitude < rhsBoundingBox._leftUpperLongitude) ||
        (_leftUpperLongitude > rhsBoundingBox._rightLowerLongitude)) {
        return BoundingBox ();
    }

    // There is intersection
    //
    //   A        B
    //    +-------+
    //    |       |B'
    //    |  E +-------+ F
    //    |    |  |    |
    //    +-------+    |
    //    C  E'|  D    |
    //         |       |
    //       G +-------+ H
    //
    // Compute EB'DE' Area

    float luLat = minimum (_leftUpperLatitude, rhsBoundingBox._leftUpperLatitude);
    float rlLat = maximum (_rightLowerLatitude, rhsBoundingBox._rightLowerLatitude);

    float luLon = maximum (_leftUpperLongitude, rhsBoundingBox._leftUpperLongitude);
    float rlLon = minimum (_rightLowerLongitude, rhsBoundingBox._rightLowerLongitude);

    return BoundingBox (luLat, luLon, rlLat, rlLon);
}

