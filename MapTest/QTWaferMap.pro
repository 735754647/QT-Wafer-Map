QT += core gui widgets

# Qt 6 requires C++17. Qt 6.6 moved QZipReader to QtCore private headers.
greaterThan(QT_MAJOR_VERSION, 5): CONFIG += c++17
lessThan(QT_MAJOR_VERSION, 6): CONFIG += c++11
greaterThan(QT_MAJOR_VERSION, 5):greaterThan(QT_MINOR_VERSION, 5): QT += core-private
greaterThan(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 6): QT += gui-private
lessThan(QT_MAJOR_VERSION, 6): QT += gui-private

TEMPLATE = app
TARGET = QTWaferMap
CONFIG -= app_bundle

INCLUDEPATH += $$PWD/include $$PWD/src
DEFINES += QTWAFERMAP_STATIC

SOURCES += \
    src/main.cpp \
    src/WaferMapWindow.cpp \
    src/RulerSlider.cpp

HEADERS += \
    include/MapTypes.h \
    include/QTWaferMapGlobal.h \
    include/WaferMapWindow.h \
    src/RulerSlider.h

FORMS += src/WaferMapWindow.ui

# Keep intermediate files under build/qmake and the executable under bin.
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/qmake/obj
MOC_DIR = $$PWD/build/qmake/moc
UI_DIR = $$PWD/build/qmake/ui
RCC_DIR = $$PWD/build/qmake/rcc

INCLUDEPATH += $$UI_DIR

msvc: QMAKE_CXXFLAGS += /utf-8
