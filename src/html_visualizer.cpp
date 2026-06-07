#include "html_visualizer.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <set>

HtmlVisualizer::HtmlVisualizer(const Config& config) : config(config)
{
}

bool HtmlVisualizer::generateHtml(const EvaluationResults& results, const std::string& outputFilename)
{
    if (results.getTeamCount() == 0 || results.getLeagueCount() == 0) {
        return false;
    }
    
    initializeLeagueColors(results.getLeagueCount());
    
    std::ofstream file(outputFilename);
    if (!file.is_open()) {
        return false;
    }
    
    // Generate HTML structure
    file << generateHtmlHeader(results);
    file << generateSummarySection(results);
    file << generateOverallMapSection(results);
    file << generateTeamListSection(results);
    file << generateHtmlFooter();
    
    file.close();
    return true;
}

HtmlVisualizer::MapBounds HtmlVisualizer::calculateMapBounds(const std::vector<TeamData>& teams) const
{
    MapBounds bounds;
    bounds.minLat = bounds.maxLat = teams[0].gps_lat;
    bounds.minLon = bounds.maxLon = teams[0].gps_long;
    
    for (const auto& team : teams) {
        bounds.minLat = std::min(bounds.minLat, team.gps_lat);
        bounds.maxLat = std::max(bounds.maxLat, team.gps_lat);
        bounds.minLon = std::min(bounds.minLon, team.gps_long);
        bounds.maxLon = std::max(bounds.maxLon, team.gps_long);
    }
    
    // Add 10% padding
    double latPadding = (bounds.maxLat - bounds.minLat) * 0.1;
    double lonPadding = (bounds.maxLon - bounds.minLon) * 0.1;
    bounds.minLat -= latPadding;
    bounds.maxLat += latPadding;
    bounds.minLon -= lonPadding;
    bounds.maxLon += lonPadding;
    
    // Set SVG dimensions (maintain aspect ratio)
    bounds.width = 800;
    double latRange = bounds.maxLat - bounds.minLat;
    double lonRange = bounds.maxLon - bounds.minLon;
    bounds.height = bounds.width * (latRange / lonRange);
    
    return bounds;
}

std::pair<double, double> HtmlVisualizer::projectCoordinates(double lat, double lon, const MapBounds& bounds) const
{
    double x = (lon - bounds.minLon) / (bounds.maxLon - bounds.minLon) * bounds.width;
    double y = bounds.height - (lat - bounds.minLat) / (bounds.maxLat - bounds.minLat) * bounds.height;
    return {x, y};
}

void HtmlVisualizer::initializeLeagueColors(size_t numLeagues)
{
    leagueColors.clear();
    
    // Predefined color palette for leagues
    const std::vector<std::string> colors = {
        "#FF6B6B", "#4ECDC4", "#45B7D1", "#FFA07A", "#98D8C8",
        "#F7DC6F", "#BB8FCE", "#85C1E2", "#F8B88B", "#ABEBC6",
        "#F5B7B1", "#85C1E2", "#D7DBDD", "#E8DAEF", "#F9E79F"
    };
    
    for (size_t i = 0; i < numLeagues; i++) {
        leagueColors.push_back(colors[i % colors.size()]);
    }
}

std::string HtmlVisualizer::getLeagueColor(int leagueIndex) const
{
    if (leagueIndex >= 0 && leagueIndex < static_cast<int>(leagueColors.size())) {
        return leagueColors[leagueIndex];
    }
    return "#000000";
}

std::string HtmlVisualizer::generateOverallMapSvg(const EvaluationResults& results) const
{
    const auto& teams = results.getTeamMetrics();
    std::vector<TeamData> teamData;
    for (const auto& tm : teams) {
        // Reconstruct TeamData from teamMetrics (simplified - we don't have full address info)
        // This is a limitation, but we have the essential GPS data
        // We'll need to get the actual team data from somewhere else
    }
    
    std::ostringstream svg;
    
    // For now, we'll generate a simplified version
    // In a real scenario, we'd need access to the full team data
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"600\" class=\"overall-map\">\n";
    svg << "  <rect width=\"800\" height=\"600\" fill=\"white\" stroke=\"#ccc\" stroke-width=\"1\"/>\n";
    
    // This will be properly implemented when we have access to team data in the visualization
    
    svg << "</svg>\n";
    return svg.str();
}

std::string HtmlVisualizer::generateLeagueMapSvg(const EvaluationResults& results, int leagueIndex) const
{
    const auto& leagues = results.getLeagues();
    if (leagueIndex < 0 || leagueIndex >= static_cast<int>(leagues.size())) {
        return "";
    }
    
    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"400\" height=\"300\" class=\"league-map\">\n";
    svg << "  <rect width=\"400\" height=\"300\" fill=\"white\" stroke=\"#ccc\" stroke-width=\"1\"/>\n";
    svg << "</svg>\n";
    
    return svg.str();
}

