#include "html_visualizer.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <numeric>

// -----------------------------------------------------------------------
// Construction & public entry point
// -----------------------------------------------------------------------

HtmlVisualizer::HtmlVisualizer(const Config& config) : config(config) {}

bool HtmlVisualizer::generateHtml(
    const EvaluationResults&   results,
    const std::string&          outputFilename)
{
    if (results.getTeamCount() == 0 || results.getLeagueCount() == 0)
        return false;

    initializeLeagueColors(results.getLeagueCount());

    std::ofstream file(outputFilename);
    if (!file.is_open())
        return false;

    const auto projected = projectAllTeams(results);

    file << generateHtmlHeader(results);
    file << generateSummarySection(results);
    file << generateOverallMapSection(results, projected);
    file << generateTeamListSection(results, projected);
    file << generateHtmlFooter();

    return true;
}

// -----------------------------------------------------------------------
// Projection
// -----------------------------------------------------------------------

std::vector<HtmlVisualizer::ProjectedTeam> HtmlVisualizer::projectAllTeams(
    const EvaluationResults&     results) const
{
    // Build a teamIndex -> leagueIndex lookup from LeagueData
    std::vector<int> teamLeague(results.getTeamCount(), -1);
    const auto& leagues = results.getLeagues();
    for (int li = 0; li < static_cast<int>(leagues.size()); li++) {
        for (int ti : leagues[li].teamIndices) {
            if (ti >= 0 && ti < static_cast<int>(results.getTeamCount()))
                teamLeague[ti] = li;
        }
    }

    std::vector<ProjectedTeam> out;
    out.reserve(results.getTeamCount());
    for (int i = 0; i < static_cast<int>(results.getTeamCount()); i++) {
        auto [x, y] = DistanceCalculator::toProjectedCoords(
            results.getTeam(i).gps_lat, results.getTeam(i).gps_long);
        out.push_back({x, y, teamLeague[i]});
    }
    return out;
}

// -----------------------------------------------------------------------
// SVG canvas helpers
// -----------------------------------------------------------------------

HtmlVisualizer::KmBounds HtmlVisualizer::computeKmBounds(
    const std::vector<ProjectedTeam>& projected,
    int    svgWidth,
    double padding) const
{
    KmBounds b{};
    b.minX = b.minY =  std::numeric_limits<double>::max();
    b.maxX = b.maxY = -std::numeric_limits<double>::max();

    for (const auto& p : projected) {
        b.minX = std::min(b.minX, p.x);
        b.maxX = std::max(b.maxX, p.x);
        b.minY = std::min(b.minY, p.y);
        b.maxY = std::max(b.maxY, p.y);
    }

    double padX = (b.maxX - b.minX) * padding;
    double padY = (b.maxY - b.minY) * padding;
    b.minX -= padX;  b.maxX += padX;
    b.minY -= padY;  b.maxY += padY;

    double aspect = (b.maxY - b.minY) / (b.maxX - b.minX);
    b.svgWidth  = svgWidth;
    b.svgHeight = static_cast<int>(svgWidth * aspect);

    return b;
}

std::pair<double, double> HtmlVisualizer::kmToSvg(
    double x, double y,
    const KmBounds& b) const
{
    double px = (x - b.minX) / (b.maxX - b.minX) * b.svgWidth;
    // y axis: km increases northward, SVG y increases downward
    double py = (1.0 - (y - b.minY) / (b.maxY - b.minY)) * b.svgHeight;
    return {px, py};
}

// -----------------------------------------------------------------------
// Color management
// -----------------------------------------------------------------------

