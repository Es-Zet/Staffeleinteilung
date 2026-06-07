#ifndef HTML_VISUALIZER_H
#define HTML_VISUALIZER_H

#include <vector>
#include <string>
#include <map>
#include "evaluation_results.h"
#include "config.h"

/**
 * @class HtmlVisualizer
 * @brief Generates HTML and SVG visualizations of league assignments
 * 
 * Creates a printable HTML document with:
 * - Overall map showing all teams and league connections
 * - League-specific team lists with mini-maps
 * - Summary statistics
 */
class HtmlVisualizer
{
public:
    HtmlVisualizer(const Config& config);
    
    /**
     * Generate and save HTML visualization from evaluation results
     */
    bool generateHtml(const EvaluationResults& results, const std::string& outputFilename);
    
private:
    const Config& config;
    std::vector<std::string> leagueColors;
    
    // SVG generation helpers
    struct MapBounds {
        double minLat, maxLat, minLon, maxLon;
        double width, height;
    };
    
    MapBounds calculateMapBounds(const std::vector<TeamData>& teams) const;
    std::pair<double, double> projectCoordinates(double lat, double lon, const MapBounds& bounds) const;
    
    // Color management
    void initializeLeagueColors(size_t numLeagues);
    std::string getLeagueColor(int leagueIndex) const;
    
    // SVG generation
    std::string generateOverallMapSvg(const EvaluationResults& results) const;
    std::string generateLeagueMapSvg(const EvaluationResults& results, int leagueIndex) const;
    
    // HTML generation
    std::string generateHtmlHeader(const EvaluationResults& results) const;
    std::string generateSummarySection(const EvaluationResults& results) const;
    std::string generateOverallMapSection(const EvaluationResults& results) const;
    std::string generateTeamListSection(const EvaluationResults& results) const;
    std::string generateLeagueSection(const EvaluationResults& results, int leagueIndex) const;
    std::string generateHtmlFooter() const;
    
    // Text generation
    std::string sanitizeTeamName(const std::string& name) const;
    std::string escapeHtml(const std::string& text) const;
    
    // Strategic team label selection
    std::vector<int> selectStrategicTeamLabels(const EvaluationResults& results, int maxLabels = 5) const;
};

#endif // HTML_VISUALIZER_H
