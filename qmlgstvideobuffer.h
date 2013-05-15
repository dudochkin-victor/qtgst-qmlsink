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

#ifndef QGSTVIDEOBUFFER_H
#define QGSTVIDEOBUFFER_H

#include <qabstractvideobuffer.h>
#include <QtCore/qvariant.h>

#include <gst/gst.h>

class QmlGstVideoBuffer : public QAbstractVideoBuffer
{
public:
    QmlGstVideoBuffer(GstBuffer *buffer, int bytesPerLine);
    QmlGstVideoBuffer(GstBuffer *buffer, int bytesPerLine,
                    HandleType handleType, const QVariant &handle);
    ~QmlGstVideoBuffer();

    MapMode mapMode() const;

    uchar *map(MapMode mode, int *numBytes, int *bytesPerLine);
    void unmap();

    QVariant handle() const { return m_handle; }
private:
    GstBuffer *m_buffer;
    int m_bytesPerLine;
    MapMode m_mode;
    QVariant m_handle;
};


#endif
