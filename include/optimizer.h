#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>
#include "file_io.h"

class Optimizer {
public:
    // Factory method to create appropriate optimizer based on metric name
    static std::unique_ptr<Optimizer> create(
        int numTeams,
        const std::string& metricName,
        const std::vector<TeamData>& teams,
        bool debug = false,
        bool disallowSameClub = false,
        double sameClubPenalty = 1000.0);

    virtual ~Optimizer() = default;

    // Calculate the metric value for a given sorting
    double calculateMetric(
        const std::vector<std::vector<double>>& distances,
        const std::vector<int>& sorting,
        const std::vector<int>& leagues,
        bool verbose = false) const;

    // Optimize team sorting using local search with pairwise swaps
    std::vector<int> optimizeSorting(
        const std::vector<std::vector<double>>& distances,
        std::vector<int> teamSorting,
        const std::vector<int>& leagues) const;

    // Output metric details for verbose mode
    void outputMetricDetails(const std::vector<std::vector<double>>& travels, double result) const;

protected:
    int numTeams;
    bool debug;
    std::vector<std::vector<double>> penaltyMatrix;

    Optimizer(int numTeams, bool debug, const std::vector<TeamData>& teams,
              bool disallowSameClub, double sameClubPenalty);

    // Calculate team travel distances for a given sorting
    std::vector<std::vector<double>> calculateTeamTravelDistances(
        const std::vector<std::vector<double>>& distances,
        const std::vector<int>& sorting,
        const std::vector<int>& leagues) const;

    // Calculate penalty for same-club teams in same league
    double calculateSameClubPenalty(
        const std::vector<int>& sorting,
        const std::vector<int>& leagues) const;

    // Abstract method for derived classes to implement metric-specific calculation
    virtual double calculateMetricImpl(const std::vector<std::vector<double>>& travels) const = 0;

    // Helper to convert metric type to string
    virtual std::string getMetricName() const = 0;
};

// Derived classes for each metric type
class TotalDistanceOptimizer : public Optimizer {
public:
    TotalDistanceOptimizer(int numTeams, const std::vector<TeamData>& teams,
                          bool debug, bool disallowSameClub, double sameClubPenalty);
protected:
    double calculateMetricImpl(const std::vector<std::vector<double>>& travels) const override;
    std::string getMetricName() const override { return "total_distance"; }
};

class MaxTeamDistanceOptimizer : public Optimizer {
public:
    MaxTeamDistanceOptimizer(int numTeams, const std::vector<TeamData>& teams,
                            bool debug, bool disallowSameClub, double sameClubPenalty);
protected:
    double calculateMetricImpl(const std::vector<std::vector<double>>& travels) const override;
    std::string getMetricName() const override { return "max_team_distance"; }
};

class MaxTravelPerTeamOptimizer : public Optimizer {
public:
    MaxTravelPerTeamOptimizer(int numTeams, const std::vector<TeamData>& teams,
                             bool debug, bool disallowSameClub, double sameClubPenalty);
protected:
    double calculateMetricImpl(const std::vector<std::vector<double>>& travels) const override;
    std::string getMetricName() const override { return "max_travel_per_team"; }
};

#endif // OPTIMIZER_H
