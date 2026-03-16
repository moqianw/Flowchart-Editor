#pragma once

#include <QString>

namespace flowchart {

class RuntimePaths {
public:
    static QString executableDirectory();
    static QString runtimeDataDirectory();
    static QString settingsFilePath();
    static QString recoveryFilePath();
    static QString componentsDirectory();
};

}  // namespace flowchart
