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
                (teams[i].gps_x + teams[j].gps_x) * M_PI / 360.0
            );
            double kmLat = kmPerDegreeLat * (teams[i].gps_x - teams[j].gps_x);
            double kmLong = kmPerDegreeLong * (teams[i].gps_y - teams[j].gps_y);
            
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
