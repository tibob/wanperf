#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <stdio.h>
#include <string.h>
#include <QtEndian>
#include <QDebug>
#include <QSettings>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QMainWindow::showMaximized();

    // This come first, as it is called from signals when setting up the drop Down Menus
    senderListModel = new UdpSenderListModel();
    ui->udpSenderView->setModel(senderListModel);

    // Initialise QtCombos
    ui->bandwidthLayer->addItem("Layer 1", QVariant(NetworkModel::EthernetLayer1));
    ui->bandwidthLayer->addItem("Layer 2", QVariant(NetworkModel::EthernetLayer2));
    ui->bandwidthLayer->addItem("Layer 2 no CRC", QVariant(NetworkModel::EthernetLayer2woCRC));
    ui->bandwidthLayer->addItem("Layer 3", QVariant(NetworkModel::IPLayer));
    ui->bandwidthLayer->addItem("Layer 4", QVariant(NetworkModel::UDPLayer));
    ui->bandwidthLayer->setCurrentIndex(DEFAULT_BWPDULayerIndex);


    ui->bandwidthUnit->addItem("bit/s", QVariant(static_cast<int>(1)));
    ui->bandwidthUnit->addItem("Kbit/s", QVariant(static_cast<int>(1000)));
    ui->bandwidthUnit->addItem("Mbit/s", QVariant(static_cast<int>(1000000)));
    ui->bandwidthUnit->setCurrentIndex(DEFAULT_BandwidthUnitIndex);


    ui->sizeLayer->addItem("Layer 1", QVariant(NetworkModel::EthernetLayer1));
    ui->sizeLayer->addItem("Layer 2", QVariant(NetworkModel::EthernetLayer2));
    ui->sizeLayer->addItem("Layer 2 no CRC", QVariant(NetworkModel::EthernetLayer2woCRC));
    ui->sizeLayer->addItem("Layer 3", QVariant(NetworkModel::IPLayer));
    ui->sizeLayer->addItem("Layer 4", QVariant(NetworkModel::UDPLayer));
    ui->sizeLayer->setCurrentIndex(DEFAULT_SizePDULayerIndex);

    //Initialise one flow
    on_insertUdpSender_clicked();

    ui->udpSenderView->resizeColumnsToContents();
    ui->udpSenderView->resizeRowsToContents();
    ui->udpSenderView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->udpSenderView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    // Layers
    m_wanLayersModel = new NetworkLayerListModel();
    ui->wanLayers->setModel(m_wanLayersModel);
    ui->wanLayers->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->wanLayers->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    senderListModel->setWANLayerModel(m_wanLayersModel);

    m_wanSubLayersModel = new NetworkLayerListModel();
    ui->wanSubLayers->setModel(m_wanSubLayersModel);
    ui->wanSubLayers->setColumnHidden(1, true);
    ui->wanSubLayers->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->wanSubLayers->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_wanSubLayersModel->fillWithLayers(NetworkLayer::possibleSubLayers(NetworkLayer::IP));

    connect(m_wanLayersModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),
            this, SLOT(wanLayersChanged()));

    connect(m_wanLayersModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(wanLayersChanged()));

    connect(m_wanLayersModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(wanLayersChanged()));

    m_wanLayersModel->appendLayer(NetworkLayer::UDP);
    m_wanLayersModel->appendLayer(NetworkLayer::IP);
    m_wanLayersModel->appendLayer(NetworkLayer::EthernetL2);
    m_wanLayersModel->appendLayer(NetworkLayer::EthernetL1);

    loadSettings();

    // Refresh global stats every second
    QTimer *statsTimer = new QTimer(this);
    connect(statsTimer, SIGNAL(timeout()), this, SLOT(updateGlobalStats()));
    statsTimer->start(STAT_PERIOD);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

/** Connects to the remote device and initialise the udp responder
 */
