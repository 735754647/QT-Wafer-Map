cmake_minimum_required(VERSION 3.16)

project(QTWaferMap VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src")

# 同时支持 Qt 5 和 Qt 6；优先使用 Qt 6。
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 可复用的地图窗口库。
add_library(QTWaferMap STATIC
    src/WaferMapWindow.cpp
    src/WaferMapWindow.ui
    src/RulerSlider.cpp
    src/RulerSlider.h
    include/MapTypes.h
    include/QTWaferMapGlobal.h
    include/WaferMapWindow.h
)

add_library(QTWaferMap::QTWaferMap ALIAS QTWaferMap)

target_include_directories(QTWaferMap
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(QTWaferMap
    PUBLIC Qt${QT_VERSION_MAJOR}::Widgets
)

# Qt 5 和 Qt 6.0-6.5 的 QZipReader 在 QtGui 私有头中；
# Qt 6.6 起移到 QtCore 私有头，需要链接 CorePrivate。
if(QT_VERSION_MAJOR GREATER_EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 6)
    target_link_libraries(QTWaferMap PRIVATE Qt${QT_VERSION_MAJOR}::CorePrivate)
else()
    target_link_libraries(QTWaferMap PRIVATE Qt${QT_VERSION_MAJOR}::GuiPrivate)
endif()

# 静态库消费者不再需要 DLL 导入声明。
target_compile_definitions(QTWaferMap PUBLIC QTWAFERMAP_STATIC)

if(MSVC)
    target_compile_options(QTWaferMap PRIVATE /utf-8)
endif()

# 桌面程序。
add_executable(QTWaferMapApp WIN32 src/main.cpp)
set_target_properties(QTWaferMapApp PROPERTIES
    OUTPUT_NAME QTWaferMap
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_link_libraries(QTWaferMapApp PRIVATE QTWaferMap)

if(MSVC)
    target_compile_options(QTWaferMapApp PRIVATE /utf-8)
endif()
