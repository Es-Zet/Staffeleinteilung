#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>

enum class MetricType {
    TOTAL_DISTANCE,
    MAX_TEAM_DISTANCE,
    MAX_TRAVEL_PER_TEAM,
    AVG_DISTANCE_PER_TEAM,
    VARIANCE_DISTANCE
};

class Optimizer {
public:
    Optimizer(int numTeams, const std::string& metricName, bool debug = false)
        : numTeams(numTeams), debug(debug) {
        setMetric(metricName);
    }

    // Set the optimization metric
    void setMetric(const std::string& metricName) {
        if (metricName == "total_distance") {
            metric = MetricType::TOTAL_DISTANCE;
        } else if (metricName == "max_team_distance") {
            metric = MetricType::MAX_TEAM_DISTANCE;
        } else if (metricName == "max_travel_per_team") {
            metric = MetricType::MAX_TRAVEL_PER_TEAM;
        } else if (metricName == "avg_distance_per_team") {
            metric = MetricType::AVG_DISTANCE_PER_TEAM;
        } else if (metricName == "variance_distance") {
            metric = MetricType::VARIANCE_DISTANCE;
        } else {
            throw std::invalid_argument("Unknown metric: " + metricName);
        }
    }

    // Calculate team travel distances for a given sorting
    std::vector<std::vector<double>> calculateTeamTravelDistances(
        const std::vector<std::vector<double>>& distances,
        const std::vector<int>& sorting,
        const std::vector<int>& leagues) const {
        
        std::vector<std::vector<double>> travels;
        int leagueIndex = 0;

        for (size_t i = 0; i < leagues.size(); i++) {
            for (int j = 0; j < leagues[i]; j++) {
                std::vector<double> teamTravel;
                for (int k = 0; k < leagues[i]; k++) {
                    teamTravel.push_back(
                        distances[sorting[leagueIndex + j]][sorting[leagueIndex + k]]
                    );
                }
                travels.push_back(teamTravel);
            }
            leagueIndex += leagues[i];
        }

        return travels;
    }

    // Calculate the metric value for a given sorting
    double calculateMetric(
        const std::vector<std::vector<double>>& distances,
        const std::vector<int>& sorting,
        const std::vector<int>& leagues,
        bool verbose = false) const {
        
        std::vector<std::vector<double>> travels = calculateTeamTravelDistances(distances, sorting, leagues);
        
        double result = 0.0;
        
        switch (metric) {
            case MetricType::TOTAL_DISTANCE:
                result = calculateTotalDistance(travels);
                break;
            case MetricType::MAX_TEAM_DISTANCE:
                result = calculateMaxTeamDistance(travels);
                break;
            case MetricType::MAX_TRAVEL_PER_TEAM:
                result = calculateMaxTravelPerTeam(travels);
                break;
            case MetricType::AVG_DISTANCE_PER_TEAM:
                result = calculateAvgDistancePerTeam(travels);
                break;
            case MetricType::VARIANCE_DISTANCE:
                result = calculateVarianceDistance(travels);
                break;
        }

        if (debug && verbose) {
            outputMetricDetails(travels, result);
        }

        return result;
    }

    // Optimize team sorting using local search with pairwise swaps
    std::vector<int> optimizeSorting(
        const std::vector<std::vector<double>>& distances,
        std::vector<int> teamSorting,
        const std::vector<int>& leagues) const {
        
        std::vector<int> betterSorting = teamSorting;
        double betterValue = calculateMetric(distances, betterSorting, leagues);
        double newValue;
        bool betterFound = true;

        while (betterFound) {
            betterFound = false;
            for (int i = 0; i < numTeams; i++) {
                for (int j = i + 1; j < numTeams; j++) {
                    std::swap(teamSorting[i], teamSorting[j]);
                    newValue = calculateMetric(distances, teamSorting, leagues);
                    
                    if (newValue < betterValue) {
                        betterFound = true;
                        betterSorting = teamSorting;
                        betterValue = newValue;
                        
                        if (debug) {
                            std::cout << "Better sorting found (metric: " 
                                      << std::fixed << std::setprecision(6) << betterValue << ")" << std::endl;
                        }
                    } else {
                        teamSorting = betterSorting;
                    }
                }
            }
        }

        return betterSorting;
    }

private:
    int numTeams;
    MetricType metric = MetricType::TOTAL_DISTANCE;
    bool debug;

