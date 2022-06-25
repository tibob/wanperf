#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QAbstractItemModel>

#include "udpsender.h"
#include "udpsenderlistmodel.h"
#include "networklayerlistmodel.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:



    void updateGlobalStats();
    void wanLayersChanged();

private slots:
    void on_insertUdpSender_clicked();
    void on_removeUdpSender_clicked();
    void on_sizeLayer_currentIndexChanged(int index);
    void on_bandwidthLayer_currentIndexChanged(int index);
    void on_bandwidthUnit_currentIndexChanged(int index);
    void on_btnGenerate_clicked();
    void on_renoveLowestLayer_clicked();
    void on_addLayer_clicked();

private:
    Ui::MainWindow *ui;
    quint64 sendingCounter = 10000;
    quint64 receivedCounter = 0;

    enum flowColumns {
        COL_NAME = 0,
        COL_PORT = 1,
        COL_BANDWIDTH = 2,
        COL_DSCP = 3,
        COL_SIZE = 4
    };

    UdpSenderListModel *senderListModel;
    NetworkLayerListModel *m_wanLayersModel;
    NetworkLayerListModel *m_wanSubLayersModel;

    // use locale to display numbers correctly
    QLocale locale;

    bool m_isGeneratingTraffic = false;
};

#endif // MAINWINDOW_H
