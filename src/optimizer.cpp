#include "optimizer.h"
#include <cmath>
#include <set>
#include <functional>
#include <algorithm>
#include <random>

// Factory method to create appropriate optimizer based on metric name
std::unique_ptr<Optimizer> Optimizer::create(
    int numTeams,
    const std::string& metricName,
    const std::vector<TeamData>& teams,
    bool debug,
    bool disallowSameClub,
    double sameClubPenalty)
{
    if (metricName == "total_distance") {
        return std::make_unique<TotalDistanceOptimizer>(numTeams, teams, debug, disallowSameClub, sameClubPenalty);
    } else if (metricName == "square_distance") {
        return std::make_unique<SquareDistanceOptimizer>(numTeams, teams, debug, disallowSameClub, sameClubPenalty);
    } else if (metricName == "max_team_distance") {
        return std::make_unique<MaxTeamDistanceOptimizer>(numTeams, teams, debug, disallowSameClub, sameClubPenalty);
    } else if (metricName == "max_travel_per_team") {
        return std::make_unique<MaxTravelPerTeamOptimizer>(numTeams, teams, debug, disallowSameClub, sameClubPenalty);
    } else {
        throw std::invalid_argument("Unknown metric: " + metricName);
    }
}

// Base class constructor
Optimizer::Optimizer(int numTeams, bool debug, const std::vector<TeamData>& teams,
                     bool disallowSameClub, double sameClubPenalty)
    : numTeams(numTeams), debug(debug)
{
    // Pre-compute penalty matrix for same-club team pairs
    penaltyMatrix.resize(numTeams, std::vector<double>(numTeams, 0.0));
    
    if (disallowSameClub) {
        for (int i = 0; i < numTeams; i++) {
            for (int j = 0; j < i; j++) {
                if (teams[i].address != teams[j].address) {
                    continue;
                }
                
                // find matching substring at beginning of team names to identify same club
                std::string clubShort = teams[i].name.length() < teams[j].name.length() ? teams[i].name : teams[j].name;
                std::string clubLong = teams[i].name.length() >= teams[j].name.length() ? teams[i].name : teams[j].name;
                size_t substringLength = clubShort.length() * 2/3;
                std::string substring = clubShort.substr(0, substringLength);
                if (clubLong.substr(0, substringLength) == substring) {
                    penaltyMatrix[i][j] = sameClubPenalty;
                    continue;
                }
            }
        }
        // symmetrize the matrix
        for (int i = 0; i < numTeams; i++) {
            for (int j = i; j < numTeams; j++) {
                penaltyMatrix[i][j] = penaltyMatrix[j][i];
            }
        }
    }
}

