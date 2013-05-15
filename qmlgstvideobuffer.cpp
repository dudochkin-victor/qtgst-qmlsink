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

#include "qmlgstvideobuffer.h"


QmlGstVideoBuffer::QmlGstVideoBuffer(GstBuffer *buffer, int bytesPerLine)
    : QAbstractVideoBuffer(NoHandle)
    , m_buffer(buffer)
    , m_bytesPerLine(bytesPerLine)
    , m_mode(NotMapped)
{
    gst_buffer_ref(m_buffer);
}

QmlGstVideoBuffer::QmlGstVideoBuffer(GstBuffer *buffer, int bytesPerLine,
                QmlGstVideoBuffer::HandleType handleType,
                const QVariant &handle)
    : QAbstractVideoBuffer(handleType)
    , m_buffer(buffer)
    , m_bytesPerLine(bytesPerLine)
    , m_mode(NotMapped)
    , m_handle(handle)
{
    gst_buffer_ref(m_buffer);
}

QmlGstVideoBuffer::~QmlGstVideoBuffer()
{
    gst_buffer_unref(m_buffer);
}


QAbstractVideoBuffer::MapMode QmlGstVideoBuffer::mapMode() const
{
    return m_mode;
}

uchar *QmlGstVideoBuffer::map(MapMode mode, int *numBytes, int *bytesPerLine)
{
    if (mode != NotMapped && m_mode == NotMapped) {
        if (numBytes)
            *numBytes = m_buffer->size;

        if (bytesPerLine)
            *bytesPerLine = m_bytesPerLine;

        m_mode = mode;

        return m_buffer->data;
    } else {
        return 0;
    }
}
void QmlGstVideoBuffer::unmap()
{
    m_mode = NotMapped;
}

