# League Assignment Optimizer (Staffeleinteilung)

A C++ application that automatically assigns football teams to leagues while minimizing travel distances.

## Disclaimer

Project is WIP!!
Readme and most of the code written by AI (ChatGPT & Claude); documentation below is partially outdated.

## Features

- **Multiple optimization metrics**: Choose between total distance, max distance, fairness (variance), and more
- **Configurable via JSON**: Easily customize algorithm parameters without recompiling
- **Modular architecture**: Clean separation of concerns for maintainability
- **GPS-based distance calculation**: Accurate haversine-approximation distance calculations
- **Flexible league sizing**: Automatically splits teams into balanced leagues

## Project Structure

```
StaffelEinteilung/
├── CMakeLists.txt              # Build configuration
├── config.json                 # Configuration file (adjustable parameters)
├── adressen.txt               # Team data (input file)
├── include/                   # Header files
│   ├── config.h              # Configuration management
│   ├── optimizer.h           # Multi-metric optimization engine
│   ├── team_data.h           # Team data structure
│   ├── file_io.h             # File I/O operations
│   ├── league_splitter.h     # League sizing logic
│   └── distance_calculator.h # GPS distance calculations
├── src/                       # Implementation files
│   ├── main.cpp              # Main program
│   ├── file_io.cpp           # File I/O implementation
│   ├── league_splitter.cpp   # League split implementation
│   └── distance_calculator.cpp # Distance calculation implementation
└── build/                     # Build output directory
```

## Building the Project

### Prerequisites
- CMake 3.12 or higher
- C++17 compatible compiler (MSVC, GCC, or Clang)

### Build Steps

**On Windows (with Visual Studio):**
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

**On Linux/macOS:**
```bash
mkdir build
cd build
cmake ..
make
```

The compiled executable will be in the `build/` directory.

## Input Data Format

Create an `adressen.txt` file with the following format (tab-separated):
```
Team Name    Address                          GPS_X         GPS_Y
1. FC Meier  Main Street 10, 70000 City      48.7758       8.6495
AC Italia    Side Road 20, 71000 Town        48.9007       9.0824
```

Each line must contain:
- Team name (tab)
- Address (tab)
- GPS coordinates as `latitude,longitude`

## Configuration

Edit `config.json` to customize:

```json
{
  "optimization": {
    "attempts": 100,              // Number of optimization runs
    "metric": "total_distance"    // Optimization metric
  },
  "geodesy": {
    "km_per_degree_latitude": 111.32,
    "max_distance_threshold": 1.0e10,
    "coordinate_precision": 1.0e-16
  },
  "debug": {
    "enabled": true,
    "verbose": true
  }
}
```

### Available Optimization Metrics

- **`total_distance`**: Minimize sum of all distances (default)
- **`max_team_distance`**: Minimize maximum distance between any two teams
- **`max_travel_per_team`**: Minimize the team with highest total travel
- **`avg_distance_per_team`**: Minimize average distance per team
- **`variance_distance`**: Minimize variance (fairness across teams)

## Running the Application

```bash
./build/StaffelEinteilung
```

The program will:
1. Load `config.json` (defaults used if missing)
2. Read team data from `adressen.txt`
3. Ask for maximum league size
4. Optionally ask for optimization metric choice (if debug enabled)
5. Run optimization
6. Write results to `Staffelaufteilung.txt`

## Output

Results are written to `Staffelaufteilung.txt` with the following format:

```
# League 1
Team A    Address A    48.7758, 8.6495
Team B    Address B    48.9007, 9.0824

# League 2
Team C    Address C    49.0415, 9.1234
...
```

## Architecture Benefits

- **Modular**: Each component has a single responsibility
- **Testable**: Isolated modules can be unit tested independently
- **Configurable**: All parameters in JSON, no recompilation needed
- **Extensible**: Easy to add new optimization metrics or visualization formats
- **Header-only parsing**: Minimal dependencies, ships as single standalone executable

## Future Enhancements

- SVG visualization of league assignments
- HTML report generation
- Interactive GUI (optional, planned)
- Export to Excel/CSV format
- Advanced optimization algorithms (genetic algorithms, simulated annealing)

