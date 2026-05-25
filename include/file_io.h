#ifndef FILE_IO_H
#define FILE_IO_H

#include <string>
#include <vector>

struct TeamData {
    std::string name;
    std::string address;
    double gps_lat;
    double gps_long;
};

class FileIO {
public:
    /// Read team data from a CSV file with format: Team Name, Address, GPS Latitude, GPS Longitude
    static std::vector<TeamData> readTeamsFromFile(const std::string& filename);

    /// Write league assignments to an output file
    static void writeLeaguesAssignment(
        const std::string& filename,
        const std::vector<TeamData>& teams,
        const std::vector<int>& teamSorting,
        const std::vector<int>& leagueSizes);
};

#endif // FILE_IO_H
