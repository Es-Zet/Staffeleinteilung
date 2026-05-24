#ifndef LEAGUE_SPLITTER_H
#define LEAGUE_SPLITTER_H

#include <vector>

class LeagueSplitter {
public:
    /// Split teams into leagues such that league sizes are as evenly distributed as possible
    /// and all leagues respect the maximum league size constraint.
    /// Returns a vector where each element is the size of a league.
    /// @param numTeams Total number of teams to split
    /// @param maxLeagueSize Maximum allowed size for a league
    static std::vector<int> splitIntoLeagues(int numTeams, int maxLeagueSize, bool preferEvenSizes = true);
};

#endif // LEAGUE_SPLITTER_H
