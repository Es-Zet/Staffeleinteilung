#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>

class Config {
public:
    Config(const std::string& filename = "config.json") {
        loadFromFile(filename);
    }

    // File names
    std::string getTeamsFile()      const { return fileNames.teams; }
    std::string getAssignmentFile() const { return fileNames.assignment; }
    std::string getEvaluationFile() const { return fileNames.evaluation; }
    std::string getResultsFile()    const { return fileNames.results; }

    // Optimization
    int         getOptimizationAttempts() const { return optimizationAttempts; }
    std::string getMetric()               const { return metric; }

    // Debug
    bool isDebugEnabled()   const { return debugEnabled; }
    bool isVerboseEnabled() const { return verboseEnabled; }

    // Geodesy
    double getKmPerDegreeLat()       const { return kmPerDegreeLat; }

    // League
    bool   shouldPreferEvenSizes()          const { return preferEvenSizes; }
    bool   shouldDisallowSameClubInLeague() const { return disallowSameClubInLeague; }
    double getSameClubPenalty()             const { return sameClubPenalty; }

    // Customization
    std::string getLeagueIdentifier() const { return leagueIdentifier; }

    // Metrics
    const std::vector<std::string>& getAvailableMetrics() const { return availableMetrics; }

    std::string getMetricDescription(const std::string& metricName) const {
        auto it = metricDescriptions.find(metricName);
        return it != metricDescriptions.end() ? it->second : "";
    }

    bool isValidMetric(const std::string& metricName) const {
        for (const auto& m : availableMetrics)
            if (m == metricName) return true;
        return false;
    }

private:
    // File names — no defaults, must be in config
    struct FileNames {
        std::string teams      = "Teamliste.csv";
        std::string assignment = "Staffeleinteilung.txt";
        std::string evaluation = "evaluation.json";
        std::string results    = "Staffeleinteilung.html";
    } fileNames;

    // All other fields have sensible fallback defaults in case config is missing
    int         optimizationAttempts    = 10;
    std::string metric                  = "total_distance";
    bool        debugEnabled            = true;
    bool        verboseEnabled          = true;
    double      kmPerDegreeLat          = 111.32;
    bool        preferEvenSizes         = true;
    bool        disallowSameClubInLeague= true;
    double      sameClubPenalty         = 1e9;
    std::string leagueIdentifier        = "Staffel";

    std::vector<std::string>             availableMetrics = { "total_distance" };
    std::map<std::string, std::string>   metricDescriptions;

    // -------------------------------------------------------------------------
    // Parsing
    // -------------------------------------------------------------------------

    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        // Extract a named { ... } section from a string
        auto section = [&](const std::string& src, const std::string& key) -> std::string {
            size_t pos = src.find("\"" + key + "\"");
            if (pos == std::string::npos) return "";
            pos = src.find("{", pos);
            if (pos == std::string::npos) return "";
            int depth = 1;
            size_t end = pos + 1;
            while (end < src.size() && depth > 0) {
                if (src[end] == '{') depth++;
                else if (src[end] == '}') depth--;
                end++;
            }
            return src.substr(pos, end - pos);
        };

        std::string fileSection    = section(content, "file_names");
        std::string optSection     = section(content, "optimization");
        std::string leagueSection  = section(content, "league");
        std::string geoSection     = section(content, "geodesy");
        std::string debugSection   = section(content, "debug");
        std::string metricsSection = section(content, "metrics");
        std::string customSection  = section(content, "customization");

        // file_names
        fileNames.teams      = extractStringValue(fileSection, "teams",      fileNames.teams);
        fileNames.assignment = extractStringValue(fileSection, "assignment", fileNames.assignment);
        fileNames.evaluation = extractStringValue(fileSection, "evaluation", fileNames.evaluation);
        fileNames.results    = extractStringValue(fileSection, "results",    fileNames.results);

        // optimization
        optimizationAttempts = extractIntValue   (optSection, "attempts", optimizationAttempts);
        metric               = extractStringValue(optSection, "metric",   metric);

        // league
        preferEvenSizes          = extractBoolValue  (leagueSection, "prefer_even_sizes",           preferEvenSizes);
        disallowSameClubInLeague = extractBoolValue  (leagueSection, "disallow_same_club_in_league", disallowSameClubInLeague);
        sameClubPenalty          = extractDoubleValue(leagueSection, "same_club_penalty",            sameClubPenalty);

