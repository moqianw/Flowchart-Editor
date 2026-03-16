cmake_minimum_required(VERSION 3.21)

project(FlowchartEditor LANGUAGES CXX)

include(GNUInstallDirs)


set(FLOWCHART_EDITOR_DEFAULT_QT_PREFIX "D:/Qt/6.9.3/msvc2022_64")
set(CMAKE_PREFIX_PATH "${FLOWCHART_EDITOR_DEFAULT_QT_PREFIX}" CACHE PATH "Qt installation prefix")



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

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg SvgWidgets Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network Svg SvgWidgets Xml)

qt_standard_project_setup()

set(FLOWCHART_EDITOR_SOURCES
    src/canvasitems.cpp
    src/canvasview.cpp
    src/commands.cpp
    src/componentextensionmanager.cpp
    src/customcomponentdialog.cpp
    src/diagramdocument.cpp
    src/diagramtypes.cpp
    src/documentserializer.cpp
    src/editorsession.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/mermaidexporter.cpp
    src/palettebutton.cpp
    src/peercollaborationmanager.cpp
    src/runtimepaths.cpp
    src/sessionpersistence.cpp
    src/shaperegistry.cpp
    include/canvasitems.h
    include/canvasview.h
    include/commands.h
    include/componentextensionmanager.h
    include/customcomponentdialog.h
    include/diagramdocument.h
    include/diagramtypes.h
    include/documentserializer.h
    include/editorsession.h
    include/mainwindow.h
    include/mermaidexporter.h
    include/palettebutton.h
    include/peercollaborationmanager.h
    include/runtimepaths.h
    include/sessionpersistence.h
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
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::SvgWidgets
    Qt${QT_VERSION_MAJOR}::Xml
)

install(TARGETS FlowchartEditor
    RUNTIME DESTINATION .
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/PROJECT_DESIGN.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/IMPLEMENTATION_DETAILS.md"
    DESTINATION docs
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/examples"
    DESTINATION .
)

if (WIN32)
    set(_qt_qmake_target "Qt${QT_VERSION_MAJOR}::qmake")
    if (TARGET "${_qt_qmake_target}")
        get_target_property(_qt_qmake_executable "${_qt_qmake_target}" IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qt_qmake_executable}" DIRECTORY)
        set(FLOWCHART_EDITOR_WINDEPLOYQT_EXECUTABLE
            "${_qt_bin_dir}/windeployqt.exe"
            CACHE FILEPATH "Path to windeployqt"
            FORCE
        )
    endif()

    if (FLOWCHART_EDITOR_WINDEPLOYQT_EXECUTABLE AND EXISTS "${FLOWCHART_EDITOR_WINDEPLOYQT_EXECUTABLE}")
        add_custom_command(TARGET FlowchartEditor POST_BUILD
            COMMAND "${FLOWCHART_EDITOR_WINDEPLOYQT_EXECUTABLE}"
                --dir "$<TARGET_FILE_DIR:FlowchartEditor>"
                $<IF:$<CONFIG:Debug>,--debug,--release>
                --force
                "$<TARGET_FILE:FlowchartEditor>"
            COMMENT "Deploying Qt runtime dependencies with windeployqt"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt was not found. The built executable may still require Qt DLLs from PATH.")
    endif()

    qt_generate_deploy_app_script(
        TARGET FlowchartEditor
        OUTPUT_SCRIPT FLOWCHART_EDITOR_DEPLOY_SCRIPT
        NO_UNSUPPORTED_PLATFORM_ERROR
    )

    if (FLOWCHART_EDITOR_DEPLOY_SCRIPT)
        install(SCRIPT "${FLOWCHART_EDITOR_DEPLOY_SCRIPT}")
    endif()
endif()

set(FLOWCHART_EDITOR_PORTABLE_PREFIX "${CMAKE_BINARY_DIR}/portable")
set(FLOWCHART_EDITOR_INSTALL_CONFIG_ARGS)
if (CMAKE_CONFIGURATION_TYPES)
    set(FLOWCHART_EDITOR_PORTABLE_PREFIX "${CMAKE_BINARY_DIR}/portable/$<CONFIG>")
    list(APPEND FLOWCHART_EDITOR_INSTALL_CONFIG_ARGS --config "$<CONFIG>")
endif()

add_custom_target(package-portable
    COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" ${FLOWCHART_EDITOR_INSTALL_CONFIG_ARGS} --prefix "${FLOWCHART_EDITOR_PORTABLE_PREFIX}"
    DEPENDS FlowchartEditor
    COMMENT "Building a portable runtime package"
    VERBATIM
)

