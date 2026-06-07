/// Metric Calculator - Evaluate League Assignments

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <map>

#include "config.h"
#include "optimizer.h"
#include "file_io.h"
#include "distance_calculator.h"
#include "evaluation_results.h"
#include "html_visualizer.h"

using namespace std;

/// Parse Staffelaufteilung.txt to extract league assignments and GPS coordinates
/// Returns: ParsedAssignment with teams (including GPS data if available), league_sizes, and flag
struct ParsedAssignment {
    vector<TeamData> teams;
    vector<int> leagueSizes;
    bool hasGpsCoordinates = false;
};

ParsedAssignment parseLeagueAssignment(const string& filename)
{
    ParsedAssignment result;
    result.teams.clear();
    result.leagueSizes.clear();
    result.hasGpsCoordinates = false;
    
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open " << filename << endl;
        return result;
    }

    string line;
    int currentLeagueSize = 0;
    
    while (getline(file, line)) {
        // Skip empty lines
        if (line.empty() || line.find_first_not_of(" \t\r\n") == string::npos) {
            continue;
        }

        // Check if this is a league header
        if (line.find("# Staffel") != string::npos) {
            // Save previous league size if any
            if (currentLeagueSize > 0) {
                result.leagueSizes.push_back(currentLeagueSize);
                currentLeagueSize = 0;
            }
            continue;
        }

        // Parse team data line ("name \t address \t gps_lat, gps_lon" or "name")
        istringstream iss(line);
        string teamName, address, gpsStr;
        
        if (getline(iss, teamName, '\t')) {
            // Trim whitespace from team name
            teamName.erase(0, teamName.find_first_not_of(" \t"));
            teamName.erase(teamName.find_last_not_of(" \t") + 1);
            
            if (!teamName.empty()) {
                TeamData team;
                team.name = teamName;
                team.address = "";
                team.gps_lat = 0.0;
                team.gps_long = 0.0;
                
                // Try to read address
                if (getline(iss, address, '\t')) {
                    address.erase(0, address.find_first_not_of(" \t"));
                    address.erase(address.find_last_not_of(" \t") + 1);
                    team.address = address;
                    
                    // Try to read GPS coordinates
                    if (getline(iss, gpsStr)) {
                        gpsStr.erase(0, gpsStr.find_first_not_of(" \t"));
                        gpsStr.erase(gpsStr.find_last_not_of(" \t\r\n") + 1);
                        
                        // Parse "lat, lon" format
                        size_t commaPos = gpsStr.find(',');
                        if (commaPos != string::npos) {
                            try {
                                string latStr = gpsStr.substr(0, commaPos);
                                string lonStr = gpsStr.substr(commaPos + 1);
                                
                                latStr.erase(0, latStr.find_first_not_of(" \t"));
                                latStr.erase(latStr.find_last_not_of(" \t") + 1);
                                lonStr.erase(0, lonStr.find_first_not_of(" \t"));
                                lonStr.erase(lonStr.find_last_not_of(" \t") + 1);
                                
                                team.gps_lat = stod(latStr);
                                team.gps_long = stod(lonStr);
                                result.hasGpsCoordinates = true;
                            } catch (...) {
                                // GPS parsing failed, coordinates remain 0
                            }
                        }
                    }
                }
                
                result.teams.push_back(team);
                currentLeagueSize++;
            }
        }
    }
    
    // Don't forget the last league
    if (currentLeagueSize > 0) {
        result.leagueSizes.push_back(currentLeagueSize);
    }

    file.close();
    return result;
}

/// Create a sorting vector by matching team names from assignment with team data
vector<int> createSortingFromAssignment(
    const vector<TeamData>& assignedTeams,
    const vector<TeamData>& allTeams)
{
    vector<int> sorting;
    map<string, int> teamIndexMap;
    
    // Build a map of team names to their indices
    for (size_t i = 0; i < allTeams.size(); i++) {
        teamIndexMap[allTeams[i].name] = i;
    }
    
    // Create sorting vector based on assigned team names
    for (const auto& assignedTeam : assignedTeams) {
        auto it = teamIndexMap.find(assignedTeam.name);
        if (it != teamIndexMap.end()) {
            sorting.push_back(it->second);
        } else {
            cerr << "Warning: Team '" << assignedTeam.name << "' not found in team data" << endl;
        }
    }
    
    return sorting;
}

