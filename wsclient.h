#ifndef WSCLIENT_H
#define WSCLIENT_H

#include <QObject>
#include "QtWebSockets/QWebSocket"
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>


class WsClient : public QObject
{
    Q_OBJECT
public:
    enum wsClientStatus {
        wscRemoteDisconnected,
        wscConnectingForSetUp,
        wscConnectedForSetUp,
        wscGeneratingTraffic,
        wscConnectingForClose,
        wscConnectedForClose,
        wscError
    };

    explicit WsClient(QObject *parent = nullptr);
    WsClient::wsClientStatus status();
    QString statusString();
    void removeUdpEcho(QUuid id);

signals:
    void remoteConnectedForSetUp();
    void udpEchoConnected(QUuid id);
    void statusChanged();

public slots:
    void connectRemoteToSetUp(QString hostname, quint16 port = 4212);
    void connectRemoteToClose(QString hostname, quint16 port = 4212);
    void connectUdpEcho(QUuid id, quint16 port, quint8 tos=0);
    void messageReceived(QString message);
    void addUDPEcho(quint16 port, quint8 tos=0);
    void allUdpEchoConnected();

private slots:
    void onConnect();
    void onDisconnect();
    void changeStatus(WsClient::wsClientStatus newStatus, QString statusString);

private:
    QWebSocket *ws;

    QJsonArray jsonPorts;



    WsClient::wsClientStatus m_status = WsClient::wscRemoteDisconnected;
    QString m_statusString;

    void errorMessage(QJsonObject jsonMessage);
    void connectRemote(QString hostname, quint16 port);
};

#endif // WSCLIENT_H
