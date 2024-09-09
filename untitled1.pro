QT       += core gui
QT       +=svg
QT += svgwidgets
QT += xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    classStream.cpp\
    OperationRecord.cpp\
    graphcopy.cpp\
    base.cpp \
    main.cpp \
    mainwindow.cpp \
    view.cpp

HEADERS += \
    classStream.h\
    OperationRecord.h\
    graphcopy.h\
    base.h \
    mainwindow.h \
    view.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
RESOURCES += \
    img.qrc
