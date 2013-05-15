/****************************************************************************
**
** Copyright (C) 2011 Collabora Ltd
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file was originally part of the Qt Mobility Components.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
****************************************************************************/

#ifndef QMLGSTXVIMAGEBUFFER_H
#define QMLGSTXVIMAGEBUFFER_H

#include <qabstractvideobuffer.h>
#include <qvideosurfaceformat.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qqueue.h>

#ifndef QT_NO_XVIDEO

#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>


#include <gst/gst.h>
class QmlGstXvImageBufferPool;

struct QmlGstXvImageBuffer {
    GstBuffer buffer;
    QmlGstXvImageBufferPool *pool;
    XvImage *xvImage;
    XShmSegmentInfo shmInfo;
    bool markedForDeletion;

    static GType get_type(void);
    static void class_init(gpointer g_class, gpointer class_data);
    static void buffer_init(QmlGstXvImageBuffer *xvimage, gpointer g_class);
    static void buffer_finalize(QmlGstXvImageBuffer * xvimage);
    static GstBufferClass *parent_class;
};

Q_DECLARE_METATYPE(XvImage*)

class QmlGstXvImageBufferPool : public QObject {
Q_OBJECT
friend class QmlGstXvImageBuffer;
public:
    QmlGstXvImageBufferPool(QObject *parent = 0);
    virtual ~QmlGstXvImageBufferPool();

    bool isFormatSupported(const QVideoSurfaceFormat &format);

    QmlGstXvImageBuffer *takeBuffer(const QVideoSurfaceFormat &format, GstCaps *caps);
    void clear();

private slots:
    void queuedAlloc();
    void queuedDestroy();

    void doClear();

    void recycleBuffer(QmlGstXvImageBuffer *);
    void destroyBuffer(QmlGstXvImageBuffer *);

private:
    void doAlloc();

    struct XvShmImage {
        XvImage *xvImage;
        XShmSegmentInfo shmInfo;
    };

    QMutex m_poolMutex;
    QMutex m_allocMutex;
    QWaitCondition m_allocWaitCondition;
    QMutex m_destroyMutex;
    QVideoSurfaceFormat m_format;
    GstCaps *m_caps;
    QList<QmlGstXvImageBuffer*> m_pool;
    QList<QmlGstXvImageBuffer*> m_allBuffers;
    QList<XvShmImage> m_imagesToDestroy;
    Qt::HANDLE m_threadId;
};

#endif //QT_NO_XVIDEO

#endif
