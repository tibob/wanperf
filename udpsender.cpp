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
    setBandwidth(100000000, NetworkModel::Layer2);
    setPduSize(512, NetworkModel::Layer2);

    connect(&m_thread, SIGNAL(statistics(qreal,qreal,quint64,int,int)), this, SLOT(receiveStatistics(qreal,qreal,quint64,int,int)));
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
    if (m_thread.setTos(tos)) {
        mTos = tos;
    }
    // if setting the TOS failed, do nothing.
}

void UdpSender::setDscp(quint8 dscp)
{
    setTos(dscp * 4);
}

quint8 UdpSender::dscp()
{
    return mTos / 4;
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

void UdpSender::setBandwidth(qreal bandwidth, NetworkModel::Layer bandwidthLayer)
{
    m_bandwidth = bandwidth;
    m_bandwidthLayer = bandwidthLayer;

    // We need to recalculate the amount of packets per second, as the Bandwidth changed
    m_ppmsec = m_networkModel.pps(m_pduSize, m_pduSizeLayer, m_bandwidth, m_bandwidthLayer) / 1000;

    m_thread.setPpmsec(m_ppmsec);
}

qreal UdpSender::specifiedBandwidth(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.bandwidth(m_bandwidth, m_bandwidthLayer, m_pduSize, m_pduSizeLayer, bandwidthLayer);
}

void UdpSender::setPduSize(uint pduSize, NetworkModel::Layer pduSizeLayer)
{
    uint l_pduSize;
    int l_datagramSDULength;
    qreal l_ppmsec;

    // To be sure that the PDU Size is valid, we let it run again the Network Model
    l_pduSize = m_networkModel.pduSize(pduSize, pduSizeLayer, pduSizeLayer);

    // Whe need to remove 8 bytes of the UDP Header to get the UDP Payload (datagram SDU) length.
    l_datagramSDULength = m_networkModel.pduSize(l_pduSize, pduSizeLayer, NetworkModel::Layer4) - 8;

    // We need to recalculate the amount of packets per second, as the PDU Size changed but not the Bandwidth
    l_ppmsec = m_networkModel.pps(l_pduSize, pduSizeLayer, m_bandwidth, m_bandwidthLayer) / 1000;

     if (m_thread.setDatagramSDULength(l_datagramSDULength) && m_thread.setPpmsec(l_ppmsec)) {
        m_pduSize = l_pduSize;
        m_pduSizeLayer = pduSizeLayer;
        m_datagramSDULength = l_datagramSDULength;
        m_ppmsec = l_ppmsec;
    }
    /* do nothing if the thread is running */
}

uint UdpSender::specifiedPduSize(NetworkModel::Layer pduLayer)
{
    return m_networkModel.pduSize(m_pduSize, m_pduSizeLayer, pduLayer);
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

qreal UdpSender::sendingBandwidth(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.bandwidth(m_statL4BandwidthSent, NetworkModel::Layer4, m_pduSize, m_pduSizeLayer, bandwidthLayer);
}

qreal UdpSender::receivingBandwidth(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.bandwidth(m_statL4BandwidthReceived, NetworkModel::Layer4, m_pduSize, m_pduSizeLayer, bandwidthLayer);
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

void UdpSender::receiveStatistics(qreal L4BandwidthSend, qreal L4BandwidthReceived, quint64 packetsLost, int ppsSent, int ppsReceived)
{
    m_statL4BandwidthReceived = L4BandwidthReceived;
    m_statL4BandwidthSent = L4BandwidthSend;
    m_PacketsLost = packetsLost;
    m_sentPps = ppsSent;
    m_receivedPps = ppsReceived;
}