    double calculateTotalDistance(const std::vector<std::vector<double>>& travels) const {
        double total = 0.0;
        for (const auto& teamTravel : travels) {
            for (double distance : teamTravel) {
                total += distance;
            }
        }
        return total;
    }

    double calculateMaxTeamDistance(const std::vector<std::vector<double>>& travels) const {
        double maxDist = 0.0;
        for (const auto& teamTravel : travels) {
            for (double distance : teamTravel) {
                if (distance > maxDist) maxDist = distance;
            }
        }
        return maxDist;
    }

    double calculateMaxTravelPerTeam(const std::vector<std::vector<double>>& travels) const {
        double maxTeamTravel = 0.0;
        for (const auto& teamTravel : travels) {
            double teamSum = 0.0;
            for (double distance : teamTravel) {
                teamSum += distance;
            }
            if (teamSum > maxTeamTravel) maxTeamTravel = teamSum;
        }
        return maxTeamTravel;
    }

    double calculateAvgDistancePerTeam(const std::vector<std::vector<double>>& travels) const {
        double total = 0.0;
        int count = 0;
        for (const auto& teamTravel : travels) {
            for (double distance : teamTravel) {
                total += distance;
                count++;
            }
        }
        return count > 0 ? total / count : 0.0;
    }

    double calculateVarianceDistance(const std::vector<std::vector<double>>& travels) const {
        // Calculate sum of distances for each team
        std::vector<double> teamDistances;
        for (const auto& teamTravel : travels) {
            double sum = 0.0;
            for (double distance : teamTravel) {
                sum += distance;
            }
            teamDistances.push_back(sum);
        }

        // Calculate mean
        double mean = 0.0;
        for (double dist : teamDistances) {
            mean += dist;
        }
        mean /= teamDistances.size();

        // Calculate variance
        double variance = 0.0;
        for (double dist : teamDistances) {
            variance += (dist - mean) * (dist - mean);
        }
        variance /= teamDistances.size();

        return variance;
    }

    void outputMetricDetails(const std::vector<std::vector<double>>& travels, double result) const {
        std::cout << "\n----- Metric Details -----\n";
        
        double totalDist = calculateTotalDistance(travels);
        double maxDist = calculateMaxTeamDistance(travels);
        double maxTravel = calculateMaxTravelPerTeam(travels);
        double avgDist = calculateAvgDistancePerTeam(travels);
        
        std::cout << "Total distance: " << std::fixed << std::setprecision(2) << totalDist << " km\n";
        std::cout << "Max single distance: " << maxDist << " km\n";
        std::cout << "Max team travel: " << maxTravel << " km\n";
        std::cout << "Avg distance per team: " << avgDist << " km\n";
        std::cout << "Current metric (" << metricTypeToString() << "): " << result << "\n";
        std::cout << "--------------------------\n\n";
    }

    std::string metricTypeToString() const {
        switch (metric) {
            case MetricType::TOTAL_DISTANCE: return "total_distance";
            case MetricType::MAX_TEAM_DISTANCE: return "max_team_distance";
            case MetricType::MAX_TRAVEL_PER_TEAM: return "max_travel_per_team";
            case MetricType::AVG_DISTANCE_PER_TEAM: return "avg_distance_per_team";
            case MetricType::VARIANCE_DISTANCE: return "variance_distance";
        }
        return "unknown";
    }
};

#endif // OPTIMIZER_H
