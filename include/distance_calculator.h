#ifndef DISTANCE_CALCULATOR_H
#define DISTANCE_CALCULATOR_H

#include <vector>
#include "team_data.h"

class DistanceCalculator {
public:
    /// Create a distance matrix from team GPS coordinates
    /// @param teams Vector of team data containing GPS coordinates
    /// @param kmPerDegreeLat Conversion factor for latitude distance
    /// @param coordinatePrecision Minimum distance threshold
    /// @param maxDistanceThreshold Maximum distance to use for zero-distance teams
    static std::vector<std::vector<double>> createDistanceMatrix(
        const std::vector<TeamData>& teams,
        double kmPerDegreeLat = 111.32,
        double coordinatePrecision = 1.0e-16,
        double maxDistanceThreshold = 1.0e10);
};

#endif // DISTANCE_CALCULATOR_H
