#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QUuid>

#include "networkmodel.h"
#include "wsclient.h"
#include "udpsenderthread.h"

class UdpSender : public QObject
{
    Q_OBJECT
public:
    explicit UdpSender(QObject *parent = nullptr);
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

    void startTraffic();
    void stopTraffic();

    /***** Statistics *****/
    qreal sendingBandwidth(NetworkModel::Layer bandwidthLayer);
    qreal receivingBandwidth(NetworkModel::Layer bandwidthLayer);
    int sendingPps();
    int receivingPps();
    int packetLost();

    /**** Communication with remote *****/
    QString connectionStatus();
    bool isConnected();
    void deleteRemoveUdpEcho();

signals:
    void connectionStatusChanged();
    void statsChanged();

public slots:
    void remoteConnectedForSetUp();
    void udpEchoConnected(QUuid id);
    void receiveStatistics(qreal L4BandwidthSend, qreal L4BandwidthReceived, quint64 packetsLost, int ppsSent, int ppsReceived);

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
    int m_datagramSDULength;
    // Packets per milisecond to send. We work with miliseconds to reduce calculation in the sending algotithm.
    qreal m_ppmsec;

    QHostAddress destination;

    UdpSenderThread m_thread;

    int m_udpPort = 4212;
    quint8 mTos = 0;

    // Unique identifier
    QUuid m_id;

    // Communication with remote
    WsClient *m_wsClient = nullptr;
    enum status {
        sRemoteDisconnected,
        sConnectingUdpEcho,
        sUdpEchoConnected,
        sSendingTraffic,
        sError
    };
    UdpSender::status m_status = UdpSender::sRemoteDisconnected;

    NetworkModel m_networkModel;

    /***** Statistics *****/
    quint64 m_PacketsLost = 0;
    qreal m_statL4BandwidthSent = 0;
    qreal m_statL4BandwidthReceived = 0;
    int m_sentPps = 0;
    int m_receivedPps = 0;

    QString m_Name;
};

#endif // UDPSENDER_H