// Calculate team travel distances for a given sorting
std::vector<std::vector<double>> Optimizer::calculateTeamTravelDistances(
    const std::vector<std::vector<double>>& distances,
    const std::vector<int>& sorting,
    const std::vector<int>& leagues) const
{
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

// Calculate penalty for same-club teams in same league using pre-computed matrix
double Optimizer::calculateSameClubPenalty(
    const std::vector<int>& sorting,
    const std::vector<int>& leagues) const
{
    double penalty = 0.0;
    int leagueIndex = 0;
    
    for (size_t i = 0; i < leagues.size(); i++) {
        int leagueSize = leagues[i];
        
        // Check for teams from same club in this league
        for (int j = 0; j < leagueSize; j++) {
            for (int k = j + 1; k < leagueSize; k++) {
                int teamJ = sorting[leagueIndex + j];
                int teamK = sorting[leagueIndex + k];
                
                // Look up pre-computed penalty
                penalty += penaltyMatrix[teamJ][teamK];
            }
        }
        
        leagueIndex += leagueSize;
    }
    
    return penalty;
}

// Calculate the metric value for a given sorting (calls derived class implementation)
double Optimizer::calculateMetric(
    const std::vector<std::vector<double>>& distances,
    const std::vector<int>& sorting,
    const std::vector<int>& leagues,
    bool verbose) const
{
    std::vector<std::vector<double>> travels = calculateTeamTravelDistances(distances, sorting, leagues);
    
    // Call derived class's metric implementation
    double result = calculateMetricImpl(travels);
    
    // Apply same-club penalty
    double penalty = calculateSameClubPenalty(sorting, leagues);
    result += penalty;

    if (debug && verbose) {
        outputMetricDetails(travels, result);
    }

    return result;
}

bool Optimizer::reshuffleLeagues(
    const std::vector<std::vector<double>>& distances,
    std::vector<int>&                       teamSorting,
    const std::vector<int>&                 leagues) const
{
    auto computeStarts = [](const std::vector<int>& lg) {
        std::vector<int> starts(lg.size());
        starts[0] = 0;
        for (size_t i = 1; i < lg.size(); i++)
            starts[i] = starts[i-1] + lg[i-1];
        return starts;
    };

    auto combinations = [](int size, int depth) -> std::vector<std::vector<int>> {
        std::vector<std::vector<int>> result;
        std::vector<int> cur;
        std::function<void(int)> gen = [&](int start) {
            if (static_cast<int>(cur.size()) == depth) { result.push_back(cur); return; }
            for (int i = start; i <= size - (depth - static_cast<int>(cur.size())); i++) {
                cur.push_back(i);
                gen(i + 1);
                cur.pop_back();
            }
        };
        gen(0);
        return result;
    };

    const auto   starts        = computeStarts(leagues);
    const double currentMetric = calculateMetric(distances, teamSorting, leagues);

    // Only consider pairs where league sizes differ
    std::vector<std::tuple<int,int,int>> possibleSwaps;
    for (size_t i = 0; i < leagues.size(); i++) {
        for (size_t j = i + 1; j < leagues.size(); j++) {
            if (leagues[i] > leagues[j])
                possibleSwaps.emplace_back(i, j, leagues[i] - leagues[j]);
            else if (leagues[j] > leagues[i])
                possibleSwaps.emplace_back(j, i, leagues[j] - leagues[i]);
        }
    }

    // randomize possible swaps to avoid bias
    std::shuffle(possibleSwaps.begin(), possibleSwaps.end(), std::default_random_engine());

    for (const auto& [from, to, sizeDiff] : possibleSwaps) {
        for (const auto& fromCombo : combinations(leagues[from], sizeDiff)) {
            // Collect moved and staying teams
            std::set<int>    movedIdx(fromCombo.begin(), fromCombo.end());
            std::vector<int> movedTeams, stayFrom;
            for (int k = 0; k < leagues[from]; k++) {
                if (movedIdx.count(k)) movedTeams.push_back(teamSorting[starts[from] + k]);
                else                   stayFrom  .push_back(teamSorting[starts[from] + k]);
            }
            std::vector<int> toTeams;
            for (int k = 0; k < leagues[to]; k++)
                toTeams.push_back(teamSorting[starts[to] + k]);

            // Build new sorting with leagues `from` and `to` relabeled:
            // `from` slot gets toTeams + movedTeams (= leagues[from] teams)
            // `to`   slot gets stayFrom             (= leagues[to]   teams)
            std::vector<int> newSorting = teamSorting;
            int pos = starts[from];
            for (int t : toTeams)    newSorting[pos++] = t;
            for (int t : movedTeams) newSorting[pos++] = t;
            pos = starts[to];
            for (int t : stayFrom)   newSorting[pos++] = t;

            if (calculateMetric(distances, newSorting, leagues) < currentMetric) {
                teamSorting = newSorting;
                return true;
            }
        }
    }
    return false;
}

// Optimize team sorting using local search with pairwise swaps
std::vector<int> Optimizer::optimizeSorting(
    const std::vector<std::vector<double>>& distances,
    std::vector<int> teamSorting,
    const std::vector<int>& leagues) const
{
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
        // found local minimum, try reshuffling team sizes to escape
        if (!betterFound && reshuffleLeagues(distances, betterSorting, leagues)) {
            betterValue = calculateMetric(distances, betterSorting, leagues);
            teamSorting = betterSorting;  // keep swap loop in sync
            betterFound = true;
            if (debug) {
                std::cout << "Reshuffling yielded better sorting (metric: " 
                          << std::fixed << std::setprecision(6) << betterValue << ")" << std::endl;
            }
        }
    }

    return betterSorting;
}

