#include "league_splitter.h"

std::vector<int> LeagueSplitter::splitIntoLeagues(int numTeams, int maxLeagueSize, bool preferEvenSizes)
{
    std::vector<int> leagues;

    // calculate the number of leagues needed
    int numLeagues = 1 + (numTeams - 1) / maxLeagueSize;

    // determine the minimal league size
    int minLeagueSize = numTeams / numLeagues;

    // if the minimal LeagueSize is even-preference and it's odd, reduce it by one
    if (preferEvenSizes) {
        minLeagueSize -= minLeagueSize % 2;
    }

    // determine how many teams are leftover
    int leftoverTeams = numTeams - minLeagueSize * numLeagues;

    // fill the leagues accordingly
    for (int i = 0; i < numLeagues; i++) {
        // if the number of leftover teams is 0, just assign the minimal league number
        if (leftoverTeams == 0) {
            leagues.push_back(minLeagueSize);
        } else if (leftoverTeams == 1) {
            leagues.push_back(minLeagueSize + 1);
            --leftoverTeams;
        } else {
            leagues.push_back(minLeagueSize + 2);
            leftoverTeams = leftoverTeams - 2;
        }
    }

    return leagues;
}
