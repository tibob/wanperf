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
    setBandwidth(100000, NetworkModel::Layer3);
    setPduSize(1000, NetworkModel::Layer3);

    /***** Statistics ****/
    // As we read stats for the last second, the reference date sould begin one second before the creator was called.
    // To be completly sure, I take 2 Seconds
    m_referenceDate = QDateTime::currentDateTime().toTime_t() - 2;

    // Statistics
    m_msecPreviousStats = QDateTime::currentMSecsSinceEpoch();

    // Run stats every statsCalculationDelay
    QTimer *statsTimer = new QTimer(this);
    connect(statsTimer, SIGNAL(timeout()), this, SLOT(calculateStatistics()));
    statsTimer->start(statsCalculationDelay);


    // NOTE: This is a workaround so that DSCP-Values are used => force IPv4
    // Qt curentliy does not support DSCP-Values in IPv6 nor when we are using QAbstractSocket::AnyIPProtocol
    // In the future: Toggle between IPv4 and IPv6 in UI
    if (!udpSocket.bind(QHostAddress(QHostAddress::AnyIPv4))) {
//    if (!udpSocket.bind(QHostAddress(QHostAddress::Any))) {
        qDebug() << "UdpSender::UdpSender! Could not bind the socket!";
        qDebug() << udpSocket.error();
        qDebug() << udpSocket.errorString();
    }

    qDebug() << udpSocket.localAddress();
    qDebug() << udpSocket.localPort();

    connect (&udpSocket, SIGNAL(readyRead()),
             this, SLOT(readPendingDatagrams()));
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
    destination = address;
}

void UdpSender::setPort(int udpPort)
{
    m_udpPort = udpPort;
}

int UdpSender::port()
{
    return m_udpPort;
}

void UdpSender::setTos(quint8 tos)
{
    mTos = tos;

    udpSocket.setSocketOption(QAbstractSocket::TypeOfServiceOption, tos);
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

void UdpSender::setWsClient(WsClient *wsClient)
{
    m_wsClient = wsClient;

    // Receive messages from Remote
    connect(wsClient, SIGNAL(remoteConnectedForSetUp()), this, SLOT(remoteConnectedForSetUp()));
    connect(wsClient, SIGNAL(udpEchoConnected(QUuid)), this, SLOT(udpEchoConnected(QUuid)));
}

void UdpSender::setBandwidth(qreal bandwidth, NetworkModel::Layer bandwidthLayer)
{
    m_bandwidth = bandwidth;
    m_bandwidthLayer = bandwidthLayer;

    // We need to recalculate the amount of packets per second, as the Bandwidth changed
    m_ppmsec = m_networkModel.pps(m_pduSize, m_pduSizeLayer, m_bandwidth, m_bandwidthLayer) / 1000;
}

qreal UdpSender::specifiedBandwidth(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.bandwidth(m_bandwidth, m_bandwidthLayer, m_pduSize, m_pduSizeLayer, bandwidthLayer);
}

void UdpSender::setPduSize(uint pduSize, NetworkModel::Layer pduSizeLayer)
{
    // To be sure that the PDU Size is valid, we let it run again the Network Model
    m_pduSize = m_networkModel.pduSize(pduSize, pduSizeLayer, pduSizeLayer);
    m_pduSizeLayer = pduSizeLayer;

    // Whe need to remove 8 bytes of the UDP Header to get the UDP Payload (datagram SDU) length.
    datagramSDULength = m_networkModel.pduSize(m_pduSize, m_pduSizeLayer, NetworkModel::Layer4) - 8;

    // FIXME: do I want this or fall back on Qt Funktions?
    if (datagram) {
        free(datagram);
    }
    datagram = (char *) malloc(datagramSDULength);

    // We need to recalculate the amount of packets per second, as the PDU Size changed but not the Bandwidth
    m_ppmsec = m_networkModel.pps(m_pduSize, m_pduSizeLayer, m_bandwidth, m_bandwidthLayer) / 1000;
}

uint UdpSender::specifiedPduSize(NetworkModel::Layer pduLayer)
{
    return m_networkModel.pduSize(m_pduSize, m_pduSizeLayer, pduLayer);
}

void UdpSender::sendOneDatagram()
{
    // NOTE: this optimisation should be counterchecked in setNetworkmodell
//    Optimisation: this check is not needed because this will never happen (minimal datagram size: 18 Bytes).
//    if (datagram == NULL || datagramLength < 16) {
//        return;
//    }

    qint64 msecNow = QDateTime::currentMSecsSinceEpoch();

    // Save Timestamp
    qToBigEndian(msecNow, reinterpret_cast<uchar *>(datagram));

    // Save Counter
    qToBigEndian(m_sendingCounter, reinterpret_cast<uchar *>(datagram + 8));

    // FIXME: use connected socket in order to use TOS-Value (?) and specify source-Port
    udpSocket.writeDatagram(datagram, datagramSDULength, destination, m_udpPort);

    m_sendingCounter++;
}

void UdpSender::startTraffic()
{
    if (timer != NULL) {
        /* Test already running */
        return;
    }
    msecLast = QDateTime::currentMSecsSinceEpoch();
    timer = new QTimer(this);
    // We need the Timer to be as precise as possible
    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, SIGNAL(timeout()), this, SLOT(sendUdpDatagrams()));
    /* adapt Timer resolution ? */
    timer->start(timerResolution);

    /* TODO: Status sending */
}

