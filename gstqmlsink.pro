TEMPLATE = lib
TARGET = qt-gst-qml-sink
VERSION = 0.0.1

target.path = $$INSTALL_ROOT/usr/lib

SOURCES += qmlgstvideobuffer.cpp
SOURCES += qmlgstxvimagebuffer.cpp
SOURCES += qmlx11videosurface.cpp
SOURCES += qmlpaintervideosurface.cpp
SOURCES += qmlgstvideoitem.cpp
SOURCES += qmlvideosurfacegstsink.cpp

HEADERS += qmlvideosurfacegstsink.h
HEADERS += qmlgstvideobuffer.h
HEADERS += qmlgstxvimagebuffer.h
HEADERS += qmlx11videosurface.h
HEADERS += qmlpaintervideosurface.h
HEADERS += qmlgstvideoitem.h

QT += declarative opengl
CONFIG += qt mobility link_pkgconfig create_pc create_prl
MOBILITY += multimedia

PKGCONFIG += xv
PKGCONFIG += gstreamer-0.10
PKGCONFIG += gstreamer-video-0.10
PKGCONFIG += gstreamer-app-0.10


headers.files = $$HEADERS
headers.path = /usr/include/QtGstQmlSink

pkgconfig.path = /usr/lib/pkgconfig
pkgconfig.files = qt-gst-qml-sink.pc

INSTALLS += target headers pkgconfig