void HtmlVisualizer::initializeLeagueColors(size_t numLeagues)
{
    leagueColors.clear();
    const std::vector<std::string> palette = {
        "#D62839", // 0°   red
        "#CC9900", // 90°  yellow
        "#0B8C6B", // 180° teal
        "#2B4ECC", // 270° blue
        "#CC5500", // 45°  burnt orange
        "#4A9900", // 135° olive green
        "#006494", // 225° ocean
        "#7B2D8B", // 315° purple
        "#E8401C", // 22°  orange-red
        "#8DB600", // 112° yellow-green
        "#1D96A0", // 202° cyan
        "#C9184A", // 292° rose
        "#E8A000", // 67°  amber
        "#2DC653", // 157° vivid green
        "#5C35CC", // 247° violet
        "#FF4D8C", // 337° pink
    };
    for (size_t i = 0; i < numLeagues; i++)
        leagueColors.push_back(palette[i % palette.size()]);
}

std::string HtmlVisualizer::getLeagueColor(int leagueIndex) const
{
    if (leagueIndex >= 0 && leagueIndex < static_cast<int>(leagueColors.size()))
        return leagueColors[leagueIndex];
    return "#888888";
}

// -----------------------------------------------------------------------
// Longest common substring across a set of team names
// -----------------------------------------------------------------------

std::string HtmlVisualizer::longestCommonSubstring(const std::vector<std::string>& names)
{
    if (names.size() < 2) return "";

    // Split each name into whitespace-delimited tokens
    auto tokenize = [](const std::string& s) {
        std::vector<std::string> tokens;
        std::istringstream ss(s);
        std::string tok;
        while (ss >> tok) tokens.push_back(tok);
        return tokens;
    };

    // Count how many names each token appears in (case-sensitive)
    std::map<std::string, int> freq;
    for (const auto& name : names) {
        // Use a set so a token appearing twice in one name counts once
        std::set<std::string> seen;
        for (const auto& tok : tokenize(name))
            if (seen.insert(tok).second)
                freq[tok]++;
    }

    // Pick the longest token that appears in at least 2 names;
    // must contain at least one alpha character
    std::string best;
    for (const auto& [tok, count] : freq) {
        if (count < 2) continue;
        bool hasAlpha = false;
        for (char c : tok) hasAlpha |= std::isalpha(static_cast<unsigned char>(c));
        if (!hasAlpha) continue;
        if (tok.size() > best.size()) best = tok;
    }

    return best;
}

// -----------------------------------------------------------------------
// Core SVG builder (shared by both map types)
// -----------------------------------------------------------------------

