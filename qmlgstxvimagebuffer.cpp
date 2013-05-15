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

#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qvariant.h>
#include <QtGui/qx11info_x11.h>

#include "qmlgstxvimagebuffer.h"
#include "qmlvideosurfacegstsink.h"

#ifndef QT_NO_XVIDEO

GstBufferClass *QmlGstXvImageBuffer::parent_class = NULL;

GType QmlGstXvImageBuffer::get_type(void)
{
    static GType buffer_type = 0;

    if (buffer_type == 0) {
        static const GTypeInfo buffer_info = {
            sizeof (GstBufferClass),
            NULL,
            NULL,
            QmlGstXvImageBuffer::class_init,
            NULL,
            NULL,
            sizeof(QmlGstXvImageBuffer),
            0,
            (GInstanceInitFunc)QmlGstXvImageBuffer::buffer_init,
            NULL
        };
        buffer_type = g_type_register_static(GST_TYPE_BUFFER,
                                             "QmlGstXvImageBuffer", &buffer_info, GTypeFlags(0));
    }
    return buffer_type;
}

void QmlGstXvImageBuffer::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);
    GST_MINI_OBJECT_CLASS(g_class)->finalize =
            (GstMiniObjectFinalizeFunction)buffer_finalize;
    parent_class = (GstBufferClass*)g_type_class_peek_parent(g_class);
}

void QmlGstXvImageBuffer::buffer_init(QmlGstXvImageBuffer *xvImage, gpointer g_class)
{
    Q_UNUSED(g_class);
    xvImage->pool = 0;
    xvImage->shmInfo.shmaddr = ((char *) -1);
    xvImage->shmInfo.shmid = -1;
    xvImage->markedForDeletion = false;
}

void QmlGstXvImageBuffer::buffer_finalize(QmlGstXvImageBuffer * xvImage)
{
    if (xvImage->pool) {
        if (xvImage->markedForDeletion)
            xvImage->pool->destroyBuffer(xvImage);
        else
            xvImage->pool->recycleBuffer(xvImage);
    }
}


QmlGstXvImageBufferPool::QmlGstXvImageBufferPool(QObject *parent)
    :QObject(parent)
{
    m_threadId = QThread::currentThreadId();
}

QmlGstXvImageBufferPool::~QmlGstXvImageBufferPool()
{
}

bool QmlGstXvImageBufferPool::isFormatSupported(const QVideoSurfaceFormat &surfaceFormat)
{
    bool ok = true;
    surfaceFormat.property("portId").toULongLong(&ok);
    if (!ok)
        return false;

    int xvFormatId = surfaceFormat.property("xvFormatId").toInt(&ok);
    if (!ok || xvFormatId < 0)
        return false;

    int dataSize = surfaceFormat.property("dataSize").toInt(&ok);
    if (!ok || dataSize<=0)
        return false;

    return true;
}

QmlGstXvImageBuffer *QmlGstXvImageBufferPool::takeBuffer(const QVideoSurfaceFormat &format, GstCaps *caps)
{
    m_poolMutex.lock();

    m_caps = caps;
    if (format != m_format) {
        doClear();
        m_format = format;
    }


    if (m_pool.isEmpty()) {
        //qDebug() << "QmlGstXvImageBufferPool::takeBuffer: no buffer available, allocate the new one" << QThread::currentThreadId() << m_threadId;
        if (QThread::currentThreadId() == m_threadId) {
            doAlloc();
        } else {
            QMetaObject::invokeMethod(this, "queuedAlloc", Qt::QueuedConnection);
            m_allocWaitCondition.wait(&m_poolMutex, 300);
        }
    }
    QmlGstXvImageBuffer *res = 0;

    if (!m_pool.isEmpty()) {
        res = m_pool.takeLast();
    }

    m_poolMutex.unlock();

    return res;
}

void QmlGstXvImageBufferPool::queuedAlloc()
{
    QMutexLocker lock(&m_poolMutex);
    doAlloc();
    m_allocWaitCondition.wakeOne();
}