std::string HtmlVisualizer::generateHtmlHeader(const EvaluationResults& results) const
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"de\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "  <title>" + config.getLeagueIdentifier() + "einteilung</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: Arial, sans-serif; line-height: 1.6; margin: 20px; background: white; color: #333; }\n";
    html << "    .container { max-width: 1200px; margin: 0 auto; }\n";
    html << "    h1 { text-align: center; color: #2c3e50; border-bottom: 3px solid #3498db; padding-bottom: 10px; }\n";
    html << "    h2 { color: #34495e; margin-top: 30px; border-left: 5px solid #3498db; padding-left: 10px; }\n";
    html << "    h3 { color: #7f8c8d; font-size: 16px; }\n";
    html << "    .summary { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin: 20px 0; }\n";
    html << "    .summary-card { background: #ecf0f1; padding: 15px; border-radius: 5px; border-left: 4px solid #3498db; }\n";
    html << "    .summary-card h3 { margin-top: 0; }\n";
    html << "    .summary-value { font-size: 24px; font-weight: bold; color: #2c3e50; }\n";
    html << "    .metrics { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 15px 0; }\n";
    html << "    .metric { background: #f8f9fa; padding: 10px; border-radius: 3px; text-align: center; }\n";
    html << "    .metric-name { font-weight: bold; color: #34495e; }\n";
    html << "    .metric-description { font-size: 14px; color: #7f8c8d; margin-right: 10px; margin-left: 10px; }\n";
    html << "    .metric-value { font-size: 18px; color: #2c3e50; margin-top: 5px; }\n";
    html << "    svg { border: 1px solid #bdc3c7; border-radius: 5px; background: white; }\n";
    html << "    .map-section { margin: 30px 0; page-break-inside: avoid; }\n";
    html << "    .league-section { page-break-inside: avoid; margin: 30px 0; background: #f8f9fa; padding: 20px; border-radius: 5px; }\n";
    html << "    .league-header { font-size: 18px; font-weight: bold; margin-bottom: 15px; display: flex; align-items: center; }\n";
    html << "    .league-color { display: inline-block; width: 20px; height: 20px; margin-right: 10px; border-radius: 3px; }\n";
    html << "    table { width: 100%; border-collapse: collapse; margin: 15px 0; }\n";
    html << "    th { background: #34495e; color: white; padding: 10px; text-align: left; font-weight: bold; }\n";
    html << "    td { padding: 10px; border-bottom: 1px solid #bdc3c7; }\n";
    html << "    tr:nth-child(even) { background: #f5f5f5; }\n";
    html << "    .stats { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; margin: 10px 0; font-size: 13px; }\n";
    html << "    .stat-item { background: white; padding: 8px; border-radius: 3px; }\n";
    html << "    .stat-label { font-weight: bold; color: #7f8c8d; }\n";
    html << "    .stat-value { color: #2c3e50; margin-top: 2px; }\n";
    html << "    @media print { body { margin: 0; } .container { margin: 0; } }\n";
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <div class=\"container\">\n";
    html << "    <h1>" + config.getLeagueIdentifier() + "einteilung</h1>\n";
    html << "    <p>Hinweis: Alle Entfernungsangaben sind Luftlinie, berechnet anhand der GPS-Koordinaten der Standorte.</p>\n";
    
    return html.str();
}

std::string HtmlVisualizer::generateSummarySection(const EvaluationResults& results) const
{
    std::ostringstream html;
    html << "    <div class=\"summary\">\n";
    
    // Total teams and leagues
    html << "      <div class=\"summary-card\">\n";
    html << "        <h3>Gesamtzahl Teams</h3>\n";
    html << "        <div class=\"summary-value\">" << results.getTeamCount() << "</div>\n";
    html << "      </div>\n";
    
    html << "      <div class=\"summary-card\">\n";
    html << "        <h3>Staffeln</h3>\n";
    html << "        <div class=\"summary-value\">" << results.getLeagueCount() << "</div>\n";
    html << "      </div>\n";
    
    // Metrics
    html << "    </div>\n";
    html << "    <h2>Kennzahlen</h2>\n";
    html << "    <div class=\"metrics\">\n";
    
    for (const auto& metric : results.getMetrics()) {
        html << "      <div class=\"metric\">\n";
        html << "        <div class=\"metric-name\">" << escapeHtml(metric.name) << "</div>\n";
        html << "        <div class=\"metric-description\">" << escapeHtml(config.getMetricDescription(metric.name)) << "</div>\n";
        html << "        <div class=\"metric-value\">" << std::fixed << std::setprecision(2) << metric.value;
        if (!metric.unit.empty()) {
            html << " " << escapeHtml(metric.unit);
        }
        html << "</div>\n";
        html << "      </div>\n";
    }
    
    html << "    </div>\n";
    
    return html.str();
}

