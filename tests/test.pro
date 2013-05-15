TEMPLATE = app
TARGET = test
VERSION = 0.0.1

SOURCES = test.cpp

QT += declarative opengl
CONFIG += qt link_pkgconfig mobility
MOBILITY += multimedia

PKGCONFIG += gstreamer-0.10 qt-gst-qml-sink
