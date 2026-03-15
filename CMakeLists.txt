cmake_minimum_required(VERSION 3.21)

project(FlowchartEditor LANGUAGES CXX)

if(NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "D:/Qt/6.9.3/msvc2022_64")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg SvgWidgets Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg SvgWidgets Xml)

qt_standard_project_setup()

set(FLOWCHART_EDITOR_SOURCES
    src/canvasitems.cpp
    src/canvasview.cpp
    src/commands.cpp
    src/diagramdocument.cpp
    src/diagramtypes.cpp
    src/documentserializer.cpp
    src/editorsession.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/palettebutton.cpp
    src/shaperegistry.cpp
    include/canvasitems.h
    include/canvasview.h
    include/commands.h
    include/diagramdocument.h
    include/diagramtypes.h
    include/documentserializer.h
    include/editorsession.h
    include/mainwindow.h
    include/palettebutton.h
    include/shaperegistry.h
    ui/mainwindow.ui
    resources/img.qrc
)

qt_add_executable(FlowchartEditor WIN32 ${FLOWCHART_EDITOR_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${FLOWCHART_EDITOR_SOURCES})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT FlowchartEditor)

set_target_properties(FlowchartEditor PROPERTIES
    OUTPUT_NAME untitled1
)

target_include_directories(FlowchartEditor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(FlowchartEditor PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::SvgWidgets
    Qt${QT_VERSION_MAJOR}::Xml
)

