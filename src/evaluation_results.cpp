#include "evaluation_results.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

EvaluationResults::EvaluationResults()
{
}

void EvaluationResults::setTeamData(const std::vector<TeamData>& teamData)
{
    teams = teamData;
}

void EvaluationResults::setLeagueSizes(const std::vector<int>& sizes)
{
    // Store the actual league sizes
    leagueSizes = sizes;
    
    // Clear previous league data
    leagues.clear();
    
    // Create league structures (will be populated with team data later)
    for (size_t i = 0; i < sizes.size(); i++) {
        LeagueData league;
        league.leagueNumber = i + 1;
        leagues.push_back(league);
    }
}

void EvaluationResults::setSorting(const std::vector<int>& sortingOrder)
{
    sorting = sortingOrder;
    buildLeagueData();
    buildTeamMetrics();
}

void EvaluationResults::setDistanceMatrix(const std::vector<std::vector<double>>& distances)
{
    distanceMatrix = distances;
}

void EvaluationResults::addMetricResult(const std::string& name, double value, const std::string& unit)
{
    MetricResult result;
    result.name = name;
    result.value = value;
    result.unit = unit;
    metrics.push_back(result);
}

void EvaluationResults::setAssignmentFile(const std::string& filename)
{
    assignmentFilename = filename;
}

void EvaluationResults::setTeamFile(const std::string& filename)
{
    teamFilename = filename;
}

void EvaluationResults::buildLeagueData()
{
    if (leagues.empty() || sorting.empty() || teams.empty() || leagueSizes.empty()) {
        return;
    }

    size_t sortingIndex = 0;
    for (size_t leagueIdx = 0; leagueIdx < leagues.size() && leagueIdx < leagueSizes.size(); leagueIdx++) {
        LeagueData& league = leagues[leagueIdx];
        league.teamNames.clear();
        league.teamAddresses.clear();
        league.teamIndices.clear();
        league.totalLeagueDistance = 0.0;
        league.maxTravel = 0.0;
        league.avgTravel = 0.0;
        league.maxSingleDistance = 0.0;
        
        // Use the actual league size from the assignment
        size_t leagueSize = leagueSizes[leagueIdx];
        // Add team info for this league
        for (size_t i = 0; i < leagueSize && sortingIndex < sorting.size(); i++) {
            int teamIdx = sorting[sortingIndex];
            if (static_cast<size_t>(teamIdx) < teams.size()) {
                league.teamNames.push_back(teams[teamIdx].name);
                league.teamAddresses.push_back(teams[teamIdx].address);
                league.teamIndices.push_back(teamIdx);
            }
            sortingIndex++;
        }
    }
}

void EvaluationResults::buildTeamMetrics()
{
    teamMetrics.clear();
    
    if (sorting.empty() || distanceMatrix.empty() || teams.empty()) {
        return;
    }

    for (size_t leagueIdx = 0; leagueIdx < leagues.size(); leagueIdx++) {
        const LeagueData& league = leagues[leagueIdx];
        
        for (size_t i = 0; i < league.teamIndices.size(); i++) {
            int teamIdx = league.teamIndices[i];
            TeamMetrics tm;
            tm.teamName = teams[teamIdx].name;
            tm.teamIndex = teamIdx;
            tm.assignedLeague = leagueIdx + 1;
            
            // Calculate distances to other teams in same league
            tm.totalTravel = 0.0;
            tm.maxSingleDistance = 0.0;
            
            for (size_t j = 0; j < league.teamIndices.size(); j++) {
                if (i == j) continue;
                
                int otherTeamIdx = league.teamIndices[j];
                if (static_cast<size_t>(teamIdx) < distanceMatrix.size() && static_cast<size_t>(otherTeamIdx) < distanceMatrix[teamIdx].size()) {
                    double dist = distanceMatrix[teamIdx][otherTeamIdx];
                    tm.distancesToOtherTeamsInLeague.push_back(dist);
                    tm.totalTravel += dist;
                    tm.maxSingleDistance = std::max(tm.maxSingleDistance, dist);
                }
            }
            
            if (!tm.distancesToOtherTeamsInLeague.empty()) {
                tm.avgTravel = tm.totalTravel / tm.distancesToOtherTeamsInLeague.size();
            }
            
            teamMetrics.push_back(tm);
        }
    }
}

double EvaluationResults::getTotalDistance() const
{
    double total = 0.0;
    for (const auto& tm : teamMetrics) {
        total += tm.totalTravel;
    }
    return total;
}

