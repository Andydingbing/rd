#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSplitter>
#include <QLabel>
#include <QVBoxLayout>
#include <QMetaType>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QProgressBar>
#include <QAction>
#include <QMenu>
#include "rf_driver.h"
#include "sp9500_child_widget_helper.hpp"
#include "dt3308_child_widget_helper.hpp"
#include "sp9500x_child_widget_helper.hpp"
#include "sp9500pro_child_widget_helper.hpp"
#include "q_log_model.hpp"
#include "dt3308_bb_f_widget.h"
#include "q_temp_ctrl_dlg.h"
#include "q_device_init_thread.h"
#include "q_iq_cap_thread.h"
#include "scroll_lineedit.h"

#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    #define setSectionResizeMode setResizeMode
#endif

QHBoxLayout MainWindow::childDlgLayout;
MainWindow *g_MainW = nullptr;

double QFreqScrollLineEdit::stepDefault = 1e6;
double QFreqScrollLineEdit::stepCtrl    = 5e6;
double QFreqScrollLineEdit::stepShift   = 10e6;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    SP3301 = &SP3301_2;
    NS_SP9500X::SP3103 = &SP3103_0;

    ui->setupUi(this);

    setWindowState(Qt::WindowMaximized);

    actionSP9500 = new QAction("SP9500(&0)",this);
    actionSP9500->setCheckable(true);

    actionDT3308 = new QAction("DT3308(&1)",this);
    actionDT3308->setCheckable(true);

    actionSP9500X = new QAction("SP9500X(&2)",this);
    actionSP9500X->setCheckable(true);

    actionSP9500Pro = new QAction("SP9500Pro(&3)",this);
    actionSP9500Pro->setCheckable(true);

    connect(actionSP9500,SIGNAL(triggered(bool)),this,SLOT(switchToSP9500()));
    connect(actionDT3308,SIGNAL(triggered(bool)),this,SLOT(switchToDT3308()));
    connect(actionSP9500X,SIGNAL(triggered(bool)),this,SLOT(switchToSP9500X()));
    connect(actionSP9500Pro,SIGNAL(triggered(bool)),this,SLOT(switchToSP9500Pro()));

    menuProject = new QMenu("Project(&P)",ui->menuBar);
    menuProject->addAction(actionSP9500);
    menuProject->addAction(actionDT3308);
    menuProject->addAction(actionSP9500X);
    menuProject->addAction(actionSP9500Pro);

    registerMetaType();

    Log.en(log_t::RD_LOG_ALL_ON,false);

    childWidgets.push_back(new NS_SP9500::ChildWidgets(this));
    childWidgets.push_back(new NS_DT3308::ChildWidgets(this));
    childWidgets.push_back(new NS_SP9500X::ChildWidgets(this));
    childWidgets.push_back(new NS_SP9500Pro::ChildWidgets(this));

    initMsgLogDlg();
    initRegLogDlg();
    initStatusBar();
    initMainTreeWidget();
    initMainTabWidget();

    QSplitter *mainSplitter   = new QSplitter(Qt::Vertical,ui->centralWidget);
    QSplitter *topSplitter    = new QSplitter(Qt::Horizontal,mainSplitter);
    QSplitter *bottomSplitter = new QSplitter(Qt::Horizontal,mainSplitter);

    topSplitter->setHandleWidth(0);
    topSplitter->addWidget(mainTree);
    topSplitter->addWidget(mainTab);
    topSplitter->setStretchFactor(0,0);
    topSplitter->setStretchFactor(1,1);

    bottomSplitter->setHandleWidth(0);
    bottomSplitter->addWidget(msgTableView);
    bottomSplitter->addWidget(regTableView);
    bottomSplitter->setStretchFactor(0,1);
    bottomSplitter->setStretchFactor(1,0);

    mainSplitter->setHandleWidth(0);
    mainSplitter->setStretchFactor(0,5);
    mainSplitter->setStretchFactor(1,1);

    QHBoxLayout *mainLayout = new QHBoxLayout(ui->centralWidget);
    mainLayout->setMargin(0);
    mainLayout->addWidget(mainSplitter);
    ui->centralWidget->setLayout(mainLayout);

    switchToSP9500();
    Log.set_default();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::switchToSP9500()
{
    project = SP9500;
    actionSP9500->setChecked(true);
    actionDT3308->setChecked(false);
    actionSP9500X->setChecked(false);
    actionSP9500Pro->setChecked(false);
    switchProject();
}

void MainWindow::switchToDT3308()
{
    project = DT3308;
    actionSP9500->setChecked(false);
    actionDT3308->setChecked(true);
    actionSP9500X->setChecked(false);
    actionSP9500Pro->setChecked(false);
    switchProject();
}

void MainWindow::switchToSP9500X()
{
    project = SP9500X;
    actionSP9500->setChecked(false);
    actionDT3308->setChecked(false);
    actionSP9500X->setChecked(true);
    actionSP9500Pro->setChecked(false);
    switchProject();
}

void MainWindow::switchToSP9500Pro()
{
    project = SP9500Pro;
    actionSP9500->setChecked(false);
    actionDT3308->setChecked(false);
    actionSP9500X->setChecked(false);
    actionSP9500Pro->setChecked(true);
    switchProject();
}

void MainWindow::switchProject()
{
    ui->menuBar->clear();
    ui->menuBar->addAction(menuProject->menuAction());

    mainTree->clear();
    mainTab->clear();

    currentWidgets = childWidgets.at(project);
    currentWidgets->initMenu();
    currentWidgets->addChildWidgets();
    currentWidgets->initMainTreeWidget();
}

void MainWindow::deviceInit()
{
    QDeviceInitThread *thread = new QDeviceInitThread(this);
    QWinThread::g_threadThread = thread;

    thread->silent = false;
    connect(thread,&QDeviceInitThread::swhwVerReady,this,&MainWindow::showSwHwVer);
    connect(thread,&QDeviceInitThread::done,currentWidgets,&ChildWidgetHelper::initChildWidgets,Qt::BlockingQueuedConnection);

    thread->start();
}

void MainWindow::deviceInitSilent()
{
    QDeviceInitThread *thread = new QDeviceInitThread(this);
    QWinThread::g_threadThread = thread;

    thread->silent = true;
    connect(thread,&QDeviceInitThread::swhwVerReady,this,&MainWindow::showSwHwVer);
    connect(thread,&QDeviceInitThread::done,currentWidgets,&ChildWidgetHelper::initChildWidgets,Qt::BlockingQueuedConnection);

    thread->start();
}

void MainWindow::exit()
{
    QApplication::exit();
}

void MainWindow::starAllIQCapture()
{
    THREAD_EXISTED_CHK();

    QIQCapThread *thread = new QIQCapThread(this);
    QWinThread::g_threadThread = thread;

    if (project == SP9500) {
        for (int i = 0;i < ns_sp9500::g_max_rf;++i) {
            connect(thread,SIGNAL(updatePlot()),widgetsSP9500()->widget_IQ_Cap[i],SLOT(updatePlot()),Qt::BlockingQueuedConnection);
        }
    } else if (project == DT3308) {
        connect(thread,SIGNAL(updatePlot()),widgetsDT3308()->widget_IQ_Cap[0],SLOT(updatePlot()),Qt::BlockingQueuedConnection);
    } else if (project == SP9500X) {
        for (int i = 0;i < ns_sp3103::g_max_rf;++i) {
            connect(thread,SIGNAL(updatePlot()),widgetsSP9500X()->widget_SA[i],SLOT(updatePlot()),Qt::BlockingQueuedConnection);
        }
    }
    QWinThread::g_threadThread->start();
}

void MainWindow::stopAllIQCapture()
{
    QWinThread::g_threadStop = true;
}

void MainWindow::showSwHwVer(const sp3301::rfu_info_t &info)
{
    labelVerK7_0->setText(QString("K7_0:0x%1").arg(info.k7_0_ver,8,16));
    labelVerK7_1->setText(QString("K7_1:0x%1").arg(info.k7_1_ver,8,16));
    labelVerS6->setText(QString("S6:0x%1").arg(info.s6_ver,8,16));
}

void MainWindow::registerMetaType()
{
    if (project == SPC5 || project == SP9500) {
        qRegisterMetaType<sp3301::rfu_info_t>("sp3301::rfu_info_t");
        qRegisterMetaType<cal_file::cal_item_t>("cal_file::cal_item_t");
        qRegisterMetaType<test_item_t>("test_item_t");
    }
    QWinThread::registerMetaType();
}

void MainWindow::initStatusBar()
{
    char *driverVer = nullptr;

    RF_DriverVersion(&driverVer);

    mainProgressBar = new QProgressBar(ui->statusBar);
    mainProgressBar->setFixedHeight(15);
    mainProgressBar->setStyleSheet("QProgressBar{text-align:center;}");
    setProgClr(Qt::green);

    labelProgressName = new QLabel(QString("..."));
    labelVerRF = new QLabel(QString("RF:"));
    labelVerK7_0 = new QLabel(QString("K7_0:0x00000000"));
    labelVerK7_1 = new QLabel(QString("K7_1:0x00000000"));
    labelVerS6 = new QLabel(QString("S6:0x00000000"));
    labelVerDriver = new QLabel(QString("Driver:%1").arg(driverVer));

    ui->statusBar->clearMessage();
    ui->statusBar->addWidget(labelProgressName);
    ui->statusBar->addWidget(mainProgressBar,1);
    ui->statusBar->addPermanentWidget(labelVerRF);
    ui->statusBar->addPermanentWidget(labelVerK7_0);
    ui->statusBar->addPermanentWidget(labelVerK7_1);
    ui->statusBar->addPermanentWidget(labelVerS6);
    ui->statusBar->addPermanentWidget(labelVerDriver);
}

void MainWindow::initMainTreeWidget()
{
    QFont font;
    font.setPointSize(10);
    font.setBold(true);
    font.setWeight(75);

    mainTree = new QTreeWidget;
    mainTree->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);
    mainTree->setHeaderHidden(true);
    mainTree->setRootIsDecorated(true);
    mainTree->clear();
    mainTree->setColumnCount(1);
    mainTree->setFont(font);
    mainTree->setSelectionMode(QAbstractItemView::SingleSelection);
    mainTree->setStyleSheet(
                "QTreeWidget {background:rgb(179,217,255)}"
                "QTreeWidget::item:selected {background:rgb(0,255,0);color:black;}");

    connect(mainTree,&QTreeWidget::itemClicked,this,&MainWindow::mainTree_itemClicked);
}

void MainWindow::initMainTabWidget()
{
    QFont font;
    font.setPointSize(10);
    font.setBold(true);
    font.setWeight(75);

    mainTab = new QTabWidget;
    mainTab->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    mainTab->setFont(font);
    mainTab->setStyleSheet(
                "QTabBar::tab {height:20px; width:120px;}"
                "QTabBar::tab:selected {background:rgb(0,255,0)}");

    QHBoxLayout *mainTabLayout = new QHBoxLayout;
    mainTabLayout->addLayout(&childDlgLayout);
    mainTab->setLayout(mainTabLayout);

    connect(mainTab,&QTabWidget::currentChanged,this,&MainWindow::mainTab_currentChanged);
}

void MainWindow::initMsgLogDlg()
{
    msgLogModel = new QMsgLogModel;

    msgTableView = new Q_RDDT_TableView;
    msgTableView->setModel(msgLogModel);
    msgTableView->setColumnWidth(0,50);
    msgTableView->setColumnWidth(1,125);
    msgTableView->setColumnWidth(2,350);
    msgTableView->setColumnWidth(3,50);
    msgTableView->setColumnWidth(4,80);
    msgTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    msgTableView->verticalHeader()->setDefaultSectionSize(18);

    msgTableView->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
    msgTableView->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Fixed);
    msgTableView->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
    msgTableView->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Fixed);
    msgTableView->horizontalHeader()->setSectionResizeMode(4,QHeaderView::Fixed);

    connect(this,SIGNAL(addMsgList(int)),msgLogModel,SLOT(update(int)));
    connect(this,SIGNAL(addMsgList(int)),this,SLOT(updateMsgTable(int)));
}

void MainWindow::initRegLogDlg()
{
    regLogModel = new QRegLogModel;

    regTableView = new Q_RDDT_TableView;
    regTableView->setMinimumSize(QSize(440, 0));
    regTableView->setModel(regLogModel);
    regTableView->setColumnWidth(0,68);
    regTableView->setColumnWidth(1,68);
    regTableView->setColumnWidth(2,60);
    regTableView->setColumnWidth(3,85);
    regTableView->setColumnWidth(4,85);
    regTableView->setColumnWidth(5,50);
    regTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    regTableView->verticalHeader()->setDefaultSectionSize(18);
    regTableView->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
    regTableView->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Fixed);
    regTableView->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Fixed);
    regTableView->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Fixed);
    regTableView->horizontalHeader()->setSectionResizeMode(4,QHeaderView::Fixed);
    regTableView->horizontalHeader()->setSectionResizeMode(5,QHeaderView::Stretch);

    connect(this,SIGNAL(addRegList(int)),regLogModel,SLOT(update(int)));
    connect(this,SIGNAL(addRegList(int)),this,SLOT(updateRegTable(int)));
}

NS_SP9500::ChildWidgets *MainWindow::widgetsSP9500()
{ return dynamic_cast<NS_SP9500::ChildWidgets *>(currentWidgets); }

NS_DT3308::ChildWidgets *MainWindow::widgetsDT3308()
{ return dynamic_cast<NS_DT3308::ChildWidgets *>(currentWidgets); }

NS_SP9500X::ChildWidgets *MainWindow::widgetsSP9500X()
{ return dynamic_cast<NS_SP9500X::ChildWidgets *>(currentWidgets); }

void MainWindow::initProg(const QString name)
{
    QWinThread::g_threadName = name;
    labelProgressName->setText(name);
}

void MainWindow::initProg(const QString name, quint32 pts)
{
    initProg(name);
    mainProgressBar->setRange(0,int(pts));
    mainProgressBar->setValue(0);
}

void MainWindow::setProgPos(quint32 pos)
{
    mainProgressBar->setValue(int(pos));
}

void MainWindow::addProgPos(quint32 off)
{
    mainProgressBar->setValue(mainProgressBar->value() + int(off));
}

void MainWindow::setProgClr(const QColor clr)
{
    QString ss = QString("QProgressBar::chunk{background-color:rgb(%1,%2,%3)}")
                        .arg(clr.red())
                        .arg(clr.green())
                        .arg(clr.blue());
    mainProgressBar->setStyleSheet(mainProgressBar->styleSheet() + ss);
}

void MainWindow::updateMsgTable(int row)
{
    msgTableView->scrollToBottom();
    msgTableView->selectRow(row - 1);
}

void MainWindow::updateRegTable(int row)
{
    regTableView->scrollToBottom();
    regTableView->selectRow(row - 1);
}

void MainWindow::threadProcess(const QWinThread::Process p)
{
    switch (p) {
    case QWinThread::PREPARE : return setProgClr(Qt::darkGreen);
    case QWinThread::STARTED : return setProgClr(Qt::green);
    case QWinThread::RUNNING_EXCEPT :
    case QWinThread::END_EXCEPT : return setProgClr(Qt::red);
    case QWinThread::PAUSED : return setProgClr(Qt::yellow);
    default : return;
    }
}

void MainWindow::mainTree_itemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    disconnect(mainTab,&QTabWidget::currentChanged,this,&MainWindow::mainTab_currentChanged);
    currentWidgets->mainTreeItemClicked(item);
    connect(mainTab,&QTabWidget::currentChanged,this,&MainWindow::mainTab_currentChanged);
    mainTab->setCurrentIndex(int(RFIdx));
}

void MainWindow::mainTab_currentChanged(int index)
{
    if (index == -1) {
        return;
    }

    RFIdx = quint32(index);

    currentWidgets->updatePtr();
    currentWidgets->mainTabCurrentChanged(index);
}
