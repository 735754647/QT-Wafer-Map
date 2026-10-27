# QT Wafer Map

中文 | [English](README.en.md)

一个基于 Qt/C++ 的晶圆地图查看、编辑与路径规划工具，支持文本、CSV 和 Excel 地图，也提供可复用的 `WaferMapWindow` 地图窗口。

![QT Wafer Map](screenshot.png)

## 目录

- [项目简介](#项目简介)
- [核心功能](#核心功能)
- [界面模块](#界面模块)
- [运行环境](#运行环境)
- [快速开始](#快速开始)
- [基本使用流程](#基本使用流程)
- [支持的输入格式](#支持的输入格式)
- [在其他代码中调用](#在其他代码中调用)
- [加载参数说明](#加载参数说明)
- [实物坐标映射](#实物坐标映射)
- [信号与坐标约定](#信号与坐标约定)
- [目录结构](#目录结构)
- [开发说明](#开发说明)
- [已知限制](#已知限制)
- [许可证](#许可证)

## 项目简介

QT Wafer Map 面向需要查看、验证和编辑文本型 wafer map 的工程师与开发者。程序将地图中的 bin 数据转换为可交互的网格，提供大图、局部小图、坐标刻度、bin 统计、对点、寻起点、蛇形路径规划和地图编辑等功能。

项目重点是处理以行列和 bin 值表示的晶圆地图数据，适合用于：

- 地图文件解析和格式确认。
- 点位、行列坐标核对。
- 加工路径和起点验证。
- 现场地图数据检查。
- 作为其他 Qt 程序中的地图窗口组件。

## 核心功能

- **多格式加载**：普通文本、`.map`、`.csv`、`.xlsx`、`.xlsm`、`.xltx`。
- **多种加载方式**：默认自动加载、自定义文本范围、CSV 列映射、Excel 工作表/单元格范围。
- **地图查看**：大图和局部小图联动，支持滚轮缩放、拖拽平移、坐标刻度和当前点定位。
- **方向变换**：支持 0°、90°、180°、270° 显示旋转。
- **bin 统计**：显示各 bin 的颜色、总数量、已加工数量和剩余数量，并可勾选参与路径规划的目标 bin。
- **对点与寻起点**：设置起点、参考点和多个验证点，按路线逐格执行寻起点。
- **蛇形路径规划**：支持 8 种横向或纵向蛇形行进方向，并显示普通路径和借助路径。
- **地图编辑**：支持点选、框选和圆选，可修改选区 bin 值或裁剪选区。
- **手动扫码查找**：根据扫码内容在指定目录或磁盘范围内搜索地图文件。
- **可复用 API**：支持文件加载、内存数据加载、地图查询、选中、旋转、对点、路径规划和状态信号。
- **稳定配色**：bin 颜色使用固定调色板，文字根据背景亮度自动切换黑白，方便反复对比。
- **实物坐标映射**：可把原始行列换算成实物 X/Y，并在地图信息中显示。
- **刷新耗时**：地图重绘后在界面底部状态标签显示本次刷新耗时，便于观察大图性能。

## 界面模块

| 模块 | 作用 |
| --- | --- |
| 大图 | 查看完整地图、缩放、平移、选择坐标；红色框表示当前选中格 |
| 小图 | 查看当前点附近的 15×15 局部地图 |
| bin 表格 | 查看 bin 颜色、总数、已加工数和剩余数，并选择加工目标 |
| Map 地图设置 | 选择加载方式、加载文件、旋转地图和恢复视图 |
| Map 地图对点 | 设置行进方向、起点、参考点、验证点和寻起点 |
| Map 路径规划 | 启动或停止蛇形路径规划 |
| Map 编辑 | 按点、矩形或圆形范围修改地图 |

## 运行环境

- Windows 为主要验证平台。
- Qt 5.12+；同时兼容 Qt 6 的代码路径（Qt 6 使用 C++17，Qt 5 使用 C++11）。
- 支持 MSVC 和 MinGW；默认按 MSVC 配置。
- Qt 5.15.2 和 Qt 6.5.3 均已编译验证；Qt 6.6+ 的私有头路径已按版本兼容，但未在本机验证。
- Excel 读取依赖 Qt 私有头：Qt 5 和 Qt 6.0–6.5 使用 `QtGui/private/qzipreader_p.h`，Qt 6.6+ 使用 `QtCore/private/qzipreader_p.h`，需要安装与 Qt 版本匹配的 private headers。
- 在 Windows 下建议保持源码 UTF-8 编码；CMake/qmake 工程已经启用 `/utf-8`，公开头文件也带 UTF-8 BOM。

## 快速开始

### 使用 qmake

```powershell
git clone https://github.com/735754647/QT-Wafer-Map.git
cd QT-Wafer-Map\MapTest

qmake QTWaferMap.pro
nmake          # MinGW 使用 mingw32-make
.\bin\QTWaferMap.exe
```

### 使用 CMake（可选）

在仓库根目录执行：

```powershell
cmake -S MapTest -B build -G "Visual Studio 17 2022" -A Win32 ^
  -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019

cmake --build build --config Release
.\build\bin\Release\QTWaferMap.exe
```

32 位 Qt 用 `-A Win32`，64 位 Qt 用 `-A x64`，必须和 Qt 的架构一致。如果使用 MinGW 或其他生成器，按对应 Qt Kit 调整即可。

### 启动时直接打开地图

```powershell
.\MapTest\bin\QTWaferMap.exe ".\地图文件\T1-2120.3070.txt"
```

### 保存界面截图

```powershell
.\MapTest\bin\QTWaferMap.exe ".\地图文件\T1-2120.3070.txt" --screenshot demo.png
```

截图模式会在窗口完成一次布局后保存图片并退出，适合无人值守检查；加载失败时不会弹出模态框，而是以非零退出码结束。

### 命令行参数

| 参数 | 说明 |
| --- | --- |
| `map` | 可选，启动时加载的地图文件 |
| `--csv` | 强制按 CSV 方式读取 |
| `--screenshot <file>` | 保存窗口截图后退出 |
| `--help` | 显示帮助 |
| `--version` | 显示版本 |

## 基本使用流程

1. 启动程序，在右侧“Map 地图设置”中选择加载方式。
2. 点击“加载文档”，选择地图文件。
3. 根据文件结构配置：
   - 文本/Map：起始行、起始列、结束行、结束列、bin 位数。
   - CSV：数据起始行、X 列、Y 列、bin 列。
   - Excel：工作表索引、单元格范围。
4. 在 bin 表格中确认统计结果，勾选参与加工的目标 bin。
5. 在“Map 地图对点”中设置行进方向、起点、参考点和验证点。
6. 点击“寻起点”执行寻起点路线；走到起点后点击“开始规划”执行蛇形路径规划。
7. 需要修改地图时，在“Map 编辑”中选择点选、框选或圆选，再修改 bin 或裁剪选区。
8. “旋转Map图”按 90° 旋转显示；“恢复视图”恢复缩放和视角。

大图支持鼠标滚轮缩放和拖拽平移。右侧“手动扫码”可以根据扫码文本搜索地图文件。

## 支持的输入格式

| 格式 | 加载方式 | 说明 |
| --- | --- | --- |
| `.txt` / `.map` | 默认加载 | 自动识别常见地图主体、行列范围和 bin 宽度 |
| `.txt` / `.map` | 自定义加载 | 手动指定起止行列和每个 bin 的字符数 |
| `.csv` | CSV 加载 | 指定数据起始行以及 X、Y、bin 所在列，列号 0-based |
| `.xlsx` | Excel 加载 | 指定工作表索引和单元格范围 |
| `.xlsm` / `.xltx` | Excel 加载 | 按 Office Open XML 读取 |
| 内存二维数据 | `setMapData()` | `QVector<QStringList>`，不需要先写文件 |

仓库中的 `地图文件/` 目录包含多种示例数据。不同厂商的地图布局可能不同，加载失败时请根据原始文件内容调整加载配置。

## 在其他代码中调用

`WaferMapWindow` 是一个自包含的 `QMainWindow`，可以直接作为程序主窗口，也可以嵌入其他 Qt 程序。

```cpp
#include <WaferMapWindow.h>

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    WaferMapWindow window;
    QString error;

    if (!window.loadMapFile("wafer.txt", &error)) {
        qWarning("load failed: %s", qPrintable(error));
        return 1;
    }

    window.selectDisplayCell(10, 20);                 // 0-based 显示坐标
    window.setMapDirection(LeftToRightTopToBottom);   // 蛇形方向
    window.show();

    return app.exec();
}
```

### 使用内存数据

```cpp
QVector<QStringList> rows;
rows << (QStringList() << "1" << "1" << "2")
     << (QStringList() << "1" << "3" << "2")
     << (QStringList() << "3" << "3" << " ");

QString error;
if (!window.setMapData(rows, &error)) {
    qWarning("load failed: %s", qPrintable(error));
}
```

空字符串和缺失的尾列会自动补成空格 bin。

### CMake 子目录集成

```cmake
add_subdirectory(path/to/QT-Wafer-Map/MapTest)

add_executable(my_tool main.cpp)
target_link_libraries(my_tool PRIVATE QTWaferMap::QTWaferMap)
```

> 目标名以实际工程为准：当前库 target 是 `QTWaferMap`，别名是 `QTWaferMap::QTWaferMap`。

如果只需要库，不想要桌面程序，可以只构建 `QTWaferMap` target。

## 加载参数说明

文件加载默认使用自动检测：

```cpp
QString error;
if (!window.loadMapFile("wafer.txt", &error)) {
    qWarning("load failed: %s", qPrintable(error));
}
```

需要无弹窗、完全可控时使用 `MapLoadOptions`：

```cpp
MapLoadOptions options;
options.mode = CustomLoadMode;
options.range.startRow = 0;       // 0-based，含起始行
options.range.endRow = 99;        // 0-based，含结束行
options.range.startColumn = 0;
options.range.endColumn = 99;
options.binCharCount = 1;         // 每个 bin 占用的字符数

QString error;
if (!window.loadMapFile("wafer.txt", options, &error)) {
    qWarning("load failed: %s", qPrintable(error));
}
```

CSV 示例：

```cpp
MapLoadOptions csv;
csv.mode = CsvLoadMode;
csv.csvDataStartRow = 1;  // 0-based；默认 1 表示跳过表头
csv.csvXColumn = 0;
csv.csvYColumn = 1;
csv.csvBinColumn = 2;
```

Excel 示例：

```cpp
MapLoadOptions excel;
excel.mode = ExcelLoadMode;
excel.excelSheetIndex = 0;
excel.range.startRow = 1;
excel.range.endRow = 100;
excel.range.startColumn = 1;
excel.range.endColumn = 80;
```

`MapLoadOptions` 的主要字段：

| 字段 | 说明 |
| --- | --- |
| `mode` | `DefaultLoadMode`、`CsvLoadMode`、`CustomLoadMode`、`ExcelLoadMode` |
| `range` | 闭区间，0-based；用于文本、自定义或 Excel 范围 |
| `binCharCount` | 文本/自定义模式下每个 bin 的字符数，范围 1–4 |
| `csvDataStartRow` | CSV 数据起始行，0-based |
| `csvXColumn` / `csvYColumn` / `csvBinColumn` | CSV X/Y/bin 列，0-based |
| `excelSheetIndex` | Excel 工作表索引，0-based |

`.xlsx`、`.xlsm`、`.xltx` 文件会自动走 Excel 读取器，即使调用方指定了其他模式。

## 实物坐标映射

晶圆地图的行列号通常还需要对应设备的实物 X/Y 坐标。程序支持线性映射：

```text
X = originX + sourceColumn × stepX
Y = originY + sourceRow × stepY
```

其中 `sourceColumn` / `sourceRow` 是原始地图的 0-based 行列号，和显示旋转无关。

### 外部绑定实物坐标

实际设备通常会直接提供每个 die 的物理 X/Y，调用方可以把这些精确坐标绑定到原始地图单元格：

```cpp
QVector<WaferMapPhysicalPoint> points;
points.append(WaferMapPhysicalPoint{QPoint(0, 0), QPointF(100.0, 200.0)});
points.append(WaferMapPhysicalPoint{QPoint(1, 0), QPointF(101.25, 200.0)});
points.append(WaferMapPhysicalPoint{QPoint(0, 1), QPointF(100.0, 198.75)});

window.setPhysicalCoordinateData(points);
QPointF currentXY = window.currentPhysicalCoordinate();
```

精确绑定的数据优先于线性映射；没有绑定的格子会回退到 `MapCoordinateMapping` 计算结果。

```cpp
window.clearPhysicalCoordinateData();
```

### 规则映射

如果地图是规则网格，也可以使用线性映射：

```cpp
MapCoordinateMapping mapping;
mapping.enabled = true;
mapping.originX = 10.0;
mapping.originY = 20.0;
mapping.stepX = 0.5;
mapping.stepY = -0.5;
mapping.unit = "mm";

window.setCoordinateMapping(mapping);
QPointF cellXY = window.sourceToPhysical(column, row);
```

`sourceColumn` / `sourceRow` 是原始地图的 0-based 行列号，和显示旋转无关。如果需要在界面显示 X/Y，设置 `mapping.enabled = true`。

## 信号与坐标约定

可监听的主要信号：

| 信号 | 触发时机 |
| --- | --- |
| `mapLoaded(QString filePath)` | 文件或内存地图加载成功；内存地图 path 为空 |
| `mapLoadFailed(QString errorMessage)` | 加载失败，错误信息也可从 `lastError()` 获取 |
| `currentCellChanged(int column, int row)` | 当前选中显示格改变 |
| `directionChanged(Direction direction)` | 行进方向改变 |
| `rotationChanged(int degrees)` | 显示旋转角度改变 |
| `mapEdited()` | 编辑操作修改了地图单元格 |

坐标约定：

- 所有公开坐标都是 0-based。
- `selectDisplayCell()` / `currentDisplayCell()` 使用当前旋转方向下的显示坐标。
- `sourceCellValue()` / `displayCellValue()` 分别查询原始地图和显示地图。
- `startPoint()`、`referencePoint()`、`verifyPoints()` 返回原始地图坐标。
- 显示旋转和行进方向是两套独立状态：旋转只影响显示，方向用于蛇形路径规划。

## 目录结构

```text
.
├── MapTest/
│   ├── include/              # 对外头文件
│   │   ├── MapTypes.h        # 枚举、加载参数、bin 统计类型
│   │   ├── QTWaferMapGlobal.h # 导出宏和版本号
│   │   └── WaferMapWindow.h  # 主窗口公开接口
│   ├── src/
│   │   ├── main.cpp          # 桌面程序入口
│   │   ├── WaferMapWindow.cpp # 地图加载、绘制、交互、路径规划
│   │   ├── WaferMapWindow.ui # 主界面布局
│   │   ├── RulerSlider.cpp   # 坐标刻度尺
│   │   └── RulerSlider.h
│   ├── CMakeLists.txt        # 可选 CMake 构建
│   └── QTWaferMap.pro        # qmake 工程
├── 地图文件/                 # 示例地图数据
├── screenshot.png            # 界面截图
├── .gitignore
├── LICENSE
├── README.md
└── README.en.md
```

## 开发说明

- 程序入口：`MapTest/src/main.cpp`。
- 主窗口和地图逻辑：`MapTest/src/WaferMapWindow.cpp`、`MapTest/include/WaferMapWindow.h`。
- 界面文件：`MapTest/src/WaferMapWindow.ui`。
- 坐标刻度尺：`MapTest/src/RulerSlider.*`。
- 公开数据类型和加载参数：`MapTest/include/MapTypes.h`。
- qmake 工程已经定义 `QTWAFERMAP_STATIC`；CMake 静态库也会自动传递该宏。
- CMake 会优先查找 Qt 6，找不到时回退到 Qt 5；Qt 6 使用 C++17，Qt 5 使用 C++11。
- Qt 6.6 起 `QZipReader` 位于 QtCore 私有头，源码和构建脚本已经按 Qt 版本选择路径。
- 大图缩小时使用一格一像素的 `QImage` 缓存，放大时才绘制文字和网格。
- 地图重绘后会更新底部状态栏的“刷新时间”，方便判断大图性能。
- 含中文的 C++ 源文件使用 UTF-8 BOM；MSVC 工程同时启用 `/utf-8`。

当前加载完成后会统一补齐地图矩阵、统计 bin 并刷新表格，因此文本、CSV、Excel 和内存数据共用同一套统计/显示逻辑。

## 已知限制

- Qt 6.5.3 已完成编译和启动验证；Qt 6.6+ 的 `QZipReader` 私有头路径已做条件兼容，但仍建议在目标 Qt 版本上再编译确认。
- Excel 只支持 Office Open XML，不支持旧版二进制 `.xls`。
- Excel 读取依赖 Qt 私有 API，升级 Qt 时需要检查 `QZipReader` 相关代码。
- `WaferMapWindow.cpp` 仍较大，加载、绘制和路径规划集中在一个编译单元；后续可以继续拆分。
- 当前组件是完整主窗口，不是轻量地图画布。
- 不负责从测量数据推导晶圆物理轮廓或工艺参数。

## 许可证

本项目采用 [MIT License](LICENSE)。
