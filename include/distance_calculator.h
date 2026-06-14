#ifndef DISTANCE_CALCULATOR_H
#define DISTANCE_CALCULATOR_H

#include <cmath>
#include <vector>
#include <utility>
#include "file_io.h"

class DistanceCalculator {
public:
    /// Project GPS coordinates to flat (x, y) positions in km
    /// @param gps_lat  Latitude in degrees
    /// @param gps_long Longitude in degrees
    /// @param kmPerDegreeLat Conversion factor for latitude distance
    /// @return Pair of (pos_x, pos_y) in km using equirectangular projection
    static std::pair<double, double> toProjectedCoords(
        double gps_lat,
        double gps_long,
        double kmPerDegreeLat = 111.32);

    /// Create a distance matrix from team GPS coordinates
    /// @param teams Vector of team data containing GPS coordinates
    /// @param kmPerDegreeLat Conversion factor for latitude distance
    static std::vector<std::vector<double>> createDistanceMatrix(
        const std::vector<TeamData>& teams,
        double kmPerDegreeLat = 111.32);
};

#endif // DISTANCE_CALCULATOR_H