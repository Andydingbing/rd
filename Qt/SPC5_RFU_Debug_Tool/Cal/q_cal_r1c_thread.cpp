#include "q_cal_r1c_thread.h"
#include "q_cal_r1c_dlg.h"
#include <QTime>

QCalR1CThread::QCalR1CThread(QObject *parent, const CalR1CParam &param) :
    QCalBaseThread(CalParam()) ,
    calParam(param)
{
    subThreadRunning = false;
    subThreadContinue = true;
}

void QCalR1CThread::subThreadDone()
{
    subThreadRunning = false;
}

void QCalR1CThread::subThreadResult(CalResult cr)
{
    subThreadContinue = (cr == CTR_COMPLETE_PASS ? true : false);

    for (int calItemIdx = 0;calItemIdx < calParam.calThreads.size();calItemIdx ++) {
        if (sender() == calParam.calThreads.at(calItemIdx)) {
            emit update(QModelIndex(),QModelIndex(),cal_file::cal_item_t(calItemIdx),cr);
            break;
        }
    }
}

void QCalR1CThread::run()
{
    QVector<QCalBaseThread *> calThreads = calParam.calThreads;
    QVector<QCalBaseThread *>::iterator iterCalThreads;

    for (iterCalThreads = calThreads.begin();iterCalThreads != calThreads.end();iterCalThreads ++) {
        connect(*iterCalThreads,SIGNAL(done(bool)),this,SLOT(subThreadDone()));
        connect(*iterCalThreads,SIGNAL(result(CalResult)),this,SLOT(subThreadResult(CalResult)));

        QWinThread::g_threadStop = subThreadContinue ? false : true;
        QWinThread::g_threadThread = *iterCalThreads;
        QWinThread::g_threadThread->start();

        waitSubThread();
    }

    CAL_THREAD_ABOART;
}

void QCalR1CThread::waitSubThread(int timeout)
{
    QTime timer;
    timer.start();

    subThreadRunning = true;
    for (;timer.elapsed() < timeout ? timeout : int(0x7FFFFFFF);) {
        if (subThreadRunning == false)
            return;
        msleep(2);
    }
}