std::string HtmlVisualizer::buildMapSvgBody(
    const std::vector<ProjectedTeam>& projected,
    const KmBounds& bounds,
    int activeLeague,
    const EvaluationResults& results) const
{
    std::ostringstream s;
    const auto& leagues = results.getLeagues();

    // --- connection lines ---------------------------------------------------
    auto drawLines = [&](int li, double opacity, double strokeWidth) {
        const std::string color = getLeagueColor(li);
        for (const int a : leagues[li].teamIndices) {
            for (const int b : leagues[li].teamIndices) {
                if (b <= a) continue;
                auto [ax, ay] = kmToSvg(projected[a].x, projected[a].y, bounds);
                auto [bx, by] = kmToSvg(projected[b].x, projected[b].y, bounds);
                s << "  <line"
                  << " x1=\"" << ax << "\" y1=\"" << ay << "\""
                  << " x2=\"" << bx << "\" y2=\"" << by << "\""
                  << " stroke=\"" << color << "\""
                  << " stroke-width=\"" << strokeWidth << "\""
                  << " stroke-opacity=\"" << opacity << "\""
                  << " />\n";
            }
        }
    };

    if (activeLeague < 0) {
        // Overall map: draw all leagues
        for (int li = 0; li < static_cast<int>(leagues.size()); li++)
            drawLines(li, 0.55, 1.5);
    } else {
        // League map: draw only the active league
        drawLines(activeLeague, 0.65, 1.8);
    }

    // --- team dots: group by position, draw pies for shared locations -------

    // Merge projected points within mergeRadius pixels into one bucket.
    // Uses a simple greedy approach: first point seen defines the bucket centre.
    const double mergeRadius = 5;
    std::vector<std::pair<double,double>> bucketCentres;
    std::map<int, std::vector<int>> byPixel;  // bucket index -> team indices

    for (int i = 0; i < static_cast<int>(projected.size()); i++) {
        auto [px, py] = kmToSvg(projected[i].x, projected[i].y, bounds);

        // Find an existing bucket within mergeRadius
        int found = -1;
        for (int b = 0; b < static_cast<int>(bucketCentres.size()); b++) {
            double dx = px - bucketCentres[b].first;
            double dy = py - bucketCentres[b].second;
            if (std::sqrt(dx*dx + dy*dy) <= mergeRadius) { found = b; break; }
        }
        if (found < 0) {
            found = static_cast<int>(bucketCentres.size());
            bucketCentres.push_back({px, py});
        }
        byPixel[found].push_back(i);
    }

    // Draw one arc per slice of the pie; for a single team just draw a plain circle
    auto drawPie = [&](double cx, double cy, double r,
                       const std::vector<std::string>& colors)
    {
        const int n = static_cast<int>(colors.size());
        if (n == 1) {
            s << "  <circle cx=\"" << cx << "\" cy=\"" << cy << "\""
              << " r=\"" << r << "\""
              << " fill=\"" << colors[0] << "\" fill-opacity=1.0\""
              << " stroke=\"white\" stroke-width=\"1\"/>\n";
            return;
        }
        // SVG arc path per slice
        const double tau = 2.0 * M_PI;
        for (int i = 0; i < n; i++) {
            double a0 = tau * i       / n - M_PI_2;
            double a1 = tau * (i + 1) / n - M_PI_2;
            double x0 = cx + r * std::cos(a0),  y0 = cy + r * std::sin(a0);
            double x1 = cx + r * std::cos(a1),  y1 = cy + r * std::sin(a1);
            // large-arc-flag = 1 if slice > 180°
            int largeArc = (a1 - a0 > M_PI) ? 1 : 0;
            s << "  <path d=\"M " << cx << " " << cy
              << " L " << x0 << " " << y0
              << " A " << r << " " << r << " 0 " << largeArc << " 1 " << x1 << " " << y1
              << " Z\""
              << " fill=\"" << colors[i] << "\" fill-opacity=1.0\""
              << " stroke=\"white\" stroke-width=\"0.5\"/>\n";
        }
        // Outer ring to unify the slices visually
        s << "  <circle cx=\"" << cx << "\" cy=\"" << cy << "\""
          << " r=\"" << r << "\""
          << " fill=\"none\" stroke=\"white\" stroke-width=\"1\"/>\n";
    };

    // Two passes so grey pies are always below colored ones
    for (int pass = 0; pass < 2; pass++) {
        for (const auto& [pixel, indices] : byPixel) {
            // Collect active/inactive slices for this location
            std::vector<std::string> colors;
            bool anyActive = false;

            for (int i : indices) {
                const int li  = projected[i].leagueIndex;
                bool isActive = (activeLeague < 0) || (li == activeLeague);
                anyActive    |= isActive;
                colors  .push_back(isActive ? getLeagueColor(li) : "#cccccc");
            }

            if ((pass == 0) == anyActive) continue;  // colored pies on pass 1

            auto [px, py] = kmToSvg(projected[indices[0]].x, projected[indices[0]].y, bounds);
            double r      = anyActive ? 5.0 : 3.5;
            drawPie(px, py, r, colors);
        }
    }

    // --- collect labels for multi-team locations, drawn after all dots ------
    struct PendingLabel {
        double x, y, r;
        std::string text;
    };
    std::vector<PendingLabel> labels;

    for (const auto& [pixel, indices] : byPixel) {
        if (indices.size() < 2) continue;

        std::vector<std::string> names;
        for (int i : indices) names.push_back(results.getTeamMetrics()[i].teamName);
        std::string town = longestCommonSubstring(names);
        if (town.empty()) continue;

        auto [px, py] = kmToSvg(projected[indices[0]].x, projected[indices[0]].y, bounds);
        double r      = 5.0;
        labels.push_back({px, py, r, town});
    }

    // Disambiguation: if the same town name appears more than once, append a
    // club prefix — the longest token that appears before the town name in all
    // team names at that location and differs between locations.
    // e.g. "London" x2  ->  "London (FC)" and "London (TV)"
    {
        // Find duplicate town names
        std::map<std::string, std::vector<int>> byTown;
        for (int i = 0; i < static_cast<int>(labels.size()); i++)
            byTown[labels[i].text].push_back(i);

        for (auto& [town, labelIndices] : byTown) {
            if (labelIndices.size() < 2) continue;

            for (int li : labelIndices) {
                // Rebuild the names for this location from byPixel
                // by finding the pixel bucket that matches this label position
                std::vector<std::string> names;
                for (const auto& [pixel, indices] : byPixel) {
                    if (indices.size() < 2) continue;
                    auto [px, py] = kmToSvg(projected[indices[0]].x, projected[indices[0]].y, bounds);
                    if (std::abs(px - labels[li].x) < 0.5 && std::abs(py - labels[li].y) < 0.5) {
                        for (int i : indices)
                            names.push_back(results.getTeamMetrics()[i].teamName);
                        break;
                    }
                }

                // Find the token immediately before the town name, shared by all names here
                std::string prefix;
                for (const auto& name : names) {
                    size_t pos = name.find(town);
                    if (pos == std::string::npos) continue;
                    // Walk backwards past spaces to find the preceding token
                    size_t end = (pos > 0 && name[pos-1] == ' ') ? pos - 1 : pos;
                    size_t start = name.find_last_of(' ', end > 0 ? end - 1 : 0);
                    std::string tok = name.substr(
                        start == std::string::npos ? 0 : start + 1,
                        start == std::string::npos ? end : end - start - 1);
                    bool hasAlpha = false;
                    for (char c : tok) hasAlpha |= std::isalpha(static_cast<unsigned char>(c));
                    if (!hasAlpha) continue;
                    if (prefix.empty()) { prefix = tok; }
                    else if (prefix != tok) { prefix = ""; break; }  // inconsistent: give up
                }

                if (!prefix.empty())
                    labels[li].text = town + " (" + prefix + ")";
            }
        }
    }

    // Density filter: if a label's location has >= 2 other labels within
    // 60px, only keep it if it represents 3+ co-located teams (i.e. is
    // the "most important" dot in a crowded area).
    {
        const double threshold = 60.0;
        // Count co-located teams per label (proxy for importance)
        std::vector<int> teamCount(labels.size(), 1);
        for (int i = 0; i < static_cast<int>(labels.size()); i++) {
            for (const auto& [pixel, indices] : byPixel) {
                auto [px, py] = kmToSvg(projected[indices[0]].x, projected[indices[0]].y, bounds);
                if (std::abs(px - labels[i].x) < 0.5 && std::abs(py - labels[i].y) < 0.5) {
                    teamCount[i] = static_cast<int>(indices.size());
                    break;
                }
            }
        }

        // Sort label indices by team count descending (most important first)
        std::vector<int> order(labels.size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](int a, int b){
            return teamCount[a] > teamCount[b];
        });

        std::vector<bool> keep(labels.size(), true);
        for (int ii = 0; ii < static_cast<int>(order.size()); ii++) {
            int i = order[ii];
            if (!keep[i]) continue;  // already suppressed
            // Suppress all lower-priority labels within threshold
            for (int jj = ii + 1; jj < static_cast<int>(order.size()); jj++) {
                int j = order[jj];
                if (!keep[j]) continue;
                double dx = labels[i].x - labels[j].x;
                double dy = labels[i].y - labels[j].y;
                if (std::sqrt(dx*dx + dy*dy) < threshold)
                    keep[j] = false;
            }
        }

        // If disambiguation made "London (FC)" but all other "London X" labels
        // were suppressed, simplify back to just "London"
        std::map<std::string, int> keptByTown;
        for (int i = 0; i < static_cast<int>(labels.size()); i++) {
            if (!keep[i]) continue;
            // Extract the base town name (before any " (...")
            std::string base = labels[i].text;
            size_t paren = base.find(" (");
            if (paren != std::string::npos) base = base.substr(0, paren);
            keptByTown[base]++;
        }
        for (int i = 0; i < static_cast<int>(labels.size()); i++) {
            if (!keep[i]) continue;
            size_t paren = labels[i].text.find(" (");
            if (paren == std::string::npos) continue;
            std::string base = labels[i].text.substr(0, paren);
            if (keptByTown[base] == 1)
                labels[i].text = base;  // only survivor: drop the suffix
        }

        // Draw surviving labels on top of everything else
        for (int i = 0; i < static_cast<int>(labels.size()); i++) {
            if (!keep[i]) continue;
            const auto& lb = labels[i];
            // White halo for readability
            s << "  <text"
              << " x=\"" << (lb.x + lb.r + 3) << "\""
              << " y=\"" << (lb.y + 4) << "\""
              << " font-size=\"10\" font-family=\"Arial, sans-serif\""
              << " stroke=\"white\" stroke-width=\"3\""
              << " stroke-opacity=\"0.75\" paint-order=\"stroke\""
              << " fill=\"#333\">"
              << escapeHtml(lb.text)
              << "</text>\n";
        }
    }

    // create scale legend
    {
        // Pick a round number of km that gives a line between 50-150px wide
        const double kmRange = (bounds.maxX - bounds.minX);
        const double pxPerKm = bounds.svgWidth / kmRange;

        // Find a nice round km value
        const std::vector<double> candidates = {1, 2, 5, 10, 20, 50, 100, 200, 500};
        double scaleKm = candidates[0];
        for (double c : candidates) {
            double px = c * pxPerKm;
            if (px >= 50 && px <= 150) { scaleKm = c; break; }
            if (px < 50) scaleKm = c;  // keep updating until we find one in range
        }
        double scalePx = scaleKm * pxPerKm;

        // Position: bottom right with some margin
        double margin = 15.0;
        double x1 = bounds.svgWidth  - margin - scalePx;
        double x2 = bounds.svgWidth  - margin;
        double y  = bounds.svgHeight - margin;

        // Line with end ticks
        s << "  <line x1=\"" << x1 << "\" y1=\"" << y << "\""
          << " x2=\"" << x2 << "\" y2=\"" << y << "\""
          << " stroke=\"#333\" stroke-width=\"1.5\"/>\n";
        s << "  <line x1=\"" << x1 << "\" y1=\"" << (y - 4) << "\""
          << " x2=\"" << x1 << "\" y2=\"" << (y + 4) << "\""
          << " stroke=\"#333\" stroke-width=\"1.5\"/>\n";
        s << "  <line x1=\"" << x2 << "\" y1=\"" << (y - 4) << "\""
          << " x2=\"" << x2 << "\" y2=\"" << (y + 4) << "\""
          << " stroke=\"#333\" stroke-width=\"1.5\"/>\n";

        // Label centered above the line
        std::ostringstream kmLabel;
        kmLabel << static_cast<int>(scaleKm) << " km";
        double xMid = (x1 + x2) / 2.0;
        s << "  <text"
          << " x=\"" << xMid << "\" y=\"" << (y - 7) << "\""
          << " font-size=\"10\" font-family=\"Arial, sans-serif\""
          << " text-anchor=\"middle\""
          << " stroke=\"white\" stroke-width=\"3\""
          << " stroke-opacity=\"0.75\" paint-order=\"stroke\""
          << " fill=\"#333\">"
          << kmLabel.str()
          << "</text>\n";
    }

    return s.str();
}

