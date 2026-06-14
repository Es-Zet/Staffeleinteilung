#include "file_io.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

std::vector<TeamData> FileIO::readTeamsFromFile(const std::string& filename)
{
    std::vector<TeamData> teamList;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        std::cerr << "File needs to be in the same folder and have the correct format." << std::endl;
        return teamList;
    }

    std::string line;
    bool firstLine = true;
    
    while (std::getline(file, line)) {
        // Skip header line
        if (firstLine) {
            firstLine = false;
            continue;
        }
        
        // Skip empty lines
        if (line.empty()) {
            continue;
        }
        
        std::istringstream iss(line);
        TeamData data;
        std::string gps_lat_str, gps_long_str;

        if (std::getline(iss, data.name, ',') &&
            std::getline(iss, data.address, ',') &&
            std::getline(iss, gps_lat_str, ',') &&
            std::getline(iss, gps_long_str, '\n')) {

            try {
                data.gps_lat = std::stod(gps_lat_str);
                data.gps_long = std::stod(gps_long_str);
                teamList.push_back(data);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing GPS coordinates: " << e.what() << std::endl;
            }
        }
    }

    file.close();
    return teamList;
}

void FileIO::writeLeaguesAssignment(
    const std::string& filename,
    const std::vector<TeamData>& teams,
    const std::vector<int>& teamSorting,
    const std::vector<int>& leagueSizes,
    const std::string& leagueIdentifier)
{
    std::ofstream myfile(filename);
    
    if (!myfile.is_open()) {
        std::cerr << "Error creating output file: " << filename << std::endl;
        return;
    }

    int counter = 0;
    for (size_t i = 0; i < leagueSizes.size(); i++) {
        myfile << std::setprecision(8) << "# " << leagueIdentifier << " " << i + 1 << "\n";
        for (int j = 0; j < leagueSizes[i]; j++) {
            const TeamData& team = teams[teamSorting[counter]];
            myfile << team.name << "\t" << team.address << "\t" 
                   << team.gps_lat << ", " << team.gps_long << "\n";
            counter++;
        }
        myfile << "\n";
    }

    myfile.close();
    std::cout << "League assignment written to: " << filename << std::endl;
}
