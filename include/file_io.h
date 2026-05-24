#ifndef FILE_IO_H
#define FILE_IO_H

#include <string>
#include <vector>
#include "team_data.h"

class FileIO {
public:
    /// Read team data from a tab-separated file with format: name \t address \t gps_x, gps_y
    static std::vector<TeamData> readTeamsFromFile(const std::string& filename);

    /// Write league assignments to an output file
    static void writeLeaguesAssignment(
        const std::string& filename,
        const std::vector<TeamData>& teams,
        const std::vector<int>& teamSorting,
        const std::vector<int>& leagueSizes);
};

#endif // FILE_IO_H