void MainWindow::on_btnGenerate_clicked()
{
    if (m_isGeneratingTraffic) {
        senderListModel->stopAllSender();
        ui->destinationHost->setEnabled(true);
        ui->btnGenerate->setText("Generate traffic");
        ui->btnGenerate->setStyleSheet("");
        m_isGeneratingTraffic = false;
    } else {
        QHostAddress destinationIP = QHostAddress(ui->destinationHost->currentText());

        // NOTE: we should check this before starting generating
        if (destinationIP.protocol() != QAbstractSocket::IPv4Protocol) {
            // we only work with IPv4
            ui->lbStatus->setText("Not an IPv4 Address");
            return;
        } else {
            ui->lbStatus->setText("");
        }
        ui->destinationHost->setEnabled(false);
        senderListModel->setDestinationIP(destinationIP);
        senderListModel->generateTraffic();
        ui->btnGenerate->setText("Stop traffic");
        ui->btnGenerate->setStyleSheet("background-color: red");
        m_isGeneratingTraffic = true;
    }
}

void MainWindow::on_insertUdpSender_clicked()
{
    senderListModel->insertRow(senderListModel->rowCount());
    // If we do not repaint first, it does not adjust the row height.
    ui->udpSenderView->repaint();
    ui->udpSenderView->resizeRowsToContents();
}

void MainWindow::on_removeUdpSender_clicked()
{
    senderListModel->removeRow(ui->udpSenderView->currentIndex().row());
}

void MainWindow::on_sizeLayer_currentIndexChanged(int /* index */)
{

    senderListModel->setPDUSizeLayer(static_cast<NetworkModel::Layer>(ui->sizeLayer->currentData().toInt()));
}

void MainWindow::on_bandwidthLayer_currentIndexChanged(int /* index */)
{
    senderListModel->setBandwidthLayer(static_cast<NetworkModel::Layer>(ui->bandwidthLayer->currentData().toInt()));
}

void MainWindow::on_bandwidthUnit_currentIndexChanged(int /* index */)
{
    senderListModel->setBandwidthUnit(ui->bandwidthUnit->currentData().toInt());
}

void MainWindow::updateGlobalStats()
{
    NetworkModel::Layer layer = static_cast<NetworkModel::Layer>(ui->bandwidthLayer->currentData().toInt());
    qreal bandwidthUnit = ui->bandwidthUnit->currentData().toInt();

    ui->specifiedBandwidth->setText(locale.toString(senderListModel->totalSpecifiedBandwidth(layer)/bandwidthUnit, 'f', 2));
    ui->sendingTotal->setText(senderListModel->totalSendingStats());
    ui->receivingTotal->setText(senderListModel->totalReceivingStats());
    ui->packetsTotal->setText(senderListModel->totalPacketsStats());

    ui->WANReceivingTotal->setText(senderListModel->WANtotalReceivingStats());
    ui->WANSendingTotal->setText(senderListModel->WANtotalSendingStats());

}

void MainWindow::wanLayersChanged()
{
    m_wanSubLayersModel->fillWithLayers(NetworkLayer::possibleSubLayers(m_wanLayersModel->lastLayer()));

    senderListModel->WANLayerModelChanged();

    ui->udpSenderView->resizeRowsToContents();
}

void MainWindow::on_renoveLowestLayer_clicked()
{
    m_wanLayersModel->removeLastLayer();
}

void MainWindow::on_addLayer_clicked()
{
     QModelIndex index = ui->wanSubLayers->currentIndex();
     if (!index.isValid()) {
         return;
     }
     NetworkLayer::Layer layer = m_wanSubLayersModel->layerAt(index);
     m_wanLayersModel->appendLayer(layer);
}

void MainWindow::on_wanLayers_doubleClicked(const QModelIndex &index)
{
    if (m_wanLayersModel->isLastLayer(index)) {
        m_wanLayersModel->removeLastLayer();
    }
}

