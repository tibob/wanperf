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
    setBandwidth(10000, NetworkModel::Layer4);
    setPduSize(500, NetworkModel::Layer4);

    m_lastStats = QDateTime::currentMSecsSinceEpoch();

    connect(&m_thread, SIGNAL(statistics(quint64,quint64,quint64)),
            this,      SLOT(receiveStatistics(quint64,quint64,quint64)));
}

void UdpSender::setNetworkModel(NetworkModel model)
{
    m_networkModel = model;
}

NetworkModel UdpSender::networkModel()
{
    return m_networkModel;
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
    qreal ppmsec;

    m_networkModel.setBandwidth(bandwidth, bandwidthLayer);

    // We need to recalculate the amount of packets per second, as the Bandwidth changed
    ppmsec = m_networkModel.pps() / 1000;
    m_thread.setPpmsec(ppmsec);
}

uint UdpSender::specifiedBandwidth(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.bandwidth(bandwidthLayer);
}

void UdpSender::setPduSize(uint pduSize, NetworkModel::Layer pduSizeLayer)
{
    uint udpPayloadLength;
    qreal ppmsec;

    m_networkModel.setPduSize(pduSize, pduSizeLayer);

    // Whe need to remove 8 bytes of the UDP Header to get the UDP Payload (datagram SDU) length.
    udpPayloadLength = m_networkModel.pduSize(NetworkModel::Layer4) - 8;
    m_thread.setDatagramSDULength(udpPayloadLength);

    // We need to recalculate the amount of packets per second, as the PDU Size changed but not the Bandwidth
    ppmsec = m_networkModel.pps() / 1000;
    m_thread.setPpmsec(ppmsec);
}

uint UdpSender::specifiedPduSize(NetworkModel::Layer pduLayer)
{
    return m_networkModel.pduSize(pduLayer);
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

void UdpSender::receiveStatistics(quint64 packetsLost, quint64 packetsSent, quint64 packetsReceived)
{
    qint64 statTime = QDateTime::currentMSecsSinceEpoch();

    if (statTime == m_lastStats) {
        // The last statistics were in the same msec. This should not occur, but perhaps the thread is busy
        // We ignore these Statistics
        qDebug() << "two stats at the same time...";
        return;
    }

    qint64 sendDelta = packetsSent - m_PacketsSend;
    qint64 receivedDelta = packetsReceived - m_PacketsReceived;
    qint64 timeDela = statTime - m_lastStats;

    m_sentPps     = 1000 * sendDelta / timeDela;
    m_receivedPps = 1000 * receivedDelta /timeDela;

    m_PacketsLost = packetsLost;
    m_PacketsSend = packetsSent;
    m_PacketsReceived = packetsReceived;
    m_lastStats = statTime;
}
