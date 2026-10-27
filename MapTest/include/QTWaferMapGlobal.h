#ifndef QTWAFERMAP_GLOBAL_H
#define QTWAFERMAP_GLOBAL_H

#include <QtCore/qglobal.h>

// 动态库导出宏：
// - 构建静态库或使用 qmake .pri 时定义 QTWAFERMAP_STATIC；
// - 构建动态库时定义 QTWAFERMAP_LIBRARY；
// - 其他工程引用头文件时自动视为导入。
#if defined(QTWAFERMAP_STATIC)
#  define QTWAFERMAP_EXPORT
#elif defined(QTWAFERMAP_LIBRARY)
#  define QTWAFERMAP_EXPORT Q_DECL_EXPORT
#else
#  define QTWAFERMAP_EXPORT Q_DECL_IMPORT
#endif

// 版本号需要与 CMake 的 project(VERSION ...) 保持一致。
#define QTWAFERMAP_VERSION_MAJOR 1
#define QTWAFERMAP_VERSION_MINOR 0
#define QTWAFERMAP_VERSION_PATCH 0
#define QTWAFERMAP_VERSION_STR "1.0.0"

#endif // QTWAFERMAP_GLOBAL_H