std::string HtmlVisualizer::generateOverallMapSection(const EvaluationResults& results) const
{
    std::ostringstream html;
    html << "    <h2>Gesamt-Übersicht aller Staffeln</h2>\n";
    html << "    <div class=\"map-section\">\n";
    html << generateOverallMapSvg(results);
    html << "    </div>\n";
    
    return html.str();
}

std::string HtmlVisualizer::generateTeamListSection(const EvaluationResults& results) const
{
    std::ostringstream html;
    html << "    <h2>Staffeln und Teamlisten</h2>\n";
    
    for (int i = 0; i < results.getLeagueCount(); i++) {
        html << generateLeagueSection(results, i);
    }
    
    return html.str();
}

std::string HtmlVisualizer::generateLeagueSection(const EvaluationResults& results, int leagueIndex) const
{
    const auto& leagues = results.getLeagues();
    if (leagueIndex < 0 || leagueIndex >= static_cast<int>(leagues.size())) {
        return "";
    }
    
    const auto& league = leagues[leagueIndex];
    std::ostringstream html;
    
    html << "    <div class=\"league-section\">\n";
    html << "      <div class=\"league-header\">\n";
    html << "        <div class=\"league-color\" style=\"background-color: " << getLeagueColor(leagueIndex) << ";\"></div>\n";
    html << "        <span>Staffel " << (leagueIndex + 1) << ": " << league.teamNames.size() << " Teams</span>\n";
    html << "      </div>\n";
    
    // Mini map
    html << "      <div style=\"margin: 10px 0;\">\n";
    html << generateLeagueMapSvg(results, leagueIndex);
    html << "      </div>\n";
    
    // League statistics
    html << "      <div class=\"stats\">\n";
    html << "        <div class=\"stat-item\">\n";
    html << "          <div class=\"stat-label\">Teams</div>\n";
    html << "          <div class=\"stat-value\">" << league.teamNames.size() << "</div>\n";
    html << "        </div>\n";
    html << "        <div class=\"stat-item\">\n";
    html << "          <div class=\"stat-label\">Gesamtentfernung</div>\n";
    html << "          <div class=\"stat-value\">" << std::fixed << std::setprecision(1) << league.totalLeagueDistance << " km</div>\n";
    html << "        </div>\n";
    html << "        <div class=\"stat-item\">\n";
    html << "          <div class=\"stat-label\">Max. Reiseentfernung (Gesamtsaison)</div>\n";
    html << "          <div class=\"stat-value\">" << std::fixed << std::setprecision(1) << league.maxTravel << " km</div>\n";
    html << "        </div>\n";
    html << "        <div class=\"stat-item\">\n";
    html << "          <div class=\"stat-label\">Ø Reiseentfernung (Gesamtsaison)</div>\n";
    html << "          <div class=\"stat-value\">" << std::fixed << std::setprecision(1) << league.avgTravel << " km</div>\n";
    html << "        </div>\n";
    html << "        <div class=\"stat-item\">\n";
    html << "          <div class=\"stat-label\">Max. Reiseentfernung (Spieltag)</div>\n";
    html << "          <div class=\"stat-value\">" << std::fixed << std::setprecision(1) << league.maxSingleDistance << " km</div>\n";
    html << "        </div>\n";
    html << "        <div class=\"stat-item\">\n";
    html << "          <div class=\"stat-label\">Ø Reiseentfernung (Spieltag)</div>\n";
    html << "          <div class=\"stat-value\">" << std::fixed << std::setprecision(1) << league.avgSingleDistance << " km</div>\n";
    html << "        </div>\n";
    html << "      </div>\n";
    
    // Team table
    html << "      <table>\n";
    html << "        <tr>\n";
    html << "          <th>#</th>\n";
    html << "          <th>Team</th>\n";
    html << "        </tr>\n";
    
    for (size_t i = 0; i < league.teamNames.size(); i++) {
        html << "        <tr>\n";
        html << "          <td>" << (i + 1) << "</td>\n";
        html << "          <td>" << escapeHtml(league.teamNames[i]) << "</td>\n";
        html << "        </tr>\n";
    }
    
    html << "      </table>\n";
    html << "    </div>\n";
    
    return html.str();
}

std::string HtmlVisualizer::generateHtmlFooter() const
{
    std::ostringstream html;
    html << "  </div>\n";
    html << "</body>\n";
    html << "</html>\n";
    return html.str();
}

std::string HtmlVisualizer::sanitizeTeamName(const std::string& name) const
{
    return name;
}

std::string HtmlVisualizer::escapeHtml(const std::string& text) const
{
    std::string result;
    for (char c : text) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c;
        }
    }
    return result;
}

std::vector<int> HtmlVisualizer::selectStrategicTeamLabels(const EvaluationResults& results, int maxLabels) const
{
    std::vector<int> selected;
    // Implementation for selecting strategic teams to label on the map
    // For now, return empty vector
    return selected;
}
