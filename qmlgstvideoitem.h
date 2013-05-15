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

#ifndef MY_QGRAPHICS_VIDEOITEM
#define MY_QGRAPHICS_VIDEOITEM

#include <QObject>
#include <QtGui/qgraphicsitem.h>

#include <qvideowidget.h>
#include <qmediabindableinterface.h>
#include "qmlpaintervideosurface.h"

QT_BEGIN_NAMESPACE
class QVideoSurfaceFormat;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

class QmlGstVideoItemPrivate;
class QmlGstVideoItem : public QGraphicsObject, public QMediaBindableInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaBindableInterface)
    Q_PROPERTY(QMediaObject* mediaObject READ mediaObject WRITE setMediaObject)
    Q_PROPERTY(Qt::AspectRatioMode aspectRatioMode READ aspectRatioMode WRITE setAspectRatioMode)
    Q_PROPERTY(QPointF offset READ offset WRITE setOffset)
    Q_PROPERTY(QSizeF size READ size WRITE setSize)
    Q_PROPERTY(QSizeF nativeSize READ nativeSize NOTIFY nativeSizeChanged)
      Q_PROPERTY(QmlPainterVideoSurface * surface WRITE setSurface)
public:
    QmlGstVideoItem(QGraphicsItem *parent = 0);
    ~QmlGstVideoItem();

    QMediaObject *mediaObject() const;    

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    QPointF offset() const;
    void setOffset(const QPointF &offset);

    QSizeF size() const;
    void setSize(const QSizeF &size);

    QSizeF nativeSize() const;

    QRectF boundingRect() const;

    void setSurface(QmlPainterVideoSurface *surface);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

Q_SIGNALS:
    void nativeSizeChanged(const QSizeF &size);

protected:
    void timerEvent(QTimerEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    bool setMediaObject(QMediaObject *object);

    QmlGstVideoItemPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(QmlGstVideoItem)
    Q_PRIVATE_SLOT(d_func(), void _q_present())
    Q_PRIVATE_SLOT(d_func(), void _q_updateNativeSize())
    Q_PRIVATE_SLOT(d_func(), void _q_serviceDestroyed())
    Q_PRIVATE_SLOT(d_func(), void _q_timerCb())
};

QT_END_NAMESPACE

#endif