void displayMetricsComparison(
    const vector<vector<double>>& distanceMatrix,
    const vector<int>& sorting,
    const vector<int>& leagueSizes,
    const vector<TeamData>& teams)
{
    // read metrics from config
    vector<string> metricNames = {
        "total_distance",
        "max_team_distance",
        "max_travel_per_team"
    };

    cout << "\n";
    cout << "==============================================================" << endl;
    cout << "=         League Assignment Metrics Evaluation               =" << endl;
    cout << "==============================================================" << endl;
    cout << endl;

    // Calculate all metrics
    vector<double> metricValues;
    for (const auto& metricName : metricNames) {
        try {
            auto optimizer = Optimizer::create((int)sorting.size(), metricName, teams);
            if (optimizer) {
                double value = optimizer->calculateMetric(distanceMatrix, sorting, leagueSizes);
                metricValues.push_back(value);
            } else {
                metricValues.push_back(-1.0);
            }
        } catch (...) {
            metricValues.push_back(-1.0);
        }
    }

    // Display metrics
    cout << "Metric Results:" << endl;
    cout << "==============================================================" << endl;
    
    int width = 30;
    for (size_t i = 0; i < metricNames.size(); i++) {
        cout << left << setw(width) << metricNames[i];
        cout << ": " << fixed << setprecision(2) << metricValues[i] << " km" << endl;
    }

    cout << endl;
    cout << "Summary Statistics:" << endl;
    cout << "==============================================================" << endl;

    // Summary calculations
    double totalDist = metricValues[0];  // total_distance
    int numTeams = sorting.size();
    int numLeagues = leagueSizes.size();

    cout << left << setw(width) << "Total teams" << ": " << numTeams << endl;
    cout << left << setw(width) << "Number of leagues" << ": " << numLeagues << endl;
    cout << left << setw(width) << "Avg league size" << ": " 
         << fixed << setprecision(1) << (numTeams / numLeagues) << endl;
    cout << left << setw(width) << "Total distance" << ": " 
         << fixed << setprecision(2) << totalDist << " km" << endl;
    cout << left << setw(width) << "Avg distance/team" << ": " 
         << fixed << setprecision(2) << (totalDist / numTeams) << " km" << endl;

    cout << endl;
}