void MainWindow::on_wanSubLayers_doubleClicked(const QModelIndex &index)
{
    m_wanLayersModel->appendLayer(m_wanSubLayersModel->layerAt(index));
}

/** Load the global settings of wanperf, saved with saveSettings()

  The settings are loaded from the organisation and application defined in main.cpp:
   a.setOrganizationName("wanperf");
   a.setApplicationName("wanperf");

 */
void MainWindow::loadSettings()
{
    QSettings settings;

    QStringList destinationList;
    destinationList = settings.value("destinationlist", QStringList()).toStringList();
    ui->destinationHost->addItems(destinationList);

    int index;
    index = settings.value("SizePDULayerIndex", DEFAULT_SizePDULayerIndex).toInt();
    index = qMax(index, 0);
    index = qMin(index, ui->sizeLayer->count()-1);
    ui->sizeLayer->setCurrentIndex(index);

    index = settings.value("BandwidthPDULayerIndex", DEFAULT_BWPDULayerIndex).toInt();
    index = qMax(index, 0);
    index = qMin(index, ui->bandwidthLayer->count()-1);
    ui->bandwidthLayer->setCurrentIndex(index);

    index = settings.value("BandwidthUnitIndex", DEFAULT_BandwidthUnitIndex).toInt();
    index = qMax(index, 0);
    index = qMin(index, ui->bandwidthUnit->count()-1);
    ui->bandwidthUnit->setCurrentIndex(index);

    // List of the last projects saved
    m_recentProjects = settings.value("recentProjects", QStringList()).toStringList();
    uiLoadRecentProjects();
}

/** Save the global settings of wanperf

  The settings are saved into the organisation and application defined in main.cpp:
   a.setOrganizationName("wanperf");
   a.setApplicationName("wanperf");
*/
void MainWindow::saveSettings()
{
    QSettings settings;
    QStringList destinationList;

    for (int index = 0; index < qMin(ui->destinationHost->count(), MAX_DESTINATIONS); index++) {
        destinationList.append(ui->destinationHost->itemText(index));
    }
    settings.setValue("destinationlist", destinationList);

    settings.setValue("SizePDULayerIndex", ui->sizeLayer->currentIndex());
    settings.setValue("BandwidthPDULayerIndex", ui->bandwidthLayer->currentIndex());
    settings.setValue("BandwidthUnitIndex", ui->bandwidthUnit->currentIndex());

    settings.setValue("recentProjects", m_recentProjects);
}

/** This private method is used to avoid duplicate code between save and save as.
*/
bool MainWindow::saveProject(QString filename)
{
    if (filename.length() == 0) {
        return false;
    }
    QSettings settings(filename, QSettings::IniFormat);

    // Remove old settings if the project file was already present.
    // If we do not clear(), QSettings will merge the settings together, which we do not want.
    settings.clear();
    settings.setValue("version", FORMAT_VERSION);
    settings.setValue("SizePDULayerIndex", ui->sizeLayer->currentIndex());
    settings.setValue("BandwidthPDULayerIndex", ui->bandwidthLayer->currentIndex());
    settings.setValue("BandwidthUnitIndex", ui->bandwidthUnit->currentIndex());

    // BUG: save destination host

    senderListModel->saveParameter(settings);

    // BUG We also need to save the WAN Model

    settings.sync();


    if (settings.status() == QSettings::NoError) {
        return true;
    } else {
        // else, we signal a failure
        QMessageBox::critical(this,
            "Could not save this project",
            QString("A problem occured while saving file \"%1\". The project could not be saved")
                          .arg(filename));

        return false;
    }
}

/** Saves the filename to the private class variable and displays it in the window title */
void MainWindow::setProjectFilename(QString fileName)
{
    QFileInfo fi(fileName);

    m_projectFileName = fi.filePath();

    if (fileName.length() > 0) {
        setWindowTitle(tr("wanperf trafic generator - %1").arg(fi.fileName()));
        addRecentProject(m_projectFileName);
    } else {
        setWindowTitle(tr("wanperf trafic generator - new project"));
    }
}

