#ifndef EVALUATION_RESULTS_H
#define EVALUATION_RESULTS_H

#include <vector>
#include <string>
#include <map>
#include "file_io.h"

/**
 * @class EvaluationResults
 * @brief Container for league assignment evaluation results
 * 
 * Holds all computed metrics, league assignments, team data, and distance information.
 * Supports export to JSON for debugging and visualization.
 */
class EvaluationResults
{
public:
    /**
     * @struct LeagueData
     * @brief Data for a single league
     */
    struct LeagueData
    {
        int leagueNumber;
        std::vector<std::string> teamNames;
        std::vector<int> teamIndices;  // Original indices in teams vector
        double totalLeagueDistance = 0.0;
        double maxTravel = 0.0;
        double avgTravel = 0.0;
        double maxSingleDistance = 0.0;
        double avgSingleDistance = 0.0;
    };

    /**
     * @struct MetricResult
     * @brief Single metric calculation result
     */
    struct MetricResult
    {
        std::string name;
        double value = 0.0;
        std::string unit;  // e.g., "km", "", "count"
    };

    /**
     * @struct TeamMetrics
     * @brief Per-team travel distances and statistics
     */
    struct TeamMetrics
    {
        std::string teamName;
        int teamIndex;
        int assignedLeague;
        double totalTravel = 0.0;
        double avgTravel = 0.0;
        double maxSingleDistance = 0.0;
        std::vector<double> distancesToOtherTeamsInLeague;
    };

public:
    EvaluationResults();

    // Data setters
    void setTeamData(const std::vector<TeamData>& teams);
    void setLeagueSizes(const std::vector<int>& sizes);
    void setSorting(const std::vector<int>& sorting);
    void setDistanceMatrix(const std::vector<std::vector<double>>& distances);
    void addMetricResult(const std::string& name, double value, const std::string& unit = "");
    void setAssignmentFile(const std::string& filename);
    void setTeamFile(const std::string& filename);
    void calculateLeagueMetrics(); // needs to be calculate after adding metric results and setting league sizes

    // Data getters
    const std::vector<LeagueData>& getLeagues() const { return leagues; }
    const std::vector<MetricResult>& getMetrics() const { return metrics; }
    const std::vector<TeamMetrics>& getTeamMetrics() const { return teamMetrics; }
    int getTeamCount() const { return teams.size(); }
    int getLeagueCount() const { return leagues.size(); }
    double getTotalDistance() const;

    // Export functions
    std::string exportToJson() const;
    bool saveToJsonFile(const std::string& filename) const;

private:
    std::vector<TeamData> teams;
    std::vector<int> sorting;
    std::vector<std::vector<double>> distanceMatrix;
    std::vector<LeagueData> leagues;
    std::vector<int> leagueSizes;  // Store actual league sizes for accurate team distribution
    std::vector<MetricResult> metrics;
    std::vector<TeamMetrics> teamMetrics;
    std::string assignmentFilename;
    std::string teamFilename;

    // Helper methods
    std::string escapeJsonString(const std::string& str) const;
    void buildLeagueData();
    void buildTeamMetrics();
};

#endif // EVALUATION_RESULTS_H