// -----------------------------------------------------------------------
// Overall and per-league SVG wrappers
// -----------------------------------------------------------------------

std::string HtmlVisualizer::generateOverallMapSvg(
    const EvaluationResults&          results,
    const std::vector<ProjectedTeam>& projected) const
{
    const KmBounds bounds = computeKmBounds(projected, 800);
    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\""
        << " width=\""  << bounds.svgWidth  << "\""
        << " height=\"" << bounds.svgHeight << "\""
        << " class=\"overall-map\">\n"
        << "  <rect width=\"" << bounds.svgWidth << "\" height=\"" << bounds.svgHeight
        << "\" fill=\"#f8f9fa\" stroke=\"#bdc3c7\" stroke-width=\"1\"/>\n"
        << buildMapSvgBody(projected, bounds, -1, results)
        << "</svg>\n";
    return svg.str();
}

std::string HtmlVisualizer::generateLeagueMapSvg(
    const EvaluationResults&          results,
    const std::vector<ProjectedTeam>& projected,
    int                               leagueIndex) const
{
    // Use the same global bounds so all mini-maps are spatially comparable
    const KmBounds bounds = computeKmBounds(projected, 400);
    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\""
        << " width=\""  << bounds.svgWidth  << "\""
        << " height=\"" << bounds.svgHeight << "\""
        << " class=\"league-map\">\n"
        << "  <rect width=\"" << bounds.svgWidth << "\" height=\"" << bounds.svgHeight
        << "\" fill=\"#f8f9fa\" stroke=\"#bdc3c7\" stroke-width=\"1\"/>\n"
        << buildMapSvgBody(projected, bounds, leagueIndex, results)
        << "</svg>\n";
    return svg.str();
}