void QmlGstXvImageBufferPool::doAlloc()
{
    //should be always called from the main thread with m_poolMutex locked
    //Q_ASSERT(QThread::currentThread() == thread());

    XSync(QX11Info::display(), false);

    QmlGstXvImageBuffer *xvBuffer = (QmlGstXvImageBuffer *)gst_mini_object_new(QmlGstXvImageBuffer::get_type());

    quint64 portId = m_format.property("portId").toULongLong();
    int xvFormatId = m_format.property("xvFormatId").toInt();

    xvBuffer->xvImage = XvShmCreateImage(
            QX11Info::display(),
            portId,
            xvFormatId,
            0,
            m_format.frameWidth(),
            m_format.frameHeight(),
            &xvBuffer->shmInfo
            );

    if (!xvBuffer->xvImage) {
        qWarning() << "QmlGstXvImageBufferPool: XvShmCreateImage failed";
        return;
    }

    XSync(QX11Info::display(), false);

    xvBuffer->shmInfo.shmid = shmget(IPC_PRIVATE, xvBuffer->xvImage->data_size, IPC_CREAT | 0777);
    xvBuffer->shmInfo.shmaddr = xvBuffer->xvImage->data = (char*)shmat(xvBuffer->shmInfo.shmid, 0, 0);
    xvBuffer->shmInfo.readOnly = False;

    if (!XShmAttach(QX11Info::display(), &xvBuffer->shmInfo)) {
        qWarning() << "QmlGstXvImageBufferPool: XShmAttach failed";
        return;
    }

    XSync(QX11Info::display(), false);

    shmctl (xvBuffer->shmInfo.shmid, IPC_RMID, NULL);

    xvBuffer->pool = this;
    GST_MINI_OBJECT_CAST(xvBuffer)->flags = 0;
    gst_buffer_set_caps(GST_BUFFER_CAST(xvBuffer), m_caps);
    GST_BUFFER_DATA(xvBuffer) = (uchar*)xvBuffer->xvImage->data;
    GST_BUFFER_SIZE(xvBuffer) = xvBuffer->xvImage->data_size;

    m_allBuffers.append(xvBuffer);
    m_pool.append(xvBuffer);

    XSync(QX11Info::display(), false);
}


void QmlGstXvImageBufferPool::clear()
{
    QMutexLocker lock(&m_poolMutex);
    doClear();
}

void QmlGstXvImageBufferPool::doClear()
{
    foreach (QmlGstXvImageBuffer *xvBuffer, m_allBuffers) {
        xvBuffer->markedForDeletion = true;
    }
    m_allBuffers.clear();

    foreach (QmlGstXvImageBuffer *xvBuffer, m_pool) {
        gst_buffer_unref(GST_BUFFER(xvBuffer));
    }
    m_pool.clear();

    m_format = QVideoSurfaceFormat();
}

void QmlGstXvImageBufferPool::queuedDestroy()
{
    QMutexLocker lock(&m_destroyMutex);

    XSync(QX11Info::display(), false);

    foreach(XvShmImage xvImage, m_imagesToDestroy) {
        if (xvImage.shmInfo.shmaddr != ((void *) -1)) {
            XShmDetach(QX11Info::display(), &xvImage.shmInfo);
            XSync(QX11Info::display(), false);

            shmdt(xvImage.shmInfo.shmaddr);
        }

        if (xvImage.xvImage)
            XFree(xvImage.xvImage);
    }

    m_imagesToDestroy.clear();

    XSync(QX11Info::display(), false);
}

void QmlGstXvImageBufferPool::recycleBuffer(QmlGstXvImageBuffer *xvBuffer)
{
    QMutexLocker lock(&m_poolMutex);
    gst_buffer_ref(GST_BUFFER_CAST(xvBuffer));
    m_pool.append(xvBuffer);
}

void QmlGstXvImageBufferPool::destroyBuffer(QmlGstXvImageBuffer *xvBuffer)
{
    XvShmImage imageToDestroy;
    imageToDestroy.xvImage = xvBuffer->xvImage;
    imageToDestroy.shmInfo = xvBuffer->shmInfo;

    m_destroyMutex.lock();
    m_imagesToDestroy.append(imageToDestroy);
    m_destroyMutex.unlock();

    if (m_imagesToDestroy.size() == 1)
        QMetaObject::invokeMethod(this, "queuedDestroy", Qt::QueuedConnection);
}

#endif //QT_NO_XVIDEO

