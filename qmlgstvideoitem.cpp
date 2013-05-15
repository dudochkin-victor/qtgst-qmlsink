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

#include "qmlgstvideoitem.h"

#include <qmediaobject.h>
#include <qmediaservice.h>
#include "qmlpaintervideosurface.h"
#include <qvideorenderercontrol.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qpointer.h>

#include <qvideosurfaceformat.h>
#include <QTimer>

#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_1_CL) && !defined(QT_OPENGL_ES_1)
#include <QtOpenGL/qgl.h>
#endif

Q_DECLARE_METATYPE(QVideoSurfaceFormat)

QT_BEGIN_NAMESPACE

class QmlGstVideoItemPrivate
{
public:
    QmlGstVideoItemPrivate()
        : q_ptr(0)
        , surface(0)
        , mediaObject(0)
        , service(0)
        , rendererControl(0)
        , aspectRatioMode(Qt::KeepAspectRatio)
        , updatePaintDevice(true)
        , rect(0.0, 0.0, 320, 240)
	, framesPresented(0)
	, framesDrawn(0)
	, curFrameId(0)
	, paintEvents(0)
	, t(new QTimer())
    {
    }

    QmlGstVideoItem *q_ptr;

    QmlPainterVideoSurface *surface;
    QPointer<QMediaObject> mediaObject;
    QMediaService *service;
    QVideoRendererControl *rendererControl;
    Qt::AspectRatioMode aspectRatioMode;
    bool updatePaintDevice;
    QRectF rect;
    QRectF boundingRect;
    QRectF sourceRect;
    QSizeF nativeSize;
  
    quint64 framesPresented;
    quint64 framesDrawn;
    quint64 curFrameId;
    quint64 paintEvents;

    QTimer *t;

    void clearService();
    void updateRects();

    void _q_present();
    void _q_formatChanged(const QVideoSurfaceFormat &format);
    void _q_updateNativeSize();
    void _q_serviceDestroyed();
    void _q_timerCb();
};

void QmlGstVideoItemPrivate::clearService()
{
    if (rendererControl) {
        surface->stop();
        rendererControl->setSurface(0);
        service->releaseControl(rendererControl);
        rendererControl = 0;
    }
    if (service) {
        QObject::disconnect(service, SIGNAL(destroyed()), q_ptr, SLOT(_q_serviceDestroyed()));
        service = 0;
    }
}

void QmlGstVideoItemPrivate::updateRects()
{
    q_ptr->prepareGeometryChange();

    if (nativeSize.isEmpty()) {
        //this is necessary for item to receive the
        //first paint event and configure video surface.
        boundingRect = rect;
    } else if (aspectRatioMode == Qt::IgnoreAspectRatio) {
        boundingRect = rect;
        sourceRect = QRectF(0, 0, 1, 1);
    } else if (aspectRatioMode == Qt::KeepAspectRatio) {
        QSizeF size = nativeSize;
        size.scale(rect.size(), Qt::KeepAspectRatio);

        boundingRect = QRectF(0, 0, size.width(), size.height());
        boundingRect.moveCenter(rect.center());

        sourceRect = QRectF(0, 0, 1, 1);
    } else if (aspectRatioMode == Qt::KeepAspectRatioByExpanding) {
        boundingRect = rect;

        QSizeF size = rect.size();
        size.scale(nativeSize, Qt::KeepAspectRatio);

        sourceRect = QRectF(
                0, 0, size.width() / nativeSize.width(), size.height() / nativeSize.height());
        sourceRect.moveCenter(QPointF(0.5, 0.5));
    }
}

void QmlGstVideoItemPrivate::_q_present()
{
    if (q_ptr->isObscured()) {
        q_ptr->update(boundingRect);
        surface->setReady(true);
    } else {
        q_ptr->update(boundingRect);
    }
    framesPresented++;
}

void QmlGstVideoItemPrivate::_q_updateNativeSize()
{
    const QSize &size = surface->surfaceFormat().sizeHint();
    if (nativeSize != size) {
        nativeSize = size;

        updateRects();
        emit q_ptr->nativeSizeChanged(nativeSize);
    }
}

void QmlGstVideoItemPrivate::_q_serviceDestroyed()
{
    rendererControl = 0;
    service = 0;

    surface->stop();
}

void QmlGstVideoItemPrivate::_q_timerCb()
{
  qint32 framesDropped = (framesPresented - framesDrawn) / 4;
  
  if (framesDropped > 0)
  {
    qDebug() << framesDrawn / 4 << "(" << framesDropped << " dropped)" << paintEvents;
  }

  else {
    qDebug() << framesDrawn / 4 << paintEvents / 4;
  }

  framesPresented = 0;
  framesDrawn = 0;
  curFrameId = 0;
  paintEvents = 0;
}


/*!
    \class GstVideoItem

    \brief The GstVideoItem class provides a graphics item which display video produced by a QMediaObject.

    \inmodule QtMultimediaKit
    \ingroup multimedia

    Attaching a GstVideoItem to a QMediaObject allows it to display
    the video or image output of that media object.  A GstVideoItem
    is attached to a media object by passing a pointer to the QMediaObject
    to the setMediaObject() function.

    \code
    player = new QMediaPlayer(this);

    GstVideoItem *item = new GstVideoItem;
    player->setVideoOutput(item);
    graphicsView->scene()->addItem(item);
    graphicsView->show();

    player->setMedia(video);
    player->play();
    \endcode

    \bold {Note}: Only a single display output can be attached to a media
    object at one time.

    \sa QMediaObject, QMediaPlayer, QVideoWidget
*/

/*!
    Constructs a graphics item that displays video.

    The \a parent is passed to QGraphicsItem.
*/
QmlGstVideoItem::QmlGstVideoItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , d_ptr(new QmlGstVideoItemPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->t->setInterval(4000);

    qRegisterMetaType<QVideoSurfaceFormat>();

    connect(d_ptr->t, SIGNAL(timeout()), this, SLOT(_q_timerCb()));
}

/*!
    Destroys a video graphics item.
*/
QmlGstVideoItem::~QmlGstVideoItem()
{
    if (d_ptr->rendererControl) {
        d_ptr->rendererControl->setSurface(0);
        d_ptr->service->releaseControl(d_ptr->rendererControl);
    }

    //delete d_ptr->surface;
    delete d_ptr;
}

/*!
    \property GstVideoItem::mediaObject
    \brief the media object which provides the video displayed by a graphics
    item.
*/

QMediaObject *QmlGstVideoItem::mediaObject() const
{
    return d_func()->mediaObject;
}

/*!
  \internal
*/
bool QmlGstVideoItem::setMediaObject(QMediaObject *object)
{
    Q_D(QmlGstVideoItem);

    if (object == d->mediaObject)
        return true;

    d->clearService();

    d->mediaObject = object;

    if (d->mediaObject) {
        d->service = d->mediaObject->service();

        if (d->service) {
            QMediaControl *control = d->service->requestControl(QVideoRendererControl_iid);
            if (control) {
                d->rendererControl = qobject_cast<QVideoRendererControl *>(control);

                if (d->rendererControl) {
                    //don't set the surface untill the item is painted
                    //at least once and the surface is configured
                    if (!d->updatePaintDevice)
                        d->rendererControl->setSurface(d->surface);
                    else
                        update(boundingRect());

                    connect(d->service, SIGNAL(destroyed()), this, SLOT(_q_serviceDestroyed()));

                    return true;
                }
                if (control)
                    d->service->releaseControl(control);
            }
        }
    }

    d->mediaObject = 0;
    return false;
}

/*!
    \property GstVideoItem::aspectRatioMode
    \brief how a video is scaled to fit the graphics item's size.
*/

Qt::AspectRatioMode QmlGstVideoItem::aspectRatioMode() const
{
    return d_func()->aspectRatioMode;
}

void QmlGstVideoItem::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    Q_D(QmlGstVideoItem);

    d->aspectRatioMode = mode;
    d->updateRects();
}

/*!
    \property GstVideoItem::offset
    \brief the video item's offset.

    GstVideoItem will draw video using the offset for its top left
    corner.
*/

QPointF QmlGstVideoItem::offset() const
{
    return d_func()->rect.topLeft();
}

void QmlGstVideoItem::setOffset(const QPointF &offset)
{
    Q_D(QmlGstVideoItem);

    d->rect.moveTo(offset);
    d->updateRects();
}

/*!
    \property GstVideoItem::size
    \brief the video item's size.

    GstVideoItem will draw video scaled to fit size according to its
    fillMode.
*/

QSizeF QmlGstVideoItem::size() const
{
    return d_func()->rect.size();
}

void QmlGstVideoItem::setSize(const QSizeF &size)
{
    Q_D(QmlGstVideoItem);

    d->rect.setSize(size.isValid() ? size : QSizeF(0, 0));
    d->updateRects();
}

/*!
    \property GstVideoItem::nativeSize
    \brief the native size of the video.
*/

QSizeF QmlGstVideoItem::nativeSize() const
{
    return d_func()->nativeSize;
}

/*!
    \fn GstVideoItem::nativeSizeChanged(const QSizeF &size)

    Signals that the native \a size of the video has changed.
*/

/*!
    \reimp
*/
QRectF QmlGstVideoItem::boundingRect() const
{
    return d_func()->boundingRect;
}

/*!
    \reimp
*/
void QmlGstVideoItem::paint(
        QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_D(QmlGstVideoItem);

    Q_UNUSED(option);
    Q_UNUSED(widget);

    d->paintEvents++;

    if (d->framesPresented == 1)
    {
      d->t->start();
    }

    if (d->framesPresented != d->curFrameId)
    {
      d->framesDrawn++;
      d->curFrameId = d->framesPresented;
    }

    if (d->surface && d->updatePaintDevice) {
        d->updatePaintDevice = false;
#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_1_CL) && !defined(QT_OPENGL_ES_1)
        if (widget)
            connect(widget, SIGNAL(destroyed()), d->surface, SLOT(viewportDestroyed()));

        d->surface->setGLContext(const_cast<QGLContext *>(QGLContext::currentContext()));
        if (d->surface->supportedShaderTypes() & QmlPainterVideoSurface::GlslShader) {
            d->surface->setShaderType(QmlPainterVideoSurface::GlslShader);
        } else {
            d->surface->setShaderType(QmlPainterVideoSurface::FragmentProgramShader);
        }
#endif
        if (d->rendererControl && d->rendererControl->surface() != d->surface)
            d->rendererControl->setSurface(d->surface);
    }

    if (d->surface && d->surface->isActive()) {
        d->surface->paint(painter, d->boundingRect, d->sourceRect);
        d->surface->setReady(true);
    }
}

/*!
    \reimp

    \internal
*/
QVariant QmlGstVideoItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsItem::itemChange(change, value);
}

/*!
  \internal
*/
void QmlGstVideoItem::timerEvent(QTimerEvent *event)
{
    QGraphicsObject::timerEvent(event);
}

void QmlGstVideoItem::setSurface(QmlPainterVideoSurface * surface)
{
  Q_D(QmlGstVideoItem);
  
  d->surface = surface;
  
  connect(d_ptr->surface, SIGNAL(frameChanged()), this, SLOT(_q_present()));
  connect(d_ptr->surface, SIGNAL(surfaceFormatChanged(QVideoSurfaceFormat)),
	  this, SLOT(_q_updateNativeSize()), Qt::QueuedConnection);
}

#include "moc_qmlgstvideoitem.cpp"

QT_END_NAMESPACE