// -----------------------------------------------------------------------
// HTML sections (pass projected through)
// -----------------------------------------------------------------------

std::string HtmlVisualizer::generateOverallMapSection(
    const EvaluationResults&          results,
    const std::vector<ProjectedTeam>& projected) const
{
    std::ostringstream html;
    html << "    <h2>Gesamt-Übersicht aller Staffeln</h2>\n"
         << "    <div class=\"map-section\">\n"
         << generateOverallMapSvg(results, projected)
         << "    </div>\n";
    return html.str();
}

std::string HtmlVisualizer::generateTeamListSection(
    const EvaluationResults&          results,
    const std::vector<ProjectedTeam>& projected) const
{
    std::ostringstream html;
    html << "    <h2>Staffeln und Teamlisten</h2>\n";
    for (int i = 0; i < results.getLeagueCount(); i++)
        html << generateLeagueSection(results, projected, i);
    return html.str();
}

std::string HtmlVisualizer::generateLeagueSection(
    const EvaluationResults&          results,
    const std::vector<ProjectedTeam>& projected,
    int                               leagueIndex) const
{
    const auto& leagues = results.getLeagues();
    if (leagueIndex < 0 || leagueIndex >= static_cast<int>(leagues.size()))
        return "";

    const auto& league = leagues[leagueIndex];
    std::ostringstream html;

    html << "    <div class=\"league-section\">\n"
         << "      <div class=\"league-header\">\n"
         << "        <div class=\"league-color\" style=\"background-color: "
         << getLeagueColor(leagueIndex) << ";\"></div>\n"
         << "        <span>Staffel " << (leagueIndex + 1) << ": "
         << league.teamNames.size() << " Teams</span>\n"
         << "      </div>\n"
         << "      <div style=\"margin: 10px 0;\">\n"
         << generateLeagueMapSvg(results, projected, leagueIndex)
         << "      </div>\n";

    // Stats grid
    html << "      <div class=\"stats\">\n";
    auto stat = [&](const std::string& label, const std::string& value) {
        html << "        <div class=\"stat-item\">\n"
             << "          <div class=\"stat-label\">" << label << "</div>\n"
             << "          <div class=\"stat-value\">" << value << "</div>\n"
             << "        </div>\n";
    };
    auto km = [](double v) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(1) << v << " km";
        return o.str();
    };
    stat("Teams",                              std::to_string(league.teamNames.size()));
    stat("Gesamtentfernung",                   km(league.totalLeagueDistance));
    stat("Max. Reiseentfernung (Gesamtsaison)",km(league.maxTravel));
    stat("Ø Reiseentfernung (Gesamtsaison)",   km(league.avgTravel));
    stat("Max. Reiseentfernung (Spieltag)",    km(league.maxSingleDistance));
    stat("Ø Reiseentfernung (Spieltag)",       km(league.avgSingleDistance));
    html << "      </div>\n";

    // Team table
    html << "      <table>\n"
         << "        <tr><th>#</th><th>Team</th><th>Adresse</th></tr>\n";
    for (size_t i = 0; i < league.teamNames.size(); i++) {
        html << "        <tr>\n"
             << "          <td>" << (i + 1) << "</td>\n"
             << "          <td>" << escapeHtml(league.teamNames[i]) << "</td>\n"
             << "          <td>" << escapeHtml(league.teamAddresses[i]) << "</td>\n"
             << "        </tr>\n";
    }
    html << "      </table>\n"
         << "    </div>\n";

    return html.str();
}

