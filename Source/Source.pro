######################################################################
# Automatically generated by qmake (3.1) Tue Aug 24 13:18:30 2021
######################################################################

QT += widgets
TEMPLATE = app
TARGET = ../egs_brachy_GUI
INCLUDEPATH += .

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += egsinp.h \
           egsphant.h \
           file_selector.h \
           metrics.h \
           options.h \
           parse_dicom.h \
           preview.h \
           priority.h \
           read_dose.h \
           tissue_check.h \
           trim.h
SOURCES += database.cpp \
           egsinp.cpp \
           egsphant.cpp \
           file_selector.cpp \
           main.cpp \
           metrics.cpp \
           options.cpp \
           parse_dicom.cpp \
           preview.cpp \
           priority.cpp \
           read_dose.cpp \
           tissue_check.cpp \
           trim.cpp
