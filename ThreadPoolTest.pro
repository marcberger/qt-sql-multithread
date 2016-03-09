#-------------------------------------------------
#
# Project created by QtCreator 2012-03-23T15:57:56
#
#-------------------------------------------------

QT       += core sql

QT       -= gui

TARGET = ThreadPoolTest
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += debug

TEMPLATE = app


SOURCES += main.cpp \
    src/qcustomqsqlquery.cpp \
    src/qsqltask.cpp \
    src/qsqlthreadpool.cpp \
    workerexample.cpp

HEADERS += \
    src/qcustomqsqlquery.h \
    src/qsqltask.h \
    src/qsqlthreadpool.h \
    src/PoolError.h \
    src/LoggerDef.h \
    workerexample.h

DEFINES += DISPLAY_THREAD_IN_LOGS


DESTDIR = $$PWD/build
OBJECTS_DIR = $$PWD/build/.obj
MOC_DIR = $$PWD/build/.moc
RCC_DIR = $$PWD/build/.rcc
UI_DIR = $$PWD/build/.ui
