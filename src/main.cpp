/// League Assignment Optimizer
/// Written by SZ with help of ChatGPT
/// Refactored for modularity and configuration

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <limits>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

#include "config.h"
#include "optimizer.h"
#include "file_io.h"
#include "league_splitter.h"
#include "distance_calculator.h"

using namespace std;

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // Load configuration from file (defaults used if file not found)
    cout << "League Assignment Optimizer" << endl;
    cout << "============================" << endl;
    cout << "Loading configuration..." << endl << endl;

    Config config;

    // Read team data from file
    // Expected format: CSV with headers (Team Name, Address, GPS X, GPS Y)
    cout << "Reading team data from " << config.getTeamsFile() << "..." << endl;
    vector<TeamData> teamList = FileIO::readTeamsFromFile(config.getTeamsFile());

    if (teamList.empty()) {
        cerr << "Error: No teams found in " << config.getTeamsFile() << endl;
        return 1;
    }

    int numTeams = teamList.size();
    cout << "Number of teams: " << numTeams << endl << endl;

    // Display available metrics
    // Display available metrics and ask for choice by index
    const auto& availableMetrics = config.getAvailableMetrics();
    cout << "Available optimization metrics:" << endl;
    for (size_t i = 0; i < availableMetrics.size(); i++)
        cout << "  [" << i << "] " << availableMetrics[i] << endl;
    cout << endl;

    string selectedMetric = config.getMetric();
    if (config.isDebugEnabled()) {
        cout << "Select metric by index [default: 0 = " << selectedMetric << "]: ";
        string userInput;
        getline(cin, userInput);
        if (!userInput.empty()) {
            try {
                int idx = std::stoi(userInput);
                if (idx >= 0 && idx < static_cast<int>(availableMetrics.size()))
                    selectedMetric = availableMetrics[idx];
                else
                    cerr << "Index out of range, using default: " << selectedMetric << endl;
            } catch (...) {
                // Also still accept typing the metric name directly
                if (config.isValidMetric(userInput))
                    selectedMetric = userInput;
                else
                    cerr << "Invalid input, using default: " << selectedMetric << endl;
            }
        }
    }
    cout << "Using metric: " << selectedMetric << endl << endl;

    // Ask for maximal league size
    int maxLeagueSize;
    while (true) {
        cout << "Maximum league size? ";
        cin >> maxLeagueSize;

        if (cin.fail() || maxLeagueSize < 2) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cerr << "Error: maximum league size must be an integer >= 2" << endl;
        } else {
            break;
        }
    }
    cout << endl;

    // Determine league split
    vector<int> leagueSizes = LeagueSplitter::splitIntoLeagues(
        numTeams,
        maxLeagueSize,
        config.shouldPreferEvenSizes()
    );

    // Display league sizes
    cout << "League sizes:" << endl;
    for (size_t i = 0; i < leagueSizes.size(); i++) {
        if (i > 0) cout << ", ";
        cout << leagueSizes[i];
    }
    cout << endl << endl;

    // Create distance matrix
    cout << "Calculating distance matrix..." << endl;
    vector<vector<double>> distanceMatrix = DistanceCalculator::createDistanceMatrix(
        teamList,
        config.getKmPerDegreeLat()
    );
    cout << "Distance matrix created." << endl << endl;

    // Create optimizer with selected metric using factory method
    auto optimizer = Optimizer::create(
        numTeams, 
        selectedMetric, 
        teamList,
        config.isDebugEnabled(), 
        config.shouldDisallowSameClubInLeague(), 
        config.getSameClubPenalty());

    // Display and allow editing of same-club pairings
    const auto& penaltyMatrix = optimizer->getPenaltyMatrix();
    std::vector<std::pair<int,int>> penalizedPairs;
    for (int i = 0; i < numTeams; i++)
        for (int j = i + 1; j < numTeams; j++)
            if (penaltyMatrix[i][j] > 0.0)
                penalizedPairs.push_back({i, j});

    if (!penalizedPairs.empty()) {
        cout << "Forbidden team pairings (" << penalizedPairs.size() << " pairs):" << endl;
        for (size_t i = 0; i < penalizedPairs.size(); i++) {
            auto [a, b] = penalizedPairs[i];
            cout << "  [" << i << "] "
                 << teamList[a].name << " <-> " << teamList[b].name << endl;
        }
        cout << endl;

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        while (true) {
            cout << "Enter index to remove, 'all' to remove all, or Enter to continue: ";
            string line;
            getline(cin, line);
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            if (line.empty()) break;

            if (line == "all") {
                for (auto [a, b] : penalizedPairs)
                    optimizer->clearPenalty(a, b);
                cout << "All pairings removed." << endl << endl;
                break;
            }

            try {
                int idx = std::stoi(line);
                if (idx >= 0 && idx < static_cast<int>(penalizedPairs.size())) {
                    auto [a, b] = penalizedPairs[idx];
                    if (optimizer->getPenaltyMatrix()[a][b] == 0.0) {
                        cout << "  Already removed." << endl;
                    } else {
                        optimizer->clearPenalty(a, b);
                        cout << "  Removed: " << teamList[a].name << " <-> " << teamList[b].name << endl;
                    }
                } else {
                    cout << "  Index out of range." << endl;
                }
            } catch (...) {
                cout << "  Invalid input." << endl;
            }

            // Reprint remaining active pairings
            cout << endl << "Remaining forbidden pairings:" << endl;
            bool any = false;
            for (size_t i = 0; i < penalizedPairs.size(); i++) {
                auto [a, b] = penalizedPairs[i];
                if (optimizer->getPenaltyMatrix()[a][b] > 0.0) {
                    cout << "  [" << i << "] "
                         << teamList[a].name << " <-> " << teamList[b].name << endl;
                    any = true;
                }
            }
            if (!any) {
                cout << "  (none)" << endl << endl;
                break;
            }
            cout << endl;
        }
    } else {
        cout << "No forbidden team pairings detected." << endl << endl;
    }

    // Run optimization
    cout << "Starting optimization..." << endl;
    vector<int> teamSorting;
    for (int i = 0; i < numTeams; i++) {
        teamSorting.push_back(i);
    }

    vector<int> bestSorting = teamSorting;
    double bestValue = optimizer->calculateMetric(distanceMatrix, teamSorting, leagueSizes);

    // Initialize random number generator
    auto rng = default_random_engine{};
    shuffle(begin(teamSorting), end(teamSorting), rng);

    int optimizationAttempts = config.getOptimizationAttempts();
    for (int attempt = 0; attempt < optimizationAttempts; attempt++) {
        if (config.isDebugEnabled()) {
            cout << "Optimization attempt " << (attempt + 1) << "/" << optimizationAttempts << endl;
        }

        teamSorting = optimizer->optimizeSorting(distanceMatrix, teamSorting, leagueSizes);
        double newValue = optimizer->calculateMetric(distanceMatrix, teamSorting, leagueSizes);

        if (newValue < bestValue) {
            bestSorting = teamSorting;
            bestValue = newValue;
            if (config.isDebugEnabled()) {
                cout << "  -> Better solution found (metric: " << fixed << setprecision(6) << newValue << ")" << endl;
            }
        }

        // Create new random initial sorting for next attempt
        shuffle(begin(teamSorting), end(teamSorting), rng);
    }

    cout << endl;
    cout << "Optimization complete." << endl << endl;

    // Sort leagues by size (larger first), then by geographic position (top-left to bottom-right)
    // Compute average projected position per league
    std::vector<std::pair<double,double>> leagueAvgPos(leagueSizes.size());
    {
        int offset = 0;
        for (size_t i = 0; i < leagueSizes.size(); i++) {
            double sumX = 0, sumY = 0;
            for (int j = 0; j < leagueSizes[i]; j++) {
                auto [x, y] = DistanceCalculator::toProjectedCoords(
                    teamList[bestSorting[offset + j]].gps_lat,
                    teamList[bestSorting[offset + j]].gps_long);
                sumX -= x; // left to right
                sumY += y;
            }
            leagueAvgPos[i] = {sumX / leagueSizes[i], sumY / leagueSizes[i]};
            offset += leagueSizes[i];
        }
    }

    // Build league order: sort by size desc, then y desc, then x asc (top-left to bottom-right)
    std::vector<int> leagueOrder(leagueSizes.size());
    std::iota(leagueOrder.begin(), leagueOrder.end(), 0);
    std::sort(leagueOrder.begin(), leagueOrder.end(), [&](int a, int b) {
        double scoreA = leagueAvgPos[a].second * 5 + leagueAvgPos[a].first;
        double scoreB = leagueAvgPos[b].second * 5 + leagueAvgPos[b].first;
        return scoreA > scoreB;
    });

    // Rebuild bestSorting and leagueSizes in the new order
    std::vector<int> sortedSorting;
    std::vector<int> sortedLeagueSizes;
    sortedSorting.reserve(bestSorting.size());
    {
        std::vector<int> starts(leagueSizes.size());
        starts[0] = 0;
        for (size_t i = 1; i < leagueSizes.size(); i++)
            starts[i] = starts[i-1] + leagueSizes[i-1];

        for (int li : leagueOrder) {
            // Collect team indices for this league
            std::vector<int> leagueTeams;
            for (int j = 0; j < leagueSizes[li]; j++)
                leagueTeams.push_back(bestSorting[starts[li] + j]);

            // Sort alphabetically by team name
            std::sort(leagueTeams.begin(), leagueTeams.end(), [&](int a, int b) {
                return teamList[a].name < teamList[b].name;
            });

            for (int t : leagueTeams)
                sortedSorting.push_back(t);
            sortedLeagueSizes.push_back(leagueSizes[li]);
        }
    }
    bestSorting   = sortedSorting;
    leagueSizes   = sortedLeagueSizes;

    // Write results to file
    FileIO::writeLeaguesAssignment(config.getAssignmentFile(), teamList, bestSorting, leagueSizes, config.getLeagueIdentifier());

    // Display final metrics
    cout << "\nFinal result:" << endl;
    cout << "=============" << endl;
    optimizer->calculateMetric(distanceMatrix, bestSorting, leagueSizes, true);

    cout << "Results written to " << config.getAssignmentFile() << endl;

    system("pause");

    return 0;
}
