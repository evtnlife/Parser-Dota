#-------------------------------------------------
#
# Project created by QtCreator 2017-03-16T22:47:09
#
#-------------------------------------------------

QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = parser
TEMPLATE = app


SOURCES += main.cpp\
        parser.cpp

HEADERS  += parser.h \
    connection.h

FORMS    += parser.ui