/** loads m_recentProjects into the recentProject menu in the ui */
void MainWindow::uiLoadRecentProjects()
{
    ui->recentProjects->clear();
    if (m_recentProjects.size() == 0) {
        ui->recentProjects->setEnabled(false);
    } else {
        ui->recentProjects->setEnabled(true);
        QAction *action;
        QString project;
        foreach (project, m_recentProjects) {
            action = ui->recentProjects->addAction(project);
            // when the action is triggered. load the project file
            connect(action, SIGNAL(triggered()), this, SLOT(openRecentproject()));
        }
    }
}

/** Adds fileName to the list of recent projects.
*/
void MainWindow::addRecentProject(QString fileName)
{
    if (fileName.length() == 0)
        return;

    /* remove the file from the list to avoid duplicates */
    m_recentProjects.removeAll(fileName);
    m_recentProjects.prepend(fileName);

    /* reduce size of list to MAX_RECENT_PROJECTS */
    while (m_recentProjects.size() > MAX_RECENT_PROJECTS) {
        m_recentProjects.removeLast();
    }

    // Reload the list of recent projects
    uiLoadRecentProjects();
}

/** Loads the previous saved project from fileName

    FIXME: test when Filename not readable or not in the right format
*/
void MainWindow::loadProject(QString fileName)
{
    if (fileName.length() == 0)
        return;

    if (m_isGeneratingTraffic) {
        senderListModel->stopAllSender();
        ui->destinationHost->setEnabled(true);
        ui->btnGenerate->setText("Generate traffic");
        ui->btnGenerate->setStyleSheet("");
        m_isGeneratingTraffic = false;
    }

    QSettings settings(fileName, QSettings::IniFormat);

    int version = settings.value("version", -1).toInt();

    if (settings.status() != QSettings::NoError || version == -1) {
        // There was a problem reading this file or the version is not present in the ini file
        QMessageBox::critical(this,
            "Could not load this project",
            QString("A problem occured while loading file \"%1\". The project could not be loaded")
                          .arg(fileName));

        return;
    }

    ui->sizeLayer->setCurrentIndex(settings.value("SizePDULayerIndex",DEFAULT_SizePDULayerIndex).toInt());
    ui->bandwidthLayer->setCurrentIndex(settings.value("BandwidthPDULayerIndex", DEFAULT_BWPDULayerIndex).toInt());
    ui->bandwidthUnit->setCurrentIndex(settings.value("BandwidthUnitIndex", DEFAULT_BandwidthUnitIndex).toInt());

    senderListModel->loadParameter(settings);

    setProjectFilename(fileName);
}

void MainWindow::on_action_Load_project_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                        tr("Choose project"),
                        QDir::currentPath(),   // FIXME: save last path
                        "wanperf projects (*.wanperf);;All files (* *.*");
    if (fileName.length() == 0) // Cancel pressed
        return;

    loadProject(fileName);
}

void MainWindow::on_action_Save_project_triggered()
{
    if (m_projectFileName.length() > 0) {
        saveProject(m_projectFileName);
    } else {
        on_actionSave_project_as_triggered();
    }
}

void MainWindow::on_actionSave_project_as_triggered()
{
    QString fileName;

    fileName = QFileDialog::getSaveFileName(this, "Save File",
                               // FIXME: use last project name
                               // FIXME: save last path
                               QDir::currentPath() + "/project.wanperf",
                               "wanperf projects (*.wanperf);;All files (* *.*");

    if (fileName.length() == 0) // Cancel pressed
        return;

    if (saveProject(fileName) == true) {
        // only set the project filename if the project could be saved
        setProjectFilename(fileName);
    }
}

/** Loads the selected recent project */
void MainWindow::openRecentproject()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        loadProject(action->text());
    }
}
