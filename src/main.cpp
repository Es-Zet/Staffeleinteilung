/// League Assignment Optimizer
/// Written by SZ with help of ChatGPT
/// Refactored for modularity and configuration

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <limits>
#include <iomanip>

#include "config.h"
#include "optimizer.h"
#include "team_data.h"
#include "file_io.h"
#include "league_splitter.h"
#include "distance_calculator.h"

using namespace std;

int main()
{
    // Load configuration from file (defaults used if file not found)
    cout << "League Assignment Optimizer" << endl;
    cout << "============================" << endl;
    cout << "Loading configuration..." << endl << endl;

    Config config;

    // Read team data from file
    // Expected format: name \t address \t gps_x, gps_y
    cout << "Reading team data from adressen.txt..." << endl;
    vector<TeamData> teamList = FileIO::readTeamsFromFile("adressen.txt");

    if (teamList.empty()) {
        cerr << "Error: No teams found in adressen.txt" << endl;
        return 1;
    }

    int numTeams = teamList.size();
    cout << "Number of teams: " << numTeams << endl << endl;

    // Display available metrics
    if (config.isVerboseEnabled()) {
        cout << "Available optimization metrics:" << endl;
        for (const auto& m : config.getAvailableMetrics()) {
            cout << "  - " << m << endl;
        }
        cout << endl;
    }

    // Ask for metric choice
    string selectedMetric = config.getMetric();
    if (config.isDebugEnabled()) {
        cout << "Enter optimization metric [" << selectedMetric << "]: ";
        string userInput;
        getline(cin, userInput);
        if (!userInput.empty() && config.isValidMetric(userInput)) {
            selectedMetric = userInput;
        }
        cout << "Using metric: " << selectedMetric << endl << endl;
    }

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
        config.getKmPerDegreeLat(),
        config.getCoordinatePrecision(),
        config.getMaxDistanceThreshold()
    );
    cout << "Distance matrix created." << endl << endl;

    // Create optimizer with selected metric
    Optimizer optimizer(numTeams, selectedMetric, config.isDebugEnabled());

    // Run optimization
    cout << "Starting optimization..." << endl;
    vector<int> teamSorting;
    for (int i = 0; i < numTeams; i++) {
        teamSorting.push_back(i);
    }

    vector<int> bestSorting = teamSorting;
    double bestValue = optimizer.calculateMetric(distanceMatrix, teamSorting, leagueSizes);

    // Initialize random number generator
    auto rng = default_random_engine{};
    shuffle(begin(teamSorting), end(teamSorting), rng);

    int optimizationAttempts = config.getOptimizationAttempts();
    for (int attempt = 0; attempt < optimizationAttempts; attempt++) {
        if (config.isDebugEnabled()) {
            cout << "Optimization attempt " << (attempt + 1) << "/" << optimizationAttempts << endl;
        }

        teamSorting = optimizer.optimizeSorting(distanceMatrix, teamSorting, leagueSizes);
        double newValue = optimizer.calculateMetric(distanceMatrix, teamSorting, leagueSizes);

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

    // Write results to file
    FileIO::writeLeaguesAssignment("Staffelaufteilung.txt", teamList, bestSorting, leagueSizes);

    // Display final metrics
    cout << "\nFinal result:" << endl;
    cout << "=============" << endl;
    optimizer.calculateMetric(distanceMatrix, bestSorting, leagueSizes, true);

    cout << "Results written to Staffelaufteilung.txt" << endl;

    system("pause");

    return 0;
}
