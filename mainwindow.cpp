#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "udpsenderlistmodel.h"

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

    // This come first, as it is colled fromn signals when setting up the drop Down Menus
    senderListModel = new UdpSenderListModel();
    ui->udpSenderView->setModel(senderListModel);
    senderListModel->setWsClient(&wsClient);

    // Bandwidth should be an int and >= 0
    QIntValidator *portValidator = new QIntValidator(ui->destinationPort);
    portValidator->setBottom(0);
    portValidator->setTop(65535);
    ui->destinationPort->setValidator(portValidator);

    // Initialise QtCombos
    ui->bandwidthLayer->addItem("Layer 1", QVariant(NetworkModel::Layer1));
    ui->bandwidthLayer->addItem("Layer 2", QVariant(NetworkModel::Layer2));
    ui->bandwidthLayer->addItem("Layer 3", QVariant(NetworkModel::Layer3));
    ui->bandwidthLayer->addItem("Layer 4", QVariant(NetworkModel::Layer4));
    ui->bandwidthLayer->setCurrentIndex(2);


    ui->bandwidthUnit->addItem("bit/s", QVariant(static_cast<int>(1)));
    ui->bandwidthUnit->addItem("Kbit/s", QVariant(static_cast<int>(1000)));
    ui->bandwidthUnit->addItem("Mbit/s", QVariant(static_cast<int>(1000000)));
    ui->bandwidthUnit->setCurrentIndex(1);


    ui->sizeLayer->addItem("Layer 1", QVariant(NetworkModel::Layer1));
    ui->sizeLayer->addItem("Layer 2", QVariant(NetworkModel::Layer2));
    ui->sizeLayer->addItem("Layer 3", QVariant(NetworkModel::Layer3));
    ui->sizeLayer->addItem("Layer 4", QVariant(NetworkModel::Layer4));
    ui->sizeLayer->setCurrentIndex(2);

    //Initialise one flow
    senderListModel->insertRow(0);

    ui->udpSenderView->resizeColumnsToContents();
    ui->udpSenderView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->udpSenderView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    connect(&wsClient, SIGNAL(statusChanged()), this, SLOT(wsClientStatusChanged()));

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
 * \brief MainWindow::on_btnConnect_clicked
 *
 * Connects to the remote device and initialise the udp responder
 */
void MainWindow::on_btnConnect_clicked()
{
    senderListModel->stopAllSender();

    QHostAddress destinationIP = QHostAddress(ui->destinationHost->text());

    if (destinationIP.protocol() != QAbstractSocket::IPv4Protocol) {
        // we only work with IPv4
        ui->lbStatus->setText("Not an IPv4 Address");
    }

    ui->destinationHost->setEnabled(false);
    ui->destinationPort->setEnabled(false);

    senderListModel->setDestinationIP(destinationIP);
    senderListModel->setGeneratingTrafficStatus(true);
    wsClient.connectRemoteToSetUp(ui->destinationHost->text());
}

void MainWindow::on_btnDisconnect_clicked()
{
    ui->lbStatus->setText("Stopping...");

    senderListModel->stopAllSender();

    wsClient.connectRemoteToClose(ui->destinationHost->text());

    senderListModel->setGeneratingTrafficStatus(false);

    ui->destinationHost->setEnabled(true);
    ui->destinationPort->setEnabled(true);

}

void MainWindow::on_insertUdpSender_clicked()
{
    senderListModel->insertRow(senderListModel->rowCount());
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

void MainWindow::wsClientStatusChanged()
{
    ui->lbStatus->setText(wsClient.statusString());
}

void MainWindow::updateGlobalStats()
{
    NetworkModel::Layer layer = static_cast<NetworkModel::Layer>(ui->bandwidthLayer->currentData().toInt());
    qreal bandwidthUnit = ui->bandwidthUnit->currentData().toInt();

    ui->specifiedBandwidth->setText(locale.toString(senderListModel->totalSpecifiedBandwidth(layer)/bandwidthUnit, 'f', 2));
    ui->sendingBandwidth->setText(locale.toString(senderListModel->totalSendingBandwidth(layer)/bandwidthUnit, 'f', 2));
    ui->receivingBandwidth->setText(locale.toString(senderListModel->totalReceivingBandwidth(layer)/bandwidthUnit, 'f', 2));
    ui->packetLost->setText(locale.toString(senderListModel->totalPacketLost()));
    ui->sendingPps->setText(locale.toString(senderListModel->totalPpsSent()));
    ui->receivingPps->setText(locale.toString(senderListModel->totalPpsReceived()));
}
