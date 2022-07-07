#include "udpsender.h"
#include <QtEndian>
#include <stdio.h>
#include <string.h>
#include <QDateTime>
#include <QDebug>

UdpSender::UdpSender(QObject *parent) :
    QObject(parent)
{
    m_id = QUuid::createUuid();

    setDestination(QHostAddress::LocalHost);

    // Initialize with a default Networkmodel
    setNetworkModel(NetworkModel());

    // Initialise PDU Size & Bandwidth to some Value
    setBandwidth(1000000, NetworkModel::EthernetLayer2);
    setPduSize(512, NetworkModel::EthernetLayer2);

    m_lastStats = QDateTime::currentMSecsSinceEpoch();

    connect(&m_thread, SIGNAL(statistics(quint64,quint64,quint64,quint64)),
            this,      SLOT(receiveStatistics(quint64,quint64,quint64,quint64)));
}

UdpSender::~UdpSender()
{
    // be sure to stop the thread or get a segfault.
    m_thread.stop();
    if (m_WANNetworkModel) {
        delete m_WANNetworkModel;
    }
}

void UdpSender::setNetworkModel(NetworkModel model)
{
    m_networkModel = model;
}

NetworkModel UdpSender::networkModel()
{
    return m_networkModel;
}

void UdpSender::setWANLayerModel(NetworkLayerListModel *model)
{
    // Do nothing if there is no model
    if (model == NULL)
        return;

    if (m_WANNetworkModel) {
        delete m_WANNetworkModel;
    }
    m_WANNetworkModel = model->clone();

    m_WANNetworkModel->setUDPPDUSize(m_specUDPPDUSize);
}

void UdpSender::setDestination(QHostAddress address)
{
    m_destination = address;

    m_thread.setDestination(address);
}

void UdpSender::setPort(int udpPort)
{
    if (m_thread.setPort(udpPort)) {
        m_udpPort = udpPort;
    }
    // if setting the port failed, do nothing.
}

int UdpSender::port()
{
    return m_udpPort;
}

void UdpSender::setTos(quint8 tos)
{
    m_tos = tos;
    m_thread.setTos(tos);
}

void UdpSender::setDscp(quint8 dscp)
{
    setTos(dscp * 4);
}

quint8 UdpSender::dscp()
{
    return m_tos / 4;
}

void UdpSender::setName(QString newName)
{
    m_Name = newName;
}

QString UdpSender::name()
{
    return m_Name;
}

QUuid UdpSender::id()
{
    return m_id;
}

void UdpSender::setBandwidth(uint bandwidth, NetworkModel::Layer bandwidthLayer)
{
    m_networkModel.setBandwidth(bandwidth, bandwidthLayer);

    // We need to recalculate the amount of packets per second, as the Bandwidth changed
    m_specPps = m_networkModel.pps();
    m_thread.setPpmsec(m_specPps / 1000);
}

uint UdpSender::specifiedBandwidth(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.bandwidth(bandwidthLayer);
}

void UdpSender::setPduSize(uint pduSize, NetworkModel::Layer pduSizeLayer)
{
    uint udpPayloadLength;

    m_networkModel.setPduSize(pduSize, pduSizeLayer);


    m_specUDPPDUSize = m_networkModel.pduSize(NetworkModel::UDPLayer);
    if (m_WANNetworkModel) {
        m_WANNetworkModel->setUDPPDUSize(m_specUDPPDUSize);
    }

    // Whe need to remove 8 bytes of the UDP Header to get the UDP Payload (datagram SDU) length.
    udpPayloadLength = m_specUDPPDUSize - 8;
    m_thread.setDatagramSDULength(udpPayloadLength);

    // We need to recalculate the amount of packets per second, as the PDU Size changed but not the Bandwidth
    m_specPps = m_networkModel.pps();
    m_thread.setPpmsec(m_specPps / 1000);
}

uint UdpSender::specifiedPduSize(NetworkModel::Layer pduLayer)
{
    return m_networkModel.pduSize(pduLayer);
}

void UdpSender::setTcMsec(uint tc)
{
    // Tc cannot be zero
    if (tc < 1)
        tc = 1;
    // Tc should not be superior to 1 second (or we may have to change some code like packet loss detection)
    if (tc > 1000)
        tc = 1000;

    m_tcMsec = tc;

    m_thread.setTcMsec(m_tcMsec);
}

uint UdpSender::tcMsec()
{
    return  m_tcMsec;
}

void UdpSender::startTraffic()
{
    if (m_thread.isRunning()) {
        /* Test already running */
        return;
    }
    m_thread.start();
}

void UdpSender::stopTraffic()
{
    m_thread.stop();
}

uint UdpSender::sendingBandwidth(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.pps2bandwidth(m_sentPps, bandwidthLayer);
}

uint UdpSender::receivingBandwidth(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.pps2bandwidth(m_receivedPps, bandwidthLayer);
}

int UdpSender::sendingPps()
{
    return m_sentPps;
}

int UdpSender::receivingPps()
{
    return m_receivedPps;
}

int UdpSender::packetLost()
{
    return m_PacketsLost;
}

int UdpSender::packetsSent()
{
    return m_PacketsSent;
}

int UdpSender::packetsReceived()
{
    return m_PacketsReceived;
}

int UdpSender::packetsNotSent()
{
    return m_PacketsNotSent;
}

QList<int> UdpSender::WANsendingBandwidth()
{
    uint PDUsize;
    QList<int> l;
    QList<uint> pduList;

    if (m_WANNetworkModel == NULL) {
        return l;
    }

    pduList = m_WANNetworkModel->layerPDUSize();

    if (pduList.isEmpty()) {
        return l;
    }

    foreach (PDUsize, pduList) {
        l.append(PDUsize * 8 * m_sentPps);
    }

    return l;
}

QList<int> UdpSender::WANreceivingBandwidth()
{
    uint PDUsize;
    QList<int> l;
    QList<uint> pduList;

    if (m_WANNetworkModel == NULL) {
        return l;
    }

    pduList = m_WANNetworkModel->layerPDUSize();

    foreach (PDUsize, pduList) {
        l.append(PDUsize * 8 * m_receivedPps);
    }

    return l;
}

void UdpSender::receiveStatistics(quint64 packetsLost, quint64 packetsSent,
                                  quint64 packetsReceived, quint64 packetsNotSent)
{
    qint64 statTime = QDateTime::currentMSecsSinceEpoch();

    if (statTime == m_lastStats) {
        // The last statistics were in the same msec. This should not occur, but perhaps the thread is busy
        // We ignore these Statistics
        qDebug() << "two stats at the same time...";
        return;
    }

    qint64 sendDelta = packetsSent - m_PacketsSent;
    qint64 receivedDelta = packetsReceived - m_PacketsReceived;
    qint64 timeDela = statTime - m_lastStats;

    m_sentPps     = 1000 * sendDelta / timeDela;
    m_receivedPps = 1000 * receivedDelta /timeDela;

    m_PacketsLost = packetsLost;
    m_PacketsSent = packetsSent;
    m_PacketsReceived = packetsReceived;
    m_PacketsNotSent = packetsNotSent;
    m_lastStats = statTime;
}
