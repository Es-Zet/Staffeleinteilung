# League Assignment Optimizer (Staffeleinteilung)

Automatically assigns football teams to leagues to minimize travel distances.

You may find an English version of the summary below the German description.

## Disclaimer

**This project is WORK IN PROGRESS!**

The code and initial architecture were created using AI assistance (ChatGPT and Claude). While functional, it should be reviewed for production use. Moreover, this file was created by AI, too, and thus might contain inaccurate information.

---

## DEUTSCHE VERSION (German)

### Was macht das Tool?

Gegeben eine Liste von Teams mit Standorten (GPS-Koordinaten) weist dieses Tool die Teams mehreren Ligen zu und minimiert dabei die Fahrtdistanzen:
- **Fahrtdistanzen minimieren** - Teams in der gleichen Liga fahren kürzer
- **Vereine trennen** - Mehrere Teams vom gleichen Verein werden in verschiedene Ligen eingeteilt
- **Ausgewogene Ligen** - Ligen haben ähnliche Größe

Der Optimizer verwendet iterative lokale Suche: Er beginnt mit einer zufälligen Zuteilung und vertauscht wiederholt Teams zwischen Ligen, wenn dies die Gesamtmetrik verbessert.

### Optimierungsmetriken

Wählen Sie das Optimierungsziel basierend auf Ihren Prioritäten:

- **total_distance** (Standard): Minimiert die Summe aller Fahrtdistanzen. Beste Wahl, um die Gesamtbelastung zu reduzieren.
- **max_team_distance**: Minimiert die längste Einzelstrecke zwischen zwei Teams. Verhindert extreme Gegner-Paarungen.
- **max_travel_per_team**: Minimiert das Team mit der höchsten Gesamtfahrtdistanz. Stellt sicher, dass kein Team überproportional belastet wird.

### Eingabeformat

Erstellen Sie eine `Teamliste.csv` mit der folgenden Struktur:

```csv
Teamname, Adresse (optional), GPS Lat (Breite; N-S), GPS Long (Länge; O-W)
1. FC Münklingen, Lehninger Weg; 71263 Weil der Stadt, 48.774344830497895, 8.818103820249467
AC Italia Markgröningen, Schwieberdinger Str.; 71706 Markgröningen, 48.90074426433602, 9.082369035125339
```

Spalten:
- **Teamname**: Name des Teams
- **Adresse (optional)**: Physische Adresse (zur Erkennung von Teams des gleichen Vereins)
- **GPS Lat (Breite; N-S)**: Breitengrad (Nord-Süd)
- **GPS Long (Länge; O-W)**: Längengrad (Ost-West)

Teams werden als vom gleichen Verein zugehörig erkannt, wenn:
1. Sie die gleiche Adresse haben (oder sehr ähnliche GPS-Koordinaten)
2. Ihre Namen einen Teilstring von mindestens 50% der längeren Namens-Länge teilen (z.B. "AC Italia II" und "AC Italia Markgröningen II" sind vom gleichen Verein)

### Konfiguration

Bearbeiten Sie die `config.json`:

```json
{
  "optimization": {
    "attempts": 10,              // Anzahl der Optimierungsdurchläufe
    "metric": "total_distance"   // Welche Metrik optimiert werden soll
  },
  "league": {
    "prefer_even_sizes": true,
    "disallow_same_club_in_league": true,   // Verhindere gleiche Vereine in einer Liga
    "same_club_penalty": 1000.0             // Strafe für Verletzung
  },
  "geodesy": {
    "km_per_degree_latitude": 111.32,       // Umwandlungsfaktor für Lat/Lon
    "coordinate_precision": 1.0e-8          // Schwellenwert für doppelte Koordinaten
  },
  "debug": {
    "enabled": true,
    "verbose": true
  },
  "metrics": {
    "available": [
      "total_distance",
      "max_team_distance",
      "max_travel_per_team"
    ]
  }
}
```

**Wichtige Einstellungen:**
- `attempts`: Mehr Versuche = bessere Optimierung, aber längere Laufzeit
- `metric`: Wählen Sie, welches Optimierungsziel verwendet werden soll
- `disallow_same_club_in_league`: Auf `false` setzen, um mehrere Teams eines Vereins in einer Liga zu erlauben
- `same_club_penalty`: Höhere Strafe = stärkere Durchsetzung (muss größer als Distanzunterschiede sein)

### Kompilierung

**Anforderungen:**
- CMake 3.12+
- C++17-kompatibler Compiler (MSVC, GCC, Clang)

**Schritte:**
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Ausführbare Datei: `build/StaffelEinteilung.exe` (Windows) oder `build/StaffelEinteilung` (Linux/macOS)

### Ausführung

1. Bereiten Sie `Teamliste.csv` mit Ihren Teamdaten vor
2. Passen Sie `config.json` ggf. an (Metrik-Wahl, Strafstärke, etc.)
3. Starten Sie den Optimizer:
   ```bash
   ./StaffelEinteilung
   ```
4. Geben Sie auf Anfrage die maximale Ligengröße ein
5. Ergebnisse werden in `Staffelaufteilung.txt` geschrieben

### Ausgabeformat

`Staffelaufteilung.txt` enthält die Ligenzuteilungen:

