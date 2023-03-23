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

    void closeEvent(QCloseEvent *event);


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

    void on_wanLayers_doubleClicked(const QModelIndex &index);

    void on_wanSubLayers_doubleClicked(const QModelIndex &index);

    void on_action_Load_project_triggered();
    void on_action_Save_project_triggered();
    void on_actionSave_project_as_triggered();
    void openRecentproject();

private:
    Ui::MainWindow *ui;

    void loadSettings();
    void saveSettings();
    bool saveProject(QString filename);
    void setProjectFilename(QString fileName);
    void uiLoadRecentProjects();
    void addRecentProject(QString fileName);
    void loadProject(QString fileName);
    void addToDestinationList(QString destination);

    static const int DEFAULT_SizePDULayerIndex = 1;
    static const int DEFAULT_BWPDULayerIndex = 1;
    static const int DEFAULT_BandwidthUnitIndex = 2;


    QString m_projectFileName;
    static const int MAX_RECENT_PROJECTS = 5;
    static const int FORMAT_VERSION = 1;
    QStringList m_recentProjects;


    quint64 sendingCounter = 10000;
    quint64 receivedCounter = 0;
    // Refresch stats every STAT_PERIOD (in msec)
    static const int STAT_PERIOD = 1000;

    // Keep an history of maximel MAX_DESTINTATIONS destination hosts
    // We need to declare MAX_DESTINATIONS als consexpr because we use it in qMin wich passes its arguments as
    // a reference. C++17 makes an inline variable of it, so wie don't get an error at compilation time
    static constexpr int MAX_DESTINATIONS = 10;

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