void EvaluationResults::calculateLeagueMetrics()
{
    for (auto& league : leagues) {
        double leagueTotal = 0.0;
        double leagueMax = 0.0;
        double leagueAvg = 0.0;
        double leagueMaxSingle = 0.0;
        double leagueAvgSingle = 0.0;
        
        for (size_t i = 0; i < league.teamIndices.size(); i++) {
            int teamIdx = league.teamIndices[i];
            auto it = std::find_if(teamMetrics.begin(), teamMetrics.end(),
                [teamIdx](const TeamMetrics& tm) { return tm.teamIndex == teamIdx; });
            
            if (it != teamMetrics.end()) {
                leagueTotal += it->totalTravel;
                leagueMax = std::max(leagueMax, it->totalTravel);
                leagueMaxSingle = std::max(leagueMaxSingle, it->maxSingleDistance);
            }
        }
        if (!league.teamIndices.empty()) {
            leagueAvg = leagueTotal / league.teamIndices.size();
            leagueAvgSingle = leagueTotal / (league.teamIndices.size() * (league.teamIndices.size() - 1));
        }
        
        league.totalLeagueDistance = leagueTotal;
        league.maxTravel = leagueMax;
        league.avgTravel = leagueAvg;
        league.maxSingleDistance = leagueMaxSingle;
        league.avgSingleDistance = leagueAvgSingle;
    }
}

std::string EvaluationResults::escapeJsonString(const std::string& str) const
{
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

std::string EvaluationResults::exportToJson() const
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    
    oss << "{\n";
    oss << "  \"metadata\": {\n";
    oss << "    \"assignment_file\": \"" << escapeJsonString(assignmentFilename) << "\",\n";
    oss << "    \"team_file\": \"" << escapeJsonString(teamFilename) << "\"\n";
    oss << "  },\n";
    
    // Summary stats
    oss << "  \"summary\": {\n";
    oss << "    \"total_teams\": " << getTeamCount() << ",\n";
    oss << "    \"total_leagues\": " << getLeagueCount() << ",\n";
    oss << "    \"total_distance_km\": " << getTotalDistance() << ",\n";
    if (getTeamCount() > 0) {
        oss << "    \"avg_distance_per_team_km\": " << (getTotalDistance() / getTeamCount()) << "\n";
    } else {
        oss << "    \"avg_distance_per_team_km\": 0\n";
    }
    oss << "  },\n";
    
    // Metrics
    oss << "  \"metrics\": [\n";
    for (size_t i = 0; i < metrics.size(); i++) {
        oss << "    {\n";
        oss << "      \"name\": \"" << escapeJsonString(metrics[i].name) << "\",\n";
        oss << "      \"value\": " << metrics[i].value << ",\n";
        oss << "      \"unit\": \"" << escapeJsonString(metrics[i].unit) << "\"\n";
        oss << "    }";
        if (i < metrics.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";
    
    // Leagues
    oss << "  \"leagues\": [\n";
    for (size_t leagueIdx = 0; leagueIdx < leagues.size(); leagueIdx++) {
        const LeagueData& league = leagues[leagueIdx];
        oss << "    {\n";
        oss << "      \"league_number\": " << league.leagueNumber << ",\n";
        oss << "      \"team_count\": " << league.teamNames.size() << ",\n";
        oss << "      \"teams\": [\n";
        
        for (size_t i = 0; i < league.teamNames.size(); i++) {
            oss << "        \"" << escapeJsonString(league.teamNames[i]) << "\"";
            if (i < league.teamNames.size() - 1) oss << ",";
            oss << "\n";
        }
        
        oss << "      ],\n";
        oss << "      \"total_league_distance_km\": " << league.totalLeagueDistance << ",\n";
        oss << "      \"max_team_travel_km\": " << league.maxTravel << ",\n";
        oss << "      \"avg_team_travel_km\": " << league.avgTravel << ",\n";
        oss << "      \"max_single_distance_km\": " << league.maxSingleDistance << ",\n";
        oss << "      \"avg_single_distance_km\": " << league.avgSingleDistance << "\n";
        oss << "    }";
        if (leagueIdx < leagues.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";
    
    // Team details
    oss << "  \"teams\": [\n";
    for (size_t i = 0; i < teamMetrics.size(); i++) {
        const TeamMetrics& tm = teamMetrics[i];
        oss << "    {\n";
        oss << "      \"index\": " << tm.teamIndex << ",\n";
        oss << "      \"name\": \"" << escapeJsonString(tm.teamName) << "\",\n";
        oss << "      \"assigned_league\": " << tm.assignedLeague << ",\n";
        oss << "      \"total_travel_km\": " << tm.totalTravel << ",\n";
        oss << "      \"max_single_distance_km\": " << tm.maxSingleDistance << ",\n";
        oss << "      \"avg_distance_km\": " << tm.avgTravel << ",\n";
        oss << "      \"distances_to_league_teams_km\": [\n";
        
        for (size_t j = 0; j < tm.distancesToOtherTeamsInLeague.size(); j++) {
            oss << "        " << tm.distancesToOtherTeamsInLeague[j];
            if (j < tm.distancesToOtherTeamsInLeague.size() - 1) oss << ",";
            oss << "\n";
        }
        
        oss << "      ]\n";
        oss << "    }";
        if (i < teamMetrics.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ]\n";
    
    oss << "}\n";
    
    return oss.str();
}

bool EvaluationResults::saveToJsonFile(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << exportToJson();
    file.close();
    
    return true;
}