#ifndef Q_CAL_RX_FILTER_TABWIDGET_H
#define Q_CAL_RX_FILTER_TABWIDGET_H

#include "q_model_rx_filter.h"
#include <QTabWidget>

class QCalPlot;
class QwtPlotCurve;
class QRDTTableView;

class QCalR1CRXFilterTabWidget : public QTabWidget
{
public:
    QCalR1CRXFilterTabWidget(QWidget *parent = nullptr);
    void init(QRXFilterModel_80M *model80, QRXFilterModel_160M *model160);
    void resetShowWidget();

public:
    // The 2(RF & IF) plots.
    QCalPlot *plotRF;
    QCalPlot *plotIF;

    // The curves in particular plot.
    QwtPlotCurve *curveRfFr;
    QwtPlotCurve *curveIfFr;

    // The curve data.
    Qwt_RX_RF_FR_Data *dataRF_FR;
    Qwt_RX_IF_FR_Data *dataIF_FR;

    // The filter coef View/Model.
    QRDTTableView *tableView_80;
    QRDTTableView *tableView_160;
    QRXFilterModel_80M *model_80;
    QRXFilterModel_160M *model_160;
};

#endif // Q_CAL_RX_FILTER_TABWIDGET_H
