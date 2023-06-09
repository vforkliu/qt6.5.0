// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCharts/QBarLegendMarker>
#include <private/qbarlegendmarker_p.h>
#include <QtCharts/QAbstractBarSeries>
#include <QtCharts/QBarSet>

QT_BEGIN_NAMESPACE

/*!
    \class QBarLegendMarker
    \inmodule QtCharts
    \brief The QBarLegendMarker class is a legend marker for a bar series.

    A bar legend marker is related to QAbstractBarSeries derived classes. With a bar series,
    each marker is related to one QBarSet.

    \sa QLegend, QAbstractBarSeries, QBarSet
*/

/*!
  \fn virtual LegendMarkerType QBarLegendMarker::type()
  \reimp
*/

/*!
  \internal
  Constructor
*/
QBarLegendMarker::QBarLegendMarker(QAbstractBarSeries *series, QBarSet *barset, QLegend *legend, QObject *parent) :
    QLegendMarker(*new QBarLegendMarkerPrivate(this,series,barset,legend), parent)
{
    d_ptr->updated();
}

/*!
    Removes the legend marker for a bar set.
*/
QBarLegendMarker::~QBarLegendMarker()
{
}

/*!
    \internal
*/
QBarLegendMarker::QBarLegendMarker(QBarLegendMarkerPrivate &d, QObject *parent) :
    QLegendMarker(d, parent)
{
}

/*!
    \reimp
*/
QAbstractBarSeries *QBarLegendMarker::series()
{
    Q_D(QBarLegendMarker);
    return d->m_series;
}

/*!
  Returns the bar set related to the marker.
*/
QBarSet* QBarLegendMarker::barset()
{
    Q_D(QBarLegendMarker);
    return d->m_barset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QBarLegendMarkerPrivate::QBarLegendMarkerPrivate(QBarLegendMarker *q, QAbstractBarSeries *series, QBarSet *barset, QLegend *legend) :
    QLegendMarkerPrivate(q,legend),
    q_ptr(q),
    m_series(series),
    m_barset(barset)
{
    QObject::connect(m_barset, SIGNAL(penChanged()), this, SLOT(updated()));
    QObject::connect(m_barset, SIGNAL(labelChanged()), this, SLOT(updated()));
    QObject::connect(m_barset, SIGNAL(brushChanged()), this, SLOT(updated()));
}

QBarLegendMarkerPrivate::~QBarLegendMarkerPrivate()
{
}

QAbstractBarSeries* QBarLegendMarkerPrivate::series()
{
    return m_series;
}

QObject* QBarLegendMarkerPrivate::relatedObject()
{
    return m_barset;
}

void QBarLegendMarkerPrivate::updated()
{
    bool labelChanged = false;
    bool brushChanged = false;
    bool penChanged = false;

    if (!m_customPen && (m_item->pen() != m_barset->pen())) {
        m_item->setPen(m_barset->pen());
        penChanged = true;
    }
    if (!m_customBrush && (m_item->brush() != m_barset->brush())) {
        m_item->setBrush(m_barset->brush());
        brushChanged = true;
    }
    if (!m_customLabel && (m_item->label() != m_barset->label())) {
        m_item->setLabel(m_barset->label());
        labelChanged = true;
    }
    invalidateLegend();

    if (labelChanged)
        emit q_ptr->labelChanged();
    if (brushChanged)
        emit q_ptr->brushChanged();
    if (penChanged)
        emit q_ptr->penChanged();
}

QT_END_NAMESPACE

#include "moc_qbarlegendmarker.cpp"
#include "moc_qbarlegendmarker_p.cpp"