        // geodesy
        kmPerDegreeLat       = extractDoubleValue(geoSection, "km_per_degree_latitude", kmPerDegreeLat);

        // debug
        debugEnabled   = extractBoolValue(debugSection, "enabled", debugEnabled);
        verboseEnabled = extractBoolValue(debugSection, "verbose", verboseEnabled);

        // customization
        leagueIdentifier = extractStringValue(customSection, "league_identifier", leagueIdentifier);

        // metrics.available
        size_t arrStart = metricsSection.find("\"available\"");
        if (arrStart != std::string::npos) {
            arrStart = metricsSection.find("[", arrStart);
            size_t arrEnd = metricsSection.find("]", arrStart);
            if (arrStart != std::string::npos && arrEnd != std::string::npos) {
                std::string arr = metricsSection.substr(arrStart + 1, arrEnd - arrStart - 1);
                availableMetrics.clear();
                size_t p = 0;
                while ((p = arr.find("\"", p)) != std::string::npos) {
                    size_t q = arr.find("\"", p + 1);
                    if (q == std::string::npos) break;
                    availableMetrics.push_back(arr.substr(p + 1, q - p - 1));
                    p = q + 1;
                }
            }
        }

        // metrics.description
        std::string descSection = section(metricsSection, "description");
        if (!descSection.empty()) {
            metricDescriptions.clear();
            size_t p = 1;  // skip opening brace
            while ((p = descSection.find("\"", p)) != std::string::npos) {
                size_t q = descSection.find("\"", p + 1);
                if (q == std::string::npos) break;
                std::string key = descSection.substr(p + 1, q - p - 1);
                size_t colon = descSection.find(":", q);
                if (colon == std::string::npos) break;
                size_t vs = descSection.find("\"", colon);
                if (vs == std::string::npos) break;
                size_t ve = descSection.find("\"", vs + 1);
                if (ve == std::string::npos) break;
                metricDescriptions[key] = descSection.substr(vs + 1, ve - vs - 1);
                p = ve + 1;
            }
        }

        // Ensure metric is valid; fall back to first available
        if (!availableMetrics.empty() && !isValidMetric(metric))
            metric = availableMetrics[0];
    }

    // -------------------------------------------------------------------------
    // Value extractors — search within a section string only
    // -------------------------------------------------------------------------

    int extractIntValue(const std::string& src, const std::string& key, int def) const {
        size_t pos = src.find("\"" + key + "\"");
        if (pos == std::string::npos) return def;
        pos = src.find(":", pos);
        if (pos == std::string::npos) return def;
        pos = src.find_first_not_of(" \t\n\r", pos + 1);
        size_t end = src.find_first_of(",}]", pos);
        try { return std::stoi(src.substr(pos, end - pos)); } catch (...) { return def; }
    }

    double extractDoubleValue(const std::string& src, const std::string& key, double def) const {
        size_t pos = src.find("\"" + key + "\"");
        if (pos == std::string::npos) return def;
        pos = src.find(":", pos);
        if (pos == std::string::npos) return def;
        pos = src.find_first_not_of(" \t\n\r", pos + 1);
        size_t end = src.find_first_of(",}]", pos);
        try { return std::stod(src.substr(pos, end - pos)); } catch (...) { return def; }
    }

    bool extractBoolValue(const std::string& src, const std::string& key, bool def) const {
        size_t pos = src.find("\"" + key + "\"");
        if (pos == std::string::npos) return def;
        pos = src.find(":", pos);
        if (pos == std::string::npos) return def;
        pos = src.find_first_not_of(" \t\n\r", pos + 1);
        if (src.substr(pos, 4) == "true")  return true;
        if (src.substr(pos, 5) == "false") return false;
        return def;
    }

    std::string extractStringValue(const std::string& src, const std::string& key, const std::string& def) const {
        size_t pos = src.find("\"" + key + "\"");
        if (pos == std::string::npos) return def;
        pos = src.find(":", pos);
        if (pos == std::string::npos) return def;
        pos = src.find("\"", pos);
        if (pos == std::string::npos) return def;
        size_t end = src.find("\"", pos + 1);
        if (end == std::string::npos) return def;
        return src.substr(pos + 1, end - pos - 1);
    }
};

#endif // CONFIG_H