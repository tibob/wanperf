#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QUuid>
#include "networkmodel.h"
#include "wsclient.h"

class UdpSender : public QObject
{
    Q_OBJECT
public:
    explicit UdpSender(QObject *parent = 0);
//    Packet Length
    void setNetworkModel(NetworkModel model);
    NetworkModel networkModel();
//    Destination Host - should this be part of the constructor?
    void setDestination(QHostAddress address);
    void setPort(int udpPort);
    int port();
    void setTos(quint8 tos=0);
    void setDscp(quint8 dscp=0);
    quint8 dscp();
    void setName ( QString newName);
    QString name();
    QUuid id();
    void setWsClient(WsClient *wsClient);

    void setBandwidth(qreal bandwidth, NetworkModel::Layer bandwidthLayer);
    qreal specifiedBandwidth(NetworkModel::Layer bandwidthLayer);
    void setPduSize(uint pduSize, NetworkModel::Layer pduSizeLayer);
    uint specifiedPduSize(NetworkModel::Layer pduLayer);

    void sendOneDatagram();
    void startTraffic();
    void stopTraffic();

    /***** Statistics *****/
    qreal sendingRate(NetworkModel::Layer bandwidthLayer);
    qreal receivingRate(NetworkModel::Layer bandwidthLayer);
    int packetLost();

    /**** Communication with remote *****/
    QString connectionStatus();
    bool isConnected();
    void deleteRemoveUdpEcho();

signals:
    void connectionStatusChanged();
    void statsChanged();

public slots:
    void sendUdpDatagrams();
    void readPendingDatagrams();
    void remoteConnectedForSetUp();
    void udpEchoConnected(QUuid id);
    void calculateStatistics();

private:
    // We have to keep track what parameters have been set for the udpSender
    // Bandwidth in bits per second
    qreal m_bandwidth;
    NetworkModel::Layer m_bandwidthLayer;
    // PDU Size in Bytes
    uint m_pduSize;
    NetworkModel::Layer m_pduSizeLayer;

    // These values are calculated from the parameters bandwidth and pduSize
    // Default values are set do avoid a division by zero somewhere
    // This is the Payoad of udp without header.
    int datagramSDULength;
    // Packets per milisecond to send. We work with miliseconds to reduce calculation in the sending algotithm.
    qreal m_ppmsec;

    char *datagram = NULL;
    quint64 m_sendingCounter = 0;
    quint64 m_awaitedCounter = 0;
    QHostAddress destination;
    QUdpSocket udpSocket;
    int m_udpPort = 4212;
    quint8 mTos = 0;

    // Unique identifier
    QUuid m_id;

    // Communication with remote
    WsClient *m_wsClient = NULL;
    enum status {
        sRemoteDisconnected,
        sConnectingUdpEcho,
        sUdpEchoConnected,
        sSendingTraffic,
        sError
    };
    UdpSender::status m_status = UdpSender::sRemoteDisconnected;

    /* Variables for running a Test */
    // Time (msec) when last time sendUdpDatagrams was called
    qint64 msecLast;
    QTimer *timer = NULL;
    // Resolution of sending timer in msec
    int timerResolution = 10;
    // how much "subpackets" (part of a packet) could not be send and will be send next timer call?
    qreal packets2Send = 0;

    NetworkModel m_networkModel;

    /***** Statistics *****/
//    // Statistics: store timestamp & L4 SDU (Payload without UDP header) Bytes sent
//    QVector<qint64> listTimestamp;
//    QVector<int>    listL4BytesSend;


    // Statistics: delay between calculations in miliseconds.
    int statsCalculationDelay = 1000;
    // Time of current Statistics
    qint64 m_msecPreviousStats;
    // Statistics: actual value and value from the last calculation
    quint64 m_L4BytesSent = 0;
    quint64 m_previousL4BytesSent = 0;
    quint64 m_L4BytesReceived = 0;
    quint64 m_previousL4BytesReceived = 0;
    // PacketsSent = m_sendingCounter
    quint64 m_PacketsReceived = 0;
    quint64 m_previousPacketsReceived = 0;
    quint64 m_PacketsLost = 0;
    quint64 m_CummuledLatency = 0;
    quint64 m_previousCummuledLatency = 0;
    // Statistical results
    qreal m_statL4BandwidthSent = 0;
    qreal m_statL4BandwidthReceived = 0;
    qreal m_statAverageLatency = 0;
    // NOTE: one could wand packet lost per second
    // qreak m_statPacketLostPerSecond;

    // Time where the Class is created, in sec to epoch
    uint m_referenceDate;

    QString m_Name;

};

#endif // UDPSENDER_H
