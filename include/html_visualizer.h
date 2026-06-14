#ifndef HTML_VISUALIZER_H
#define HTML_VISUALIZER_H

#include <vector>
#include <string>
#include <map>
#include "evaluation_results.h"
#include "config.h"
#include "distance_calculator.h"

/**
 * @class HtmlVisualizer
 * @brief Generates HTML and SVG visualizations of league assignments
 *
 * Creates a printable HTML document with:
 * - Overall map showing all teams and league connections
 * - League-specific team lists with mini-maps
 * - Summary statistics
 *
 * All maps use an equirectangular km projection via DistanceCalculator::toProjectedCoords
 * so that distances on screen are proportional to real distances.
 */
class HtmlVisualizer
{
public:
    HtmlVisualizer(const Config& config);

    /// Generate and save HTML visualization from evaluation results
    /// @param results  Evaluated league assignments with team and league data
    /// @param outputFilename  Path to write the HTML file
    bool generateHtml(
        const EvaluationResults& results,
        const std::string& outputFilename);

private:
    const Config& config;
    std::vector<std::string> leagueColors;

    // -------------------------------------------------------------------------
    // Projected coordinate system
    // -------------------------------------------------------------------------

    /// Flat (x, y) position in km after equirectangular projection
    struct ProjectedTeam {
        double x, y;   ///< km east / km north
        int    leagueIndex;
    };

    /// Project all teams to a shared km plane using DistanceCalculator::toProjectedCoords
    /// @param results  Used to look up each team's assigned league
    std::vector<ProjectedTeam> projectAllTeams(
        const EvaluationResults&     results) const;

    // -------------------------------------------------------------------------
    // SVG canvas helpers
    // -------------------------------------------------------------------------

    /// Viewport in km for a set of projected teams, with optional padding fraction
    struct KmBounds {
        double minX, maxX, minY, maxY;
        int svgWidth, svgHeight;  ///< pixel dimensions for the SVG element
    };

    /// Compute axis-aligned bounding box over all projected points
    /// @param projected  Full projected team list
    /// @param svgWidth   Desired pixel width (height is derived from aspect ratio)
    /// @param padding    Fractional padding added on each side (default 0.08)
    KmBounds computeKmBounds(
        const std::vector<ProjectedTeam>& projected,
        int    svgWidth  = 800,
        double padding   = 0.08) const;

    /// Map a km point to SVG pixel coordinates inside the given bounds
    std::pair<double, double> kmToSvg(
        double x, double y,
        const KmBounds& bounds) const;

    // -------------------------------------------------------------------------
    // Color management
    // -------------------------------------------------------------------------

    void initializeLeagueColors(size_t numLeagues);
    std::string getLeagueColor(int leagueIndex) const;

    // -------------------------------------------------------------------------
    // SVG generation
    // -------------------------------------------------------------------------

    /// Overall map: all league connections + all team dots, colored by league
    /// @param results    League data (teamIndices used for connections)
    /// @param projected  Projected coordinates for all teams
    std::string generateOverallMapSvg(
        const EvaluationResults&          results,
        const std::vector<ProjectedTeam>& projected) const;

    /// Per-league mini-map: all team dots (grey for others) + connections only for this league
    /// @param results      League data
    /// @param projected    Projected coordinates for all teams
    /// @param leagueIndex  Which league's connections to highlight
    std::string generateLeagueMapSvg(
        const EvaluationResults&          results,
        const std::vector<ProjectedTeam>& projected,
        int leagueIndex) const;

    /// Find the longest substring shared by all entries in @p names.
    /// Used to derive a town label for dots that represent multiple co-located teams.
    /// Returns an empty string if no common alphabetic substring of length ≥ 2 exists.
    static std::string longestCommonSubstring(const std::vector<std::string>& names);

    /// Shared SVG body builder used by both map functions
    /// @param projected    All projected teams
    /// @param bounds       Viewport to render into
    /// @param activeLeague League whose connections are drawn (-1 = all leagues)
    /// @param results      Source of league membership
    std::string buildMapSvgBody(
        const std::vector<ProjectedTeam>& projected,
        const KmBounds&                   bounds,
        int                               activeLeague,
        const EvaluationResults&          results) const;

    // -------------------------------------------------------------------------
    // HTML section generators
    // -------------------------------------------------------------------------

    std::string generateHtmlHeader(const EvaluationResults& results) const;
    std::string generateSummarySection(const EvaluationResults& results) const;

    /// @param projected  Passed through to the SVG generator
    std::string generateOverallMapSection(
        const EvaluationResults&          results,
        const std::vector<ProjectedTeam>& projected) const;

    /// @param projected  Passed through to per-league SVG generators
    std::string generateTeamListSection(
        const EvaluationResults&          results,
        const std::vector<ProjectedTeam>& projected) const;

    /// @param projected  Passed through to the league mini-map generator
    std::string generateLeagueSection(
        const EvaluationResults&          results,
        const std::vector<ProjectedTeam>& projected,
        int leagueIndex) const;

    std::string generateHtmlFooter() const;

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    std::string sanitizeTeamName(const std::string& name) const;
    std::string escapeHtml(const std::string& text) const;

    /// Select up to maxLabels team indices to label on the map (spread heuristic)
    std::vector<int> selectStrategicTeamLabels(
        const std::vector<ProjectedTeam>& projected,
        int maxLabels = 5) const;
};

#endif // HTML_VISUALIZER_H