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
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        TeamData data;
        std::string gps_coordinates;

        if (std::getline(iss, data.name, '\t') &&
            std::getline(iss, data.address, '\t') &&
            std::getline(iss, gps_coordinates, '\n')) {

            std::istringstream gps_stream(gps_coordinates);
            std::string gps_x_str, gps_y_str;

            if (std::getline(gps_stream, gps_x_str, ',') &&
                std::getline(gps_stream, gps_y_str)) {
                try {
                    data.gps_x = std::stod(gps_x_str);
                    data.gps_y = std::stod(gps_y_str);
                    teamList.push_back(data);
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing GPS coordinates: " << e.what() << std::endl;
                }
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
    const std::vector<int>& leagueSizes)
{
    std::ofstream myfile(filename);
    
    if (!myfile.is_open()) {
        std::cerr << "Error creating output file: " << filename << std::endl;
        return;
    }

    int counter = 0;
    for (size_t i = 0; i < leagueSizes.size(); i++) {
        myfile << std::setprecision(8) << "# League " << i + 1 << "\n";
        for (int j = 0; j < leagueSizes[i]; j++) {
            const TeamData& team = teams[teamSorting[counter]];
            myfile << team.name << "\t" << team.address << "\t" 
                   << team.gps_x << ", " << team.gps_y << "\n";
            counter++;
        }
        myfile << "\n";
    }

    myfile.close();
    std::cout << "League assignment written to: " << filename << std::endl;
}
