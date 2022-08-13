#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <stdio.h>
#include <string.h>
#include <QtEndian>
#include <QDebug>





MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // This come first, as it is called from signals when setting up the drop Down Menus
    senderListModel = new UdpSenderListModel();
    ui->udpSenderView->setModel(senderListModel);

    // Initialise QtCombos
    ui->bandwidthLayer->addItem("Layer 1", QVariant(NetworkModel::EthernetLayer1));
    ui->bandwidthLayer->addItem("Layer 2", QVariant(NetworkModel::EthernetLayer2));
    ui->bandwidthLayer->addItem("Layer 2 no CRC", QVariant(NetworkModel::EthernetLayer2woCRC));
    ui->bandwidthLayer->addItem("Layer 3", QVariant(NetworkModel::IPLayer));
    ui->bandwidthLayer->addItem("Layer 4", QVariant(NetworkModel::UDPLayer));
    ui->bandwidthLayer->setCurrentIndex(1);


    ui->bandwidthUnit->addItem("bit/s", QVariant(static_cast<int>(1)));
    ui->bandwidthUnit->addItem("Kbit/s", QVariant(static_cast<int>(1000)));
    ui->bandwidthUnit->addItem("Mbit/s", QVariant(static_cast<int>(1000000)));
    ui->bandwidthUnit->setCurrentIndex(2);


    ui->sizeLayer->addItem("Layer 1", QVariant(NetworkModel::EthernetLayer1));
    ui->sizeLayer->addItem("Layer 2", QVariant(NetworkModel::EthernetLayer2));
    ui->sizeLayer->addItem("Layer 2 no CRC", QVariant(NetworkModel::EthernetLayer2woCRC));
    ui->sizeLayer->addItem("Layer 3", QVariant(NetworkModel::IPLayer));
    ui->sizeLayer->addItem("Layer 4", QVariant(NetworkModel::UDPLayer));
    ui->sizeLayer->setCurrentIndex(1);

    //Initialise one flow
    on_insertUdpSender_clicked();

    ui->udpSenderView->resizeColumnsToContents();
    ui->udpSenderView->resizeRowsToContents();
    ui->udpSenderView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->udpSenderView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    // Layers
    m_wanLayersModel = new NetworkLayerListModel();
    ui->wanLayers->setModel(m_wanLayersModel);

    senderListModel->setWANLayerModel(m_wanLayersModel);

    m_wanSubLayersModel = new NetworkLayerListModel();
    ui->wanSubLayers->setModel(m_wanSubLayersModel);
    ui->wanSubLayers->setColumnHidden(1, true);
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

    // Refresh global stats every second
    QTimer *statsTimer = new QTimer(this);
    connect(statsTimer, SIGNAL(timeout()), this, SLOT(updateGlobalStats()));
    statsTimer->start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*!
 * \brief MainWindow::on_btnGenerate_clicked
 *
 * Connects to the remote device and initialise the udp responder
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
        QHostAddress destinationIP = QHostAddress(ui->destinationHost->text());

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