int main(int argc, char* argv[])
{
    cout << "Metric Calculator" << endl;
    cout << "==============================================================" << endl << endl;

    // Load configuration
    Config config;

    // Parse league assignment (may include GPS coordinates)
    string assignmentFile = config.getAssignmentFile();
    if (argc > 1) {
        assignmentFile = argv[1];
        cout << "Overriding assignment data file with " << assignmentFile << "..." << endl;
    }
    
    cout << "Reading league assignment from " << assignmentFile << "..." << endl;
    ParsedAssignment parsedAssignment = parseLeagueAssignment(assignmentFile);

    if (parsedAssignment.teams.empty()) {
        cerr << "Error: No teams found in " << assignmentFile << endl;
        return 1;
    }
    
    cout << "Loaded " << parsedAssignment.teams.size() << " teams in " 
         << parsedAssignment.leagueSizes.size() << " leagues." << endl;
    
    if (parsedAssignment.hasGpsCoordinates) {
        cout << "GPS coordinates found in assignment file." << endl << endl;
    } else {
        cout << "No GPS coordinates in assignment file, will read from " << config.getTeamsFile() << "." << endl << endl;
    }

    // Read team data with GPS coordinates
    vector<TeamData> teams;
    
    if (parsedAssignment.hasGpsCoordinates) {
        // Use parsed teams from assignment file (which already have GPS)
        teams = parsedAssignment.teams;
        cout << "Using GPS coordinates from " << assignmentFile << "." << endl << endl;
    } else {
        // Read full team data from Teamliste.csv for GPS coordinates
        cout << "Reading GPS coordinates from " << config.getTeamsFile() << "..." << endl;
        vector<TeamData> teamsFromFile = FileIO::readTeamsFromFile(config.getTeamsFile());
        
        if (teamsFromFile.empty()) {
            cerr << "Error: No teams found in " << config.getTeamsFile() << endl;
            return 1;
        }
        
        // Build a map from team name to GPS data
        map<string, pair<double, double>> gpsMap;
        for (const auto& team : teamsFromFile) {
            gpsMap[team.name] = {team.gps_lat, team.gps_long};
        }
        
        // Merge GPS data into parsed teams
        for (auto& team : parsedAssignment.teams) {
            auto it = gpsMap.find(team.name);
            if (it != gpsMap.end()) {
                team.gps_lat = it->second.first;
                team.gps_long = it->second.second;
            } else {
                cerr << "Warning: GPS coordinates not found for team '" << team.name << "'" << endl;
            }
        }
        
        teams = parsedAssignment.teams;
        cout << "GPS coordinates loaded from " << config.getTeamsFile() << "." << endl << endl;
    }
    
    // Clean data by trimming whitespace at end of team names
    for (auto& team : teams) {
        team.name.erase(team.name.find_last_not_of(" \t\r\n") + 1);
    }

    // Create sorting vector
    cout << "Mapping teams..." << endl;
    vector<int> sorting = createSortingFromAssignment(parsedAssignment.teams, teams);
    
    if (sorting.size() != parsedAssignment.teams.size()) {
        cerr << "Error: Could not match all teams" << endl;
        return 1;
    }
    cout << "Mapping complete." << endl << endl;

    // Calculate distance matrix
    cout << "Calculating distance matrix..." << endl;
    vector<vector<double>> distanceMatrix = DistanceCalculator::createDistanceMatrix(
        teams,
        config.getKmPerDegreeLat(),
        config.getCoordinatePrecision(),
        config.getMaxDistanceThreshold()
    );
    cout << "Distance matrix calculated." << endl << endl;

    // Create and populate evaluation results
    cout << "Collecting evaluation results..." << endl;
    EvaluationResults results;
    results.setTeamFile(parsedAssignment.hasGpsCoordinates ? "" : config.getTeamsFile());
    results.setAssignmentFile(assignmentFile);
    results.setTeamData(teams);
    results.setDistanceMatrix(distanceMatrix);
    results.setLeagueSizes(parsedAssignment.leagueSizes);
    results.setSorting(sorting);
    
    // Calculate all metrics and add to results
    vector<string> metricNames = {
        "total_distance",
        "max_team_distance",
        "max_travel_per_team"
    };
    
    for (const auto& metricName : metricNames) {
        try {
            auto optimizer = Optimizer::create((int)sorting.size(), metricName, teams);
            if (optimizer) {
                double value = optimizer->calculateMetric(distanceMatrix, sorting, parsedAssignment.leagueSizes);
                results.addMetricResult(metricName, value, "km");
            }
        } catch (const exception& e) {
            cerr << "Error calculating " << metricName << ": " << e.what() << endl;
        }
    }
    results.calculateLeagueMetrics();
    
    cout << "Results collected." << endl << endl;

    // Display metrics on console
    displayMetricsComparison(distanceMatrix, sorting, parsedAssignment.leagueSizes, teams);

    cout << "League Sizes: ";
    for (size_t i = 0; i < parsedAssignment.leagueSizes.size(); i++) {
        if (i > 0) cout << ", ";
        cout << parsedAssignment.leagueSizes[i];
    }
    cout << endl << endl;

    // Save results to JSON file
    string jsonFilename = config.getEvaluationFile();
    cout << "Saving results to " << jsonFilename << "..." << endl;
    if (results.saveToJsonFile(jsonFilename)) {
        cout << "Results saved successfully to " << jsonFilename << "." << endl;
    } else {
        cerr << "Warning: Could not save results to " << jsonFilename << endl;
    }

    // Save results to HTML file
    string htmlFilename = config.getResultsFile();
    HtmlVisualizer visualizer(config);
    if (!visualizer.generateHtml(results, htmlFilename)) {
        cerr << "Warning: Could not save results to " << htmlFilename << endl;
    } else {
        cout << "Results saved successfully to " << htmlFilename << "." << endl;
    }

    return 0;
}
