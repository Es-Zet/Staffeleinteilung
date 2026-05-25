#include "distance_calculator.h"
#include <cmath>

std::vector<std::vector<double>> DistanceCalculator::createDistanceMatrix(
    const std::vector<TeamData>& teams,
    double kmPerDegreeLat,
    double coordinatePrecision,
    double maxDistanceThreshold)
{
    std::vector<std::vector<double>> matrix;
    int numTeams = teams.size();

    for (int i = 0; i < numTeams; i++) {
        std::vector<double> row;
        for (int j = 0; j < numTeams; j++) {
            // Calculate distance using haversine approximation
            double kmPerDegreeLong = kmPerDegreeLat * std::cos(
                (teams[i].gps_lat + teams[j].gps_lat) * M_PI / 360.0
            );
            double kmLat = kmPerDegreeLat * (teams[i].gps_lat - teams[j].gps_lat);
            double kmLong = kmPerDegreeLong * (teams[i].gps_long - teams[j].gps_long);
            
            double dist = std::sqrt(kmLat * kmLat + kmLong * kmLong);
            
            // Handle edge cases
            if (dist < coordinatePrecision) {
                dist = maxDistanceThreshold;
            }
            if (i == j) {
                dist = 0.0;
            }
            
            row.push_back(dist);
        }
        matrix.push_back(row);
    }

    return matrix;
}