// -----------------------------------------------------------------------
// HTML header / footer (unchanged from original)
// -----------------------------------------------------------------------

std::string HtmlVisualizer::generateHtmlHeader(const EvaluationResults& results) const
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"de\">\n<head>\n"
         << "  <meta charset=\"UTF-8\">\n"
         << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "  <title>" << config.getLeagueIdentifier() << "einteilung</title>\n"
         << "  <style>\n"
         << "    body { font-family: Arial, sans-serif; line-height: 1.6; margin: 20px; background: white; color: #333; }\n"
         << "    .container { max-width: 1200px; margin: 0 auto; }\n"
         << "    h1 { text-align: center; color: #2c3e50; border-bottom: 3px solid #3498db; padding-bottom: 10px; }\n"
         << "    h2 { color: #34495e; margin-top: 30px; border-left: 5px solid #3498db; padding-left: 10px; }\n"
         << "    h3 { color: #7f8c8d; font-size: 16px; }\n"
         << "    .summary { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin: 20px 0; }\n"
         << "    .summary-card { background: #ecf0f1; padding: 15px; border-radius: 5px; border-left: 4px solid #3498db; }\n"
         << "    .summary-card h3 { margin-top: 0; }\n"
         << "    .summary-value { font-size: 24px; font-weight: bold; color: #2c3e50; }\n"
         << "    .metrics { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 15px 0; }\n"
         << "    .metric { background: #f8f9fa; padding: 10px; border-radius: 3px; text-align: center; }\n"
         << "    .metric-name { font-weight: bold; color: #34495e; }\n"
         << "    .metric-description { font-size: 14px; color: #7f8c8d; margin: 0 10px; }\n"
         << "    .metric-value { font-size: 18px; color: #2c3e50; margin-top: 5px; }\n"
         << "    svg { border: 1px solid #bdc3c7; border-radius: 5px; background: white; }\n"
         << "    .map-section { margin: 30px 0; page-break-inside: avoid; }\n"
         << "    .league-section { page-break-inside: avoid; margin: 30px 0; background: #f8f9fa; padding: 20px; border-radius: 5px; }\n"
         << "    .league-header { font-size: 18px; font-weight: bold; margin-bottom: 15px; display: flex; align-items: center; }\n"
         << "    .league-color { display: inline-block; width: 20px; height: 20px; margin-right: 10px; border-radius: 3px; }\n"
         << "    table { width: 100%; border-collapse: collapse; margin: 15px 0; }\n"
         << "    th { background: #34495e; color: white; padding: 10px; text-align: left; }\n"
         << "    td { padding: 10px; border-bottom: 1px solid #bdc3c7; }\n"
         << "    tr:nth-child(even) { background: #f5f5f5; }\n"
         << "    .stats { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; margin: 10px 0; font-size: 13px; }\n"
         << "    .stat-item { background: white; padding: 8px; border-radius: 3px; }\n"
         << "    .stat-label { font-weight: bold; color: #7f8c8d; }\n"
         << "    .stat-value { color: #2c3e50; margin-top: 2px; }\n"
         << "    @media print { body { margin: 0; } .container { margin: 0; } }\n"
         << "  </style>\n</head>\n<body>\n  <div class=\"container\">\n"
         << "    <h1>" << config.getLeagueIdentifier() << "einteilung</h1>\n"
         << "    <p>Hinweis: Alle Entfernungsangaben sind Luftlinie, berechnet anhand der GPS-Koordinaten der Standorte.</p>\n";
    return html.str();
}