```
# Liga 1
1. FC Münklingen    Lehninger Weg; 71263 Weil der Stadt    48.77, 8.82
AC Italia Markgröningen    Schwieberdinger Str.; 71706 Markgröningen    48.90, 9.08

# Liga 2
...
```

### Wie es funktioniert

1. **Distanzberechnung**: Verwendet Lat/Lon-Koordinaten mit Haversine-Approximation
2. **Straf-System**: Teams vom gleichen Verein in der gleichen Liga erhalten eine hohe Strafe
3. **Lokale Suche**: Vertauscht Teams iterativ zwischen Ligen und behält Vertauschungen, die die Metrik verbessern
4. **Mehrere Versuche**: Führt Optimierung von verschiedenen zufälligen Startpunkten durch und behält das beste Ergebnis

Die Strafe für Verein-Zuordnungen ist konfigurierbar - erhöhen Sie sie, wenn Teams weiterhin zusammengefasst werden, oder verringern Sie sie, um etwas Flexibilität zu erlauben.

---

## ENGLISH VERSION

### What This Does

Given a list of teams with locations (GPS coordinates), this tool assigns them to multiple leagues while:
- **Minimizing travel distances** - Teams in the same league travel less
- **Preventing club duplicates** - Multiple teams from the same club are placed in different leagues
- **Creating balanced leagues** - Leagues are roughly equal in size

The optimizer uses iterative local search: it starts with a random assignment and repeatedly swaps teams between leagues if it improves the overall metric.

### Optimization Metrics

Choose the optimization goal based on your priorities:

- **total_distance** (default): Minimize the sum of all distances. Best for keeping total travel low across all teams.
- **max_team_distance**: Minimize the longest single distance between any two teams. Prevents extreme matchups.
- **max_travel_per_team**: Minimize the team that travels the most. Ensures no single team has an unfairly long schedule.

### Input Format

Create `Teamliste.csv` with the following structure:

```csv
Teamname, Adresse (optional), GPS Lat (Breite; N-S), GPS Long (Länge; O-W)
1. FC Münklingen, Lehninger Weg; 71263 Weil der Stadt, 48.774344830497895, 8.818103820249467
AC Italia Markgröningen, Schwieberdinger Str.; 71706 Markgröningen, 48.90074426433602, 9.082369035125339
```

Columns:
- **Teamname**: Name of the team
- **Adresse (optional)**: Physical address (used for detecting teams from same club)
- **GPS Lat (Breite; N-S)**: Latitude (North-South)
- **GPS Long (Länge; O-W)**: Longitude (East-West)

Teams are considered to be from the same club if:
1. They share the same address (or very similar GPS coordinates)
2. Their names share a substring of at least 50% of the longer name (e.g., "AC Italia II" and "AC Italia Markgröningen II" are from same club)

### Configuration

Edit `config.json`:

```json
{
  "optimization": {
    "attempts": 10,              // Number of optimization attempts
    "metric": "total_distance"   // Which metric to optimize
  },
  "league": {
    "prefer_even_sizes": true,
    "disallow_same_club_in_league": true,   // Prevent same club teams together
    "same_club_penalty": 1000.0             // Penalty for violating this
  },
  "geodesy": {
    "km_per_degree_latitude": 111.32,       // Conversion factor for lat/lon
    "coordinate_precision": 1.0e-8          // Threshold for duplicate coords
  },
  "debug": {
    "enabled": true,
    "verbose": true
  },
  "metrics": {
    "available": [
      "total_distance",
      "max_team_distance",
      "max_travel_per_team"
    ]
  }
}
```

**Key settings:**
- `attempts`: More attempts = better optimization but slower runtime
- `metric`: Change which optimization goal to use
- `disallow_same_club_in_league`: Set to `false` to allow multiple teams from same club in one league
- `same_club_penalty`: Higher penalty = stronger enforcement (must be large enough to outweigh distance differences)

### Building

**Prerequisites:**
- CMake 3.12+
- C++17 compiler (MSVC, GCC, or Clang)

**Steps:**
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Executable: `build/StaffelEinteilung.exe` (Windows) or `build/StaffelEinteilung` (Linux/macOS)

### Running

1. Prepare `Teamliste.csv` with your team data
2. Adjust `config.json` if needed (metric choice, penalty strength, etc.)
3. Run the optimizer:
   ```bash
   ./StaffelEinteilung
   ```
4. When prompted, enter maximum league size
5. Results written to `Staffelaufteilung.txt`

### Output Format

`Staffelaufteilung.txt` contains the league assignments:

```
# League 1
1. FC Münklingen    Lehninger Weg; 71263 Weil der Stadt    48.77, 8.82
AC Italia Markgröningen    Schwieberdinger Str.; 71706 Markgröningen    48.90, 9.08

# League 2
...
```

### How It Works

1. **Distance calculation**: Uses lat/long coordinates with haversine approximation
2. **Penalty system**: Same-club teams in same league incur a high penalty
3. **Local search**: Iteratively swaps teams between leagues, keeping swaps that improve the metric
4. **Multiple attempts**: Runs optimization from different random starting points, keeps the best result

The penalty for same-club assignments is configurable - increase it if teams are still being placed together, or decrease it to allow some flexibility.