void UdpSender::stopTraffic()
{
    if (timer) {
        timer->stop();
        delete timer;
        timer = NULL;
    }
}

qreal UdpSender::sendingRate(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.bandwidth(m_statL4BandwidthSent, NetworkModel::Layer4, m_pduSize, m_pduSizeLayer, bandwidthLayer);
}

qreal UdpSender::receivingRate(NetworkModel::Layer bandwidthLayer)
{
    return m_networkModel.bandwidth(m_statL4BandwidthReceived, NetworkModel::Layer4, m_pduSize, m_pduSizeLayer, bandwidthLayer);
}

int UdpSender::packetLost()
{
    return m_PacketsLost;
}

QString UdpSender::connectionStatus()
{
    switch (m_status){
    case sRemoteDisconnected:
        return "Remote disconected";
    case sConnectingUdpEcho:
        return "Connecting Flow";
    case sUdpEchoConnected:
        return "Flow connected";
    case sError:
        return "Error";
    }

    return "no defined";
}

bool UdpSender::isConnected()
{
    return m_status == sUdpEchoConnected;
}

void UdpSender::sendUdpDatagrams()
{
    qint64 msecNow = QDateTime::currentMSecsSinceEpoch();
    qint64 msecDelta = msecNow - msecLast;

    packets2Send = msecDelta * m_ppmsec + packets2Send;
    // Stats
    m_L4BytesSent += (int)packets2Send * (datagramSDULength + 8);

    // we never exceed the bandwidth: only send when packets2Send > 1
    // FIXME we should allow microbursts or we will never reach the specified Bandwidth
    while (packets2Send > 1) {
        sendOneDatagram();
        packets2Send--;
    }
    msecLast = msecNow;
}

