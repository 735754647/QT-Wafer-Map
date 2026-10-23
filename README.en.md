# QT-Wafer-Map

[中文](README.md) | English

A Qt/C++ desktop application for viewing, editing, inspecting, and planning traversal paths on wafer maps.

![QT-Wafer-Map](chip.png)

## Overview

QT-Wafer-Map is designed for engineers and developers who need to inspect and validate text-based wafer maps. It converts bin data into an interactive grid and provides overview/local map views, coordinate rulers, bin statistics, point alignment, start-point discovery, serpentine path planning, and map editing.

The project focuses on wafer-map data represented by rows, columns, and bin values. It is intended for map parsing, coordinate verification, processing-path validation, and engineering data inspection.

## Features

- **Multiple input formats**: plain text, `.map`, `.csv`, `.xlsx`, `.xlsm`, and `.xltx`.
- **Configurable loading modes**: default loading, custom text ranges, CSV column mapping, and Excel worksheet/cell-range loading.
- **Interactive map viewer**: synchronized overview and local views with zooming, panning, coordinate rulers, and cell selection.
- **Map transforms**: rotate the displayed map by 0°, 90°, 180°, or 270°.
- **Bin statistics**: show bin colors, total counts, processed counts, and remaining counts; select target bins for path planning.
- **Point alignment and start-point discovery**: define a start point, one reference point, and multiple verification points, then execute the discovery route step by step.
- **Serpentine path planning**: support eight horizontal or vertical traversal directions and display assist-path cells and planning state.
- **Map editing**: edit a point, rectangle, or circular region; change bin values or crop a selected region.
- **Manual scan search**: search for a matching map file in a selected directory or across available drives.

## UI Modules

| Module | Purpose |
| --- | --- |
| Main map | Full-map view, zooming, panning, and coordinate selection |
| Local map | Local view around the current cell |
| Bin table | Bin colors and counts, plus target selection |
| Map Settings | Loading mode, file loading, rotation, and view reset |
| Point Alignment | Direction, start/reference/verification points, and start-point discovery |
| Path Planning | Start or stop serpentine path planning |
| Map Editing | Point, rectangle, or circular region editing |

## Requirements

- Windows
- Qt 5.x
- Qt Creator or qmake
- A C++11-capable compiler; the project is configured for MSVC by default

The project uses the Qt private header `QtGui/private/qzipreader_p.h` to read Office Open XML files. Install the matching Qt GUI private development files before building.

## Quick Start

### Qt Creator

1. Clone the repository:

   ```powershell
   git clone https://github.com/735754647/QT-Wafer-Map.git
   ```

2. Open `MapTest/MapTest.pro` in Qt Creator.
3. Select a Qt 5 kit and compiler.
4. Configure, build, and run the project.

### qmake

Run the following commands on Windows:

```powershell
git clone https://github.com/735754647/QT-Wafer-Map.git
cd QT-Wafer-Map\MapTest
qmake MapTest.pro
nmake
```

For MinGW, replace `nmake` with:

```powershell
mingw32-make
```

The repository currently provides a qmake project and does not include a CMake configuration.

## Typical Workflow

1. Start the application and select a map loading mode in the right-side panel.
2. Click “加载文档” and choose a map file.
3. Configure row/column ranges, bin width, or CSV column mapping when prompted.
4. Review the bin table and select the bins that should participate in processing.
5. Set the traversal direction, start point, reference point, and verification points in “Map 地图对点”.
6. Click “寻起点” to execute the start-point route, then click “开始规划” to run path planning.
7. To modify the map, use “Map 编辑” and choose point, rectangle, or circular selection before changing bin values or cropping the selection.

The main map supports mouse-wheel zooming and drag panning. “恢复视图” resets the view, while “手动扫码” searches for a map file from the entered scan text.

## Supported Input Formats

| Format | Loading mode | Notes |
| --- | --- | --- |
| `.txt` / `.map` | Default or custom loading | Detects common map bodies or allows explicit row/column ranges and bin width |
| `.csv` | CSV loading | Configure the data start row and the X, Y, and bin columns |
| `.xlsx` | Excel loading | Configure the worksheet index and cell range |
| `.xlsm` / `.xltx` | Excel loading | Supported as Office Open XML files |

The `地图文件/` directory contains sample maps in several formats. Map layouts vary between vendors, so adjust the loading options to match the source file.

## Repository Layout

```text
.
├── MapTest/       # Qt/qmake project and C++ sources
├── 地图文件/      # Sample wafer map data
├── chip.png       # Project screenshot
├── LICENSE        # MIT License
├── README.md      # Chinese documentation
└── README.en.md   # English documentation
```

## Development Notes

- Application entry point: `MapTest/main.cpp`
- Qt project file: `MapTest/MapTest.pro`
- Main window and map logic: `MapTest/maptest.cpp`, `MapTest/maptest.h`
- UI form: `MapTest/maptest.ui`
- Custom ruler and map widgets: `rulerslider.*`, `rulerwidget.*`, and `mylabel.*`

## Known Limitations

- The current project depends on Windows APIs and primarily targets Windows.
- Excel support covers `.xlsx`, `.xlsm`, and `.xltx`; legacy binary `.xls` files are not supported.
- The project uses Qt private APIs, so Qt upgrades may require changes around `QZipReader`.
- The application processes row/column/bin grid data. It does not derive physical wafer outlines or process parameters from measurement data.

## Contributing

Issues and pull requests are welcome. When reporting a parsing or path-planning problem, please include a sanitized input sample, reproduction steps, and the Qt/compiler versions used.

## License

This project is licensed under the [MIT License](LICENSE).

