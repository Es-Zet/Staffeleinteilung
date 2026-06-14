#include "distance_calculator.h"

std::pair<double, double> DistanceCalculator::toProjectedCoords(double gps_lat, double gps_long, double kmPerDegreeLat)
{
    double kmPerDegreeLong = kmPerDegreeLat * std::cos(gps_lat * M_PI / 180.0);
    double pos_x = kmPerDegreeLong * gps_long;
    double pos_y = kmPerDegreeLat  * gps_lat;
    return {pos_x, pos_y};
}

std::vector<std::vector<double>> DistanceCalculator::createDistanceMatrix(
    const std::vector<TeamData>& teams,
    double kmPerDegreeLat)
{
    // Pre-project all teams once
    std::vector<std::pair<double,double>> projected;
    projected.reserve(teams.size());
    for (const auto& t : teams)
        projected.push_back(toProjectedCoords(t.gps_lat, t.gps_long, kmPerDegreeLat));

    int numTeams = teams.size();
    std::vector<std::vector<double>> matrix;
    matrix.reserve(numTeams);

    for (int i = 0; i < numTeams; i++) {
        std::vector<double> row;
        row.reserve(numTeams);
        for (int j = 0; j < numTeams; j++) {
            double dx = projected[i].first  - projected[j].first;
            double dy = projected[i].second - projected[j].second;
            double dist = std::sqrt(dx * dx + dy * dy);

            if (i == j) dist = 0.0;

            row.push_back(dist);
        }
        matrix.push_back(std::move(row));
    }

    return matrix;
}