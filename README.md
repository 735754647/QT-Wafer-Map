# QT-Wafer-Map

中文 | [English](README.en.md)

一个基于 Qt/C++ 的晶圆地图查看、编辑与路径规划工具。

![QT-Wafer-Map](chip.png)

## 项目简介

QT-Wafer-Map 面向需要查看和验证文本型 wafer map 的工程师与开发者。程序将地图中的 bin 数据转换为可交互的网格视图，提供大图、局部小图、坐标刻度、bin 统计、对点、寻起点、蛇形路径规划和地图编辑等功能。

项目重点是处理以行列和 bin 值表示的晶圆地图数据，适合用于地图解析、点位确认、加工路径验证和现场数据检查。

## 核心功能

- **多格式加载**：普通文本、`.map`、`.csv`、`.xlsx`、`.xlsm` 和 `.xltx`。
- **多种加载方式**：默认加载、自定义文本范围加载、CSV 列映射加载和 Excel 工作表/单元格范围加载。
- **地图查看**：大图和局部小图联动，支持滚轮缩放、拖拽平移、坐标刻度和当前点定位。
- **方向变换**：支持 0°、90°、180° 和 270° 旋转。
- **bin 统计**：显示各 bin 的颜色、总数量、已加工数量和剩余数量，并可选择参与路径规划的目标 bin。
- **对点与寻起点**：设置起点、参考点和多个验证点，按路线逐格执行寻起点。
- **蛇形路径规划**：支持 8 种横向或纵向蛇形行进方向，并显示借助路径和当前规划状态。
- **地图编辑**：支持点选、框选和圆选，可修改选区 bin 值或裁剪选区。
- **手动扫码查找**：根据扫描内容在指定目录或磁盘范围内搜索并加载地图文件。

## 界面模块

| 模块 | 作用 |
| --- | --- |
| 大图 | 查看完整地图、缩放、平移和选择坐标 |
| 小图 | 查看当前点附近的局部地图 |
| bin 表格 | 查看 bin 颜色和数量，并选择加工目标 |
| Map 地图设置 | 选择加载方式、加载文件、旋转和恢复视图 |
| Map 地图对点 | 设置方向、起点、参考点、验证点和寻起点 |
| Map 路径规划 | 启动或停止蛇形路径规划 |
| Map 编辑 | 按点、矩形或圆形范围修改地图 |

## 运行环境

- Windows
- Qt 5.x
- Qt Creator 或 qmake
- 支持 C++11 的编译器，项目默认按 MSVC 配置

项目使用 Qt 私有头文件 `QtGui/private/qzipreader_p.h` 读取 Office Open XML 文件。编译时需要安装与 Qt 版本匹配的 Qt GUI 私有开发文件。

## 快速开始

### 使用 Qt Creator

1. 克隆仓库：

   ```powershell
   git clone https://github.com/735754647/QT-Wafer-Map.git
   ```

2. 使用 Qt Creator 打开 `MapTest/MapTest.pro`。
3. 选择 Qt 5 Kit 和编译器。
4. 配置、构建并运行项目。

### 使用 qmake

在 Windows 命令行中执行：

```powershell
git clone https://github.com/735754647/QT-Wafer-Map.git
cd QT-Wafer-Map\MapTest
qmake MapTest.pro
nmake
```

如果使用 MinGW，可将 `nmake` 替换为：

```powershell
mingw32-make
```

项目当前提供 qmake 工程，未提供 CMake 配置。

## 基本使用流程

1. 启动程序，在右侧选择地图加载方式。
2. 点击“加载文档”并选择地图文件。
3. 根据文件结构配置行列范围、bin 位数或 CSV 列映射。
4. 在 bin 表格中查看统计结果，并勾选需要参与加工的 bin。
5. 在“Map 地图对点”中设置行进方向、起点、参考点和验证点。
6. 点击“寻起点”执行寻起点路线；完成后点击“开始规划”执行路径规划。
7. 需要修改地图时，在“Map 编辑”中选择点选、框选或圆选，再修改 bin 或裁剪选区。

大图支持鼠标滚轮缩放和拖拽平移；“恢复视图”可以回到默认视图。“手动扫码”可以按输入内容搜索地图文件。

## 支持的输入格式

| 格式 | 加载方式 | 说明 |
| --- | --- | --- |
| `.txt` / `.map` | 默认加载或自定义加载 | 可自动识别常见地图主体，也可手动指定行列范围和 bin 值位数 |
| `.csv` | CSV 加载 | 配置数据起始行以及 X、Y、bin 所在列 |
| `.xlsx` | Excel 加载 | 配置工作表索引和单元格范围 |
| `.xlsm` / `.xltx` | Excel 加载 | 支持 Office Open XML 格式 |

仓库中的 `地图文件/` 目录包含多种示例数据。不同厂商的地图布局可能不同，加载时请根据原始文件内容调整配置。

## 仓库结构

```text
.
├── MapTest/       # Qt/qmake 工程和 C++ 源码
├── 地图文件/      # 示例晶圆地图数据
├── chip.png       # 项目截图
├── LICENSE        # MIT License
├── README.md      # 中文说明
└── README.en.md   # English documentation
```

## 开发说明

- 工程入口：`MapTest/main.cpp`
- Qt 工程文件：`MapTest/MapTest.pro`
- 主窗口与地图逻辑：`MapTest/maptest.cpp`、`MapTest/maptest.h`
- 界面文件：`MapTest/maptest.ui`
- 自定义刻度尺与地图绘制控件：`rulerslider.*`、`rulerwidget.*`、`mylabel.*`

## 已知限制

- 当前工程依赖 Windows API，主要面向 Windows 环境。
- Excel 支持 `.xlsx`、`.xlsm` 和 `.xltx`，不支持旧版二进制 `.xls`。
- 项目依赖 Qt 私有 API，升级 Qt 版本时可能需要调整 `QZipReader` 相关代码。
- 当前处理的是行列/bin 网格数据，不负责从测量数据推导晶圆物理轮廓或工艺参数。

## 参与贡献

欢迎通过 Issue 反馈无法解析的地图格式、路径规划问题或界面改进建议，也欢迎提交 Pull Request。提交代码时请尽量附上可脱敏的输入示例、复现步骤和 Qt/编译器版本。

## 许可证

本项目采用 [MIT License](LICENSE)。

