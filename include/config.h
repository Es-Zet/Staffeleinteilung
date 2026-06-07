#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>

// Simple JSON parser for config file (minimal dependency approach)
class Config {
public:
    // Constructor loads the config from file
    Config(const std::string& filename = "config.json") {
        loadFromFile(filename);
    }

    // Getters for file names
    std::string getTeamsFile() const { return fileNames.teams; }
    std::string getAssignmentFile() const { return fileNames.assignment; }
    std::string getEvaluationFile() const { return fileNames.evaluation; }
    std::string getResultsFile() const { return fileNames.results; }

    // Getters for optimization settings
    int getOptimizationAttempts() const { return optimizationAttempts; }
    std::string getMetric() const { return metric; }
    bool isDebugEnabled() const { return debugEnabled; }
    bool isVerboseEnabled() const { return verboseEnabled; }

    // Getters for geodesy settings
    double getKmPerDegreeLat() const { return kmPerDegreeLat; }
    double getMaxDistanceThreshold() const { return maxDistanceThreshold; }
    double getCoordinatePrecision() const { return coordinatePrecision; }

    // Getters for league settings
    bool shouldPreferEvenSizes() const { return preferEvenSizes; }
    bool shouldDisallowSameClubInLeague() const { return disallowSameClubInLeague; }
    double getSameClubPenalty() const { return sameClubPenalty; }

    // Getters for customization options
    std::string getLeagueIdentifier() const { return leagueIdentifier; }

    // List available metrics
    const std::vector<std::string>& getAvailableMetrics() const { return availableMetrics; }
    std::string getMetricDescription(const std::string& metricName) const {
        auto it = metricDescriptions.find(metricName);
        if (it != metricDescriptions.end()) {
            return it->second;
        }
        return "No description available.";
    }

    // Validate that metric is available
    bool isValidMetric(const std::string& metricName) const {
        for (const auto& m : availableMetrics) {
            if (m == metricName) return true;
        }
        return false;
    }

private:
    // Configuration values with defaults
    int optimizationAttempts = 100;
    std::string metric = "total_distance";
    bool debugEnabled = true;
    bool verboseEnabled = true;
    double kmPerDegreeLat = 111.32;
    double maxDistanceThreshold = 1.0e10;
    double coordinatePrecision = 1.0e-16;
    bool preferEvenSizes = true;
    bool disallowSameClubInLeague = true;
    double sameClubPenalty = 1000.0;
    std::vector<std::string> availableMetrics = {
        "total_distance",
        "max_team_distance",
        "max_travel_per_team"
    };
    struct FileNames {
        std::string teams = "Teamliste.csv";
        std::string assignment = "Staffeleinteilung.txt";
        std::string evaluation = "evaluation.json";
        std::string results = "Staffeleinteilung.html";
    } fileNames;
    std::string leagueIdentifier = "Staffel";
    std::map<std::string, std::string> metricDescriptions = {
        {"total_distance", "Summe aller Reisestrecken aller Teams über die gesamte Saison"},
        {"max_team_distance", "Maximale Reisestrecke eines einzelnen Teams über die gesamte Saison"},
        {"max_travel_per_team", "Maximale Reisestrecke eines einzelnen Teams an einem einzelnen Spieltag"}
    };
    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // If config file doesn't exist, use defaults
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        // Simple key-value extraction from JSON
        // This is a minimal parser sufficient for our config structure

        // extract file names
        fileNames.teams = extractStringValue(content, "teams", fileNames.teams);
        fileNames.assignment = extractStringValue(content, "assignment", fileNames.assignment);
        fileNames.evaluation = extractStringValue(content, "evaluation", fileNames.evaluation);
        fileNames.results = extractStringValue(content, "results", fileNames.results);

        // Extract integer values
        optimizationAttempts = extractIntValue(content, "attempts", optimizationAttempts);
        
        // Extract string values
        metric = extractStringValue(content, "metric", metric);
        leagueIdentifier = extractStringValue(content, "league_identifier", leagueIdentifier);
        metricDescriptions["total_distance"] = extractStringValue(content, "description_total_distance", metricDescriptions["total_distance"]);
        metricDescriptions["max_team_distance"] = extractStringValue(content, "description_max_team_distance", metricDescriptions["max_team_distance"]);
        metricDescriptions["max_travel_per_team"] = extractStringValue(content, "description_max_travel_per_team", metricDescriptions["max_travel_per_team"]);

        // Extract boolean values
        debugEnabled = extractBoolValue(content, "enabled", debugEnabled);
        verboseEnabled = extractBoolValue(content, "verbose", verboseEnabled);
        preferEvenSizes = extractBoolValue(content, "prefer_even_sizes", preferEvenSizes);
        disallowSameClubInLeague = extractBoolValue(content, "disallow_same_club_in_league", disallowSameClubInLeague);
        
        // Extract double values
        kmPerDegreeLat = extractDoubleValue(content, "km_per_degree_latitude", kmPerDegreeLat);
        maxDistanceThreshold = extractDoubleValue(content, "max_distance_threshold", maxDistanceThreshold);
        coordinatePrecision = extractDoubleValue(content, "coordinate_precision", coordinatePrecision);
        sameClubPenalty = extractDoubleValue(content, "same_club_penalty", sameClubPenalty);
    }

    // Helper functions for JSON value extraction
    int extractIntValue(const std::string& content, const std::string& key, int defaultVal) const {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        
        pos = content.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        
        pos = content.find_first_not_of(" \t\n\r", pos + 1);
        size_t endPos = content.find_first_of(",}]", pos);
        
        try {
            return std::stoi(content.substr(pos, endPos - pos));
        } catch (...) {
            return defaultVal;
        }
    }

    double extractDoubleValue(const std::string& content, const std::string& key, double defaultVal) const {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        
        pos = content.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        
        pos = content.find_first_not_of(" \t\n\r", pos + 1);
        size_t endPos = content.find_first_of(",}]", pos);
        
        try {
            return std::stod(content.substr(pos, endPos - pos));
        } catch (...) {
            return defaultVal;
        }
    }

    bool extractBoolValue(const std::string& content, const std::string& key, bool defaultVal) const {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        
        pos = content.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        
        pos = content.find_first_not_of(" \t\n\r", pos + 1);
        
        if (content.substr(pos, 4) == "true") return true;
        if (content.substr(pos, 5) == "false") return false;
        return defaultVal;
    }

    std::string extractStringValue(const std::string& content, const std::string& key, const std::string& defaultVal) const {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        
        pos = content.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        
        pos = content.find("\"", pos);
        if (pos == std::string::npos) return defaultVal;
        
        size_t endPos = content.find("\"", pos + 1);
        if (endPos == std::string::npos) return defaultVal;
        
        return content.substr(pos + 1, endPos - pos - 1);
    }
};

#endif // CONFIG_H
