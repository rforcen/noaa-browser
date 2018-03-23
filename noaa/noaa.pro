#-------------------------------------------------
#
# Project created by QtCreator 2018-02-14T14:08:16
#
#-------------------------------------------------

QT       += core gui
QT       += charts datavisualization

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
ICON = noaa.icns
CONFIG += c++11
TARGET = noaa
TEMPLATE = app
INCLUDEPATH += /usr/local/Cellar/libarchive/3.3.2/include
LIBS += -L//usr/local/Cellar/libarchive/3.3.2/lib -larchive

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    tabledaily.cpp \
    daily.cpp \
    reader.cpp \
    targz.cpp \
    statistic.cpp \
    auxfile.cpp \
    tableauxfile.cpp \
    treegroup.cpp \
    treeitem.cpp \
    config.cpp \
    bitkey.cpp \
    comboboxmodel.cpp \
    sqlCompiler.cpp \
    leastsquarefit.cpp \
    charter.cpp

HEADERS += \
        mainwindow.h \
    tabledaily.h \
    daily.h \
    reader.h \
    targz.h \
    statistic.h \
    auxfile.h \
    tableauxfile.h \
    treegroup.h \
    treeitem.h \
    config.h \
    bitkey.h \
    comboboxmodel.h \
    sqlCompiler.h \
    leastsquarefit.h \
    charter.h

FORMS += \
        mainwindow.ui

RESOURCES += \
    auxfiles.qrc

DISTFILES += \
    snippets
