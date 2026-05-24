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

    // List available metrics
    const std::vector<std::string>& getAvailableMetrics() const { return availableMetrics; }

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
    std::vector<std::string> availableMetrics = {
        "total_distance",
        "max_team_distance",
        "max_travel_per_team",
        "avg_distance_per_team",
        "variance_distance"
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
        
        // Extract integer values
        optimizationAttempts = extractIntValue(content, "attempts", optimizationAttempts);
        
        // Extract string values
        metric = extractStringValue(content, "metric", metric);
        
        // Extract boolean values
        debugEnabled = extractBoolValue(content, "enabled", debugEnabled);
        verboseEnabled = extractBoolValue(content, "verbose", verboseEnabled);
        preferEvenSizes = extractBoolValue(content, "prefer_even_sizes", preferEvenSizes);
        
        // Extract double values
        kmPerDegreeLat = extractDoubleValue(content, "km_per_degree_latitude", kmPerDegreeLat);
        maxDistanceThreshold = extractDoubleValue(content, "max_distance_threshold", maxDistanceThreshold);
        coordinatePrecision = extractDoubleValue(content, "coordinate_precision", coordinatePrecision);
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
