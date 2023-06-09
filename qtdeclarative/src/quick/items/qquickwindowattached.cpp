// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwindow.h"
#include "qquickitem.h"
#include "qquickwindowattached_p.h"

QT_BEGIN_NAMESPACE

// QDoc comments must go in qquickwindow.cpp to avoid overwriting the Window docs

QQuickWindowAttached::QQuickWindowAttached(QObject* attachee)
    : QObject(attachee)
    , m_window(nullptr)
{
    m_attachee = qobject_cast<QQuickItem*>(attachee);
    if (m_attachee && m_attachee->window()) // It might not be in a window yet
        windowChange(m_attachee->window());
    if (m_attachee)
        connect(m_attachee, &QQuickItem::windowChanged, this, &QQuickWindowAttached::windowChange);
}

QWindow::Visibility QQuickWindowAttached::visibility() const
{
    return (m_window ? m_window->visibility() : QWindow::Hidden);
}

bool QQuickWindowAttached::isActive() const
{
    return (m_window ? m_window->isActive() : false);
}

QQuickItem *QQuickWindowAttached::activeFocusItem() const
{
    return (m_window ? m_window->activeFocusItem() : nullptr);
}

QQuickItem *QQuickWindowAttached::contentItem() const
{
    return (m_window ? m_window->contentItem() : nullptr);
}

int QQuickWindowAttached::width() const
{
    return (m_window ? m_window->width() : 0);
}

int QQuickWindowAttached::height() const
{
    return (m_window ? m_window->height() : 0);
}

QQuickWindow *QQuickWindowAttached::window() const
{
    return m_window;
}

void QQuickWindowAttached::windowChange(QQuickWindow *window)
{
    if (window != m_window) {
        QQuickWindow* oldWindow = m_window;
        m_window = window;

        if (oldWindow)
            oldWindow->disconnect(this);

        emit windowChanged();

        if (!oldWindow || !window || window->visibility() != oldWindow->visibility())
            emit visibilityChanged();
        if (!oldWindow || !window || window->isActive() != oldWindow->isActive())
            emit activeChanged();
        if (!oldWindow || !window || window->activeFocusItem() != oldWindow->activeFocusItem())
            emit activeFocusItemChanged();
        emit contentItemChanged();
        if (!oldWindow || !window || window->width() != oldWindow->width())
            emit widthChanged();
        if (!oldWindow || !window || window->height() != oldWindow->height())
            emit heightChanged();

        if (!window)
            return;

        // QQuickWindowQmlImpl::visibilityChanged also exists, and window might even
        // be QQuickWindowQmlImpl, but that's not what we are connecting to.
        // So this is actual window state rather than a buffered or as-requested one.
        // If we used the metaobject connect syntax there would be a warning:
        // QMetaObjectPrivate::indexOfSignalRelative - QMetaObject::indexOfSignal:
        // signal visibilityChanged(QWindow::Visibility) from QQuickWindow redefined in QQuickWindowQmlImpl
        connect(window, &QQuickWindow::visibilityChanged,
                this, &QQuickWindowAttached::visibilityChanged);
        connect(window, &QQuickWindow::activeChanged,
                this, &QQuickWindowAttached::activeChanged);
        connect(window, &QQuickWindow::activeFocusItemChanged,
                this, &QQuickWindowAttached::activeFocusItemChanged);
        connect(window, &QQuickWindow::widthChanged,
                this, &QQuickWindowAttached::widthChanged);
        connect(window, &QQuickWindow::heightChanged,
                this, &QQuickWindowAttached::heightChanged);
    }
}

QT_END_NAMESPACE

#include "moc_qquickwindowattached_p.cpp"
