# League Assignment Optimizer (Staffeleinteilung)

Automatically assigns football teams to leagues to minimize travel distances.


## Disclaimer

This project is WORK IN PROGRESS!

## What This Does

Given a list of teams with locations (GPS coordinates), this tool assigns them to multiple leagues while:
- **Minimizing travel distances** - Teams in the same league travel less
- **Preventing club duplicates** - Multiple teams from the same club are placed in different leagues
- **Creating balanced leagues** - Leagues are roughly equal in size

The optimizer uses iterative local search: it starts with a random assignment and repeatedly swaps teams between leagues if it improves the overall metric.

## Optimization Metrics

Choose the optimization goal based on your priorities:

- **total_distance** (default): Minimize the sum of all distances. Best for keeping total travel low across all teams.
- **max_team_distance**: Minimize the longest single distance between any two teams. Prevents extreme matchups.
- **max_travel_per_team**: Minimize the team that travels the most. Ensures no single team has an unfairly long schedule.

## Input Format

Create `Teamliste.csv` with headers and team data:

```csv
Team Name,Address,GPS X,GPS Y
1. FC Münklingen,Lehninger Weg; 71263 Weil der Stadt,48.774344830497895,8.818103820249467
AC Italia Markgröningen,Schwieberdinger Str.; 71706 Markgröningen,48.90074426433602,9.082369035125339
```

Columns:
- **Team Name**: Name of the team
- **Address**: Physical address (used for detecting teams from same club)
- **GPS X**: Latitude
- **GPS Y**: Longitude

Teams are considered to be from the same club if:
1. They share the same address (or very similar GPS coordinates)
2. Their names share a substring of at least 50% of the longer name (e.g., "AC Italia II" and "AC Italia Markgröningen II" are from same club)

## Configuration

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

### Available Optimization Metrics

- **`total_distance`**: Minimize sum of all distances (default)
- **`max_team_distance`**: Minimize maximum distance between any two teams
- **`max_travel_per_team`**: Minimize the team with highest total travel

## Running the Application

## Building

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

## Running

1. Prepare `Teamliste.csv` with your team data
2. Adjust `config.json` if needed (metric choice, penalty strength, etc.)
3. Run the optimizer:
   ```bash
   ./StaffelEinteilung
   ```
4. When prompted, enter maximum league size
5. Results written to `Staffelaufteilung.txt`

## Output Format

`Staffelaufteilung.txt` contains the league assignments:

```
# League 1
1. FC Münklingen    Lehninger Weg; 71263 Weil der Stadt    48.77, 8.82
AC Italia Markgröningen    Schwieberdinger Str.; 71706 Markgröningen    48.90, 9.08

# League 2
...
```

## How It Works

1. **Distance calculation**: Uses lat/long coordinates with haversine approximation
2. **Penalty system**: Same-club teams in same league incur a high penalty
3. **Local search**: Iteratively swaps teams between leagues, keeping swaps that improve the metric
4. **Multiple attempts**: Runs optimization from different random starting points, keeps the best result

The penalty for same-club assignments is configurable - increase it if teams are still being placed together, or decrease it to allow some flexibility.