// Output metric details for verbose mode
void Optimizer::outputMetricDetails(const std::vector<std::vector<double>>& travels, double result) const
{
    std::cout << "\n----- Metric Details -----\n";
    
    double totalDist = 0.0;
    double maxDist = 0.0;
    double maxTeamTravel = 0.0;
    double avgDist = 0.0;
    
    // Calculate total distance
    for (const auto& teamTravel : travels) {
        for (double distance : teamTravel) {
            totalDist += distance;
        }
    }
    
    // Calculate max single distance
    for (const auto& teamTravel : travels) {
        for (double distance : teamTravel) {
            if (distance > maxDist) maxDist = distance;
        }
    }
    
    // Calculate max team travel
    for (const auto& teamTravel : travels) {
        double teamSum = 0.0;
        for (double distance : teamTravel) {
            teamSum += distance;
        }
        if (teamSum > maxTeamTravel) maxTeamTravel = teamSum;
    }
    
    // Calculate average distance
    int count = 0;
    for (const auto& teamTravel : travels) {
        for (double distance : teamTravel) {
            avgDist += distance;
            count++;
        }
    }
    avgDist = count > 0 ? avgDist / count : 0.0;
    
    std::cout << "Total distance: " << std::fixed << std::setprecision(2) << totalDist << " km\n";
    std::cout << "Max single distance: " << maxDist << " km\n";
    std::cout << "Max team travel: " << maxTeamTravel << " km\n";
    std::cout << "Avg distance per team: " << avgDist << " km\n";
    std::cout << "Current metric (" << getMetricName() << "): " << result << "\n";
    std::cout << "--------------------------\n\n";
}

// ===== Derived Class Implementations =====

// TotalDistanceOptimizer
TotalDistanceOptimizer::TotalDistanceOptimizer(int numTeams, const std::vector<TeamData>& teams,
                                               bool debug, bool disallowSameClub, double sameClubPenalty)
    : Optimizer(numTeams, debug, teams, disallowSameClub, sameClubPenalty) {}

double TotalDistanceOptimizer::calculateMetricImpl(const std::vector<std::vector<double>>& travels) const
{
    double total = 0.0;
    for (const auto& teamTravel : travels) {
        for (double distance : teamTravel) {
            total += distance;
        }
    }
    return total;
}

// SquareDistanceOptimizer
SquareDistanceOptimizer::SquareDistanceOptimizer(int numTeams, const std::vector<TeamData>& teams,
                                                 bool debug, bool disallowSameClub, double sameClubPenalty)
    : Optimizer(numTeams, debug, teams, disallowSameClub, sameClubPenalty) {}

double SquareDistanceOptimizer::calculateMetricImpl(const std::vector<std::vector<double>>& travels) const
{
    double total = 0.0;
    for (const auto& teamTravel : travels) {
        for (double distance : teamTravel) {
            total += distance * distance;
        }
    }
    return total;
}

// MaxTeamDistanceOptimizer
MaxTeamDistanceOptimizer::MaxTeamDistanceOptimizer(int numTeams, const std::vector<TeamData>& teams,
                                                   bool debug, bool disallowSameClub, double sameClubPenalty)
    : Optimizer(numTeams, debug, teams, disallowSameClub, sameClubPenalty) {}

double MaxTeamDistanceOptimizer::calculateMetricImpl(const std::vector<std::vector<double>>& travels) const
{
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

// MaxTravelPerTeamOptimizer
MaxTravelPerTeamOptimizer::MaxTravelPerTeamOptimizer(int numTeams, const std::vector<TeamData>& teams,
                                                     bool debug, bool disallowSameClub, double sameClubPenalty)
    : Optimizer(numTeams, debug, teams, disallowSameClub, sameClubPenalty) {}

double MaxTravelPerTeamOptimizer::calculateMetricImpl(const std::vector<std::vector<double>>& travels) const
{
    double maxDist = 0.0;
    for (const auto& teamTravel : travels) {
        for (double distance : teamTravel) {
            if (distance > maxDist) maxDist = distance;
        }
    }
    return maxDist;
}
