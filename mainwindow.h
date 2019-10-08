#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QAbstractItemModel>

#include "udpsender.h"
#include "wsclient.h"
#include "udpsenderlistmodel.h"

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
    void on_insertUdpSender_clicked();
    void on_removeUdpSender_clicked();
    void on_sizeLayer_currentIndexChanged(int index);
    void on_bandwidthLayer_currentIndexChanged(int index);
    void on_bandwidthUnit_currentIndexChanged(int index);

    void on_btnConnect_clicked();
    void on_btnDisconnect_clicked();

    void wsClientStatusChanged();

    void updateGlobalStats();

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

    WsClient wsClient;

    UdpSenderListModel *senderListModel;

    // use locale to display numbers correctly
    QLocale locale;

};

#endif // MAINWINDOW_H
