#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QUuid>

#include "networkmodel.h"
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

    void setBandwidth(uint bandwidth, NetworkModel::Layer bandwidthLayer);
    uint specifiedBandwidth(NetworkModel::Layer bandwidthLayer);
    void setPduSize(uint pduSize, NetworkModel::Layer pduSizeLayer);
    uint specifiedPduSize(NetworkModel::Layer pduLayer);

    void startTraffic();
    void stopTraffic();

    /***** Statistics *****/
    uint sendingBandwidth(NetworkModel::Layer bandwidthLayer);
    uint receivingBandwidth(NetworkModel::Layer bandwidthLayer);
    int sendingPps();
    int receivingPps();
    int packetLost();

signals:
    void statsChanged();

public slots:
    void receiveStatistics(quint64 packetsLost, quint64 packetsSent, quint64 packetsReceived);

private:
    QHostAddress m_destination;

    UdpSenderThread m_thread;

    int m_udpPort = 7;
    quint8 m_tos = 0;

    // Unique identifier
    QUuid m_id;

    NetworkModel m_networkModel;

    /***** Statistics *****/
    quint64 m_PacketsLost = 0;
    quint64 m_PacketsSend = 0;
    quint64 m_PacketsReceived = 0;
    qint64 m_lastStats;
    int m_sentPps = 0;
    int m_receivedPps = 0;

    QString m_Name;
};

#endif // UDPSENDER_H