std::string HtmlVisualizer::generateSummarySection(const EvaluationResults& results) const
{
    std::ostringstream html;
    html << "    <div class=\"summary\">\n"
         << "      <div class=\"summary-card\"><h3>Gesamtzahl Teams</h3>"
         << "<div class=\"summary-value\">" << results.getTeamCount() << "</div></div>\n"
         << "      <div class=\"summary-card\"><h3>Staffeln</h3>"
         << "<div class=\"summary-value\">" << results.getLeagueCount() << "</div></div>\n"
         << "    </div>\n"
         << "    <h2>Kennzahlen</h2>\n"
         << "    <div class=\"metrics\">\n";

    for (const auto& m : results.getMetrics()) {
        html << "      <div class=\"metric\">\n"
             << "        <div class=\"metric-name\">"        << escapeHtml(m.name) << "</div>\n"
             << "        <div class=\"metric-description\">" << escapeHtml(config.getMetricDescription(m.name)) << "</div>\n"
             << "        <div class=\"metric-value\">"       << std::fixed << std::setprecision(2) << m.value;
        if (!m.unit.empty())
            html << " " << escapeHtml(m.unit);
        html << "</div>\n      </div>\n";
    }
    html << "    </div>\n";
    return html.str();
}

std::string HtmlVisualizer::generateHtmlFooter() const
{
    return "  </div>\n</body>\n</html>\n";
}