void UdpSender::readPendingDatagrams()
{
    QHostAddress sender;
    quint16 senderPort;
    char buffer[1500];
    quint64 receivedCounter;

    qint64 msecNow = QDateTime::currentMSecsSinceEpoch();
    qint64 msecSend;

    qint64 latency;

    qint64 datagramSize = udpSocket.pendingDatagramSize();
    while (datagramSize != -1) {
        //We just need the first 16 bytes, read timestamp & counter.
        udpSocket.readDatagram(buffer, 16, &sender, &senderPort);
        msecSend = qFromBigEndian<quint64>(reinterpret_cast<const uchar *>(buffer));
        receivedCounter = qFromBigEndian<quint64>(reinterpret_cast<const uchar *>(buffer + 8));


        // Stats calculation
        latency = msecNow - msecSend;

//        qDebug() << "Awaited " << m_awaitedCounter << "Received: " << receivedCounter << " Current" << m_sendingCounter
//                 << "Latency" << latency << "SDU" << datagramSize;

        // We only work wih latency < 2 sec
        if (latency >=0 && latency < 2000
                //, the datagram Size must be the same as sending
                && datagramSize == datagramSDULength
                // and the received Counter must me smaller as the curent sending counter
                && receivedCounter < m_sendingCounter) {
            if (receivedCounter == m_awaitedCounter) {
                // This is the awaited Paket
                m_awaitedCounter++;
                m_PacketsReceived++;
                m_CummuledLatency += latency;
                m_L4BytesReceived += datagramSDULength + 8; // PDU = Header (8 Bytes) + SDU
            } else if (receivedCounter > m_awaitedCounter) {
                qDebug() << "Loss at " << receivedCounter << "Awaited " << m_awaitedCounter << " Current" << m_sendingCounter;

                // One correct packet received, but Packet loss !
                m_PacketsLost += receivedCounter - m_awaitedCounter;
                m_PacketsReceived++;
                m_CummuledLatency += latency;
                m_L4BytesReceived += datagramSDULength + 8; // PDU = Header (8 Bytes) + SDU
                m_awaitedCounter = receivedCounter + 1;

            }
            // else {
                // We have a Packet duplication or a reordered packet => we ignore it
            //}
        }

//        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss,zzz") +
//                                " Datagram received from " + sender.toString() +
//                                " Counter " + QString::number(receivedCounter) +
//                                " Size " + QString::number(datagramSize) +
//                                " Send at " + QDateTime::fromMSecsSinceEpoch(msecSend).toString("hh:mm:ss,zzz");

        // read next datagram Size
        datagramSize = udpSocket.pendingDatagramSize();
    }
}

void UdpSender::remoteConnectedForSetUp()
{
    m_wsClient->connectUdpEcho(m_id, m_udpPort, mTos);
    m_status = UdpSender::sConnectingUdpEcho;
    emit connectionStatusChanged();
}

void UdpSender::udpEchoConnected(QUuid id)
{
    if (id != m_id) /* The message is not for this UdpSender */
        return;

    m_status = UdpSender::sUdpEchoConnected;
    emit connectionStatusChanged();
}

void UdpSender::calculateStatistics()
{
    qint64 msecNow = QDateTime::currentMSecsSinceEpoch();
    qint64 deltaTime = msecNow - m_msecPreviousStats;

    /* we use quint64 for the delta value so that an overflow on the counter (m_L4BytesSend reaches 2^64)
       still returns the correct difference
       Example with quint8; 2^8 = 256:
          Slice A: 250 - 200 = 50
          Next value would be (250 + 50) modulo 256 = 44
          Slice B: 44  - 250 = 50 (  44 - 250 = -206; -206 modulo 256 = 50)
    */

    quint64 deltaL4BytesSend = m_L4BytesSent - m_previousL4BytesSent;
    // As deltatime is in msec, we have to divide it throught 1000
    m_statL4BandwidthSent =  deltaL4BytesSend * 8 * 1000 / deltaTime;

    quint64 deltaL4BytesReceived = m_L4BytesReceived - m_previousL4BytesReceived;
    // As deltatime is in msec, we have to divide it throught 1000
    m_statL4BandwidthReceived =  deltaL4BytesReceived * 8 * 1000 / deltaTime;

    quint64 deltaPacketsReceived = m_PacketsReceived - m_previousPacketsReceived;
    if (deltaPacketsReceived != 0) {
        m_statAverageLatency = m_CummuledLatency / deltaPacketsReceived;
    } else {
        m_statAverageLatency = 0;
    }

    // set the "previous" values to the actual value for the next statistic
    m_previousL4BytesSent = m_L4BytesSent;
    m_previousL4BytesReceived = m_L4BytesReceived;
    m_previousPacketsReceived = m_PacketsReceived;
    // we don't need an overall latency => set it back to 0
    m_CummuledLatency = 0;
    m_msecPreviousStats = msecNow;

    emit statsChanged();
}
