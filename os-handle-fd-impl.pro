TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += \
        src/include/kernel/* \
        src/include/lib/* \
        src/include/os/*

SOURCES += \
        src/init.c \
        src/kernel/* \
        src/lib/* \
        src/os/*
