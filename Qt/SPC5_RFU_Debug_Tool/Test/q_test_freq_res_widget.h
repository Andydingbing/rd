#ifndef Q_TEST_FREQ_RES_WIDGET_H
#define Q_TEST_FREQ_RES_WIDGET_H

#include <QWidget>
#include "q_plot.h"
#include "q_model_freq_res.h"

struct TestBaseParam;
class QwtPlotCurve;
class QTestFreqResWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QTestFreqResWidget(QWidget *parent = nullptr);
    void resetShowWidget(TestBaseParam *param);

public:
    QwtRF_TX_FreqResData *dataRF_TX;
    QwtIF_TX_FreqResData *dataIF_TX;
    QwtRF_RX_FreqResData *dataRF_RX;
    QwtIF_RX_FreqResData *dataIF_RX;
    QwtPlotCurve *curveRF_TX;
    QwtPlotCurve *curveIF_TX;
    QwtPlotCurve *curveRF_RX;
    QwtPlotCurve *curveIF_RX;
    QTestPlot *plotRF;
    QTestPlot *plotIF;
};

#endif // Q_TEST_FREQ_RES_WIDGET_H