// -----------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------

std::string HtmlVisualizer::sanitizeTeamName(const std::string& name) const
{
    return name;
}

std::string HtmlVisualizer::escapeHtml(const std::string& text) const
{
    std::string r;
    for (char c : text) {
        switch (c) {
            case '&':  r += "&amp;";  break;
            case '<':  r += "&lt;";   break;
            case '>':  r += "&gt;";   break;
            case '"':  r += "&quot;"; break;
            case '\'': r += "&#39;";  break;
            default:   r += c;
        }
    }
    return r;
}

std::vector<int> HtmlVisualizer::selectStrategicTeamLabels(
    const std::vector<ProjectedTeam>& projected,
    int maxLabels) const
{
    // Simple spread heuristic: pick teams that are furthest from already-selected ones
    std::vector<int> selected;
    if (projected.empty() || maxLabels <= 0)
        return selected;

    selected.push_back(0);
    while (static_cast<int>(selected.size()) < maxLabels) {
        double bestDist = -1.0;
        int    bestIdx  = -1;
        for (int i = 0; i < static_cast<int>(projected.size()); i++) {
            double minD = std::numeric_limits<double>::max();
            for (int s : selected) {
                double dx = projected[i].x - projected[s].x;
                double dy = projected[i].y - projected[s].y;
                minD = std::min(minD, dx*dx + dy*dy);
            }
            if (minD > bestDist) { bestDist = minD; bestIdx = i; }
        }
        if (bestIdx < 0) break;
        selected.push_back(bestIdx);
    }
    return selected;
}