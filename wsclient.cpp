#include "wsclient.h"

#include <QJsonDocument>

/*!
 * \brief WsClient::WsClient
 * \param parent
 *
 * WsClient is used for the communication with the remote Unit
 *
 *    Client                     Remote
 *    WebService Connect  ---->  newConnection()
 *    onconnect()        <----
 *    Wen ws is connected:
 *    New Message messagetype "connect" --->
 *                            <-------  Message "connected"
 *                            <--or---  Message "error", errortype "connect", errorMessage "Message"
 *  This triggers a signal remoteConnectedForSetUp
 *    New message messagetype "newUdpSender" (ID, Port, tos) ---->  (creates udpEcho class)
 *                            <------- Message "udpEchoConnected" or "error", errortype "newUdpSender"
 *
 *   //FIXME Next is a message "deleteUdpSender" and "changeUdpSender"
 *
 *   //FIXME And we need a message "disconnect"
 *
 */

WsClient::WsClient(QObject *parent) :
    QObject(parent),
    m_statusString("Not connected")
{
    ws = new QWebSocket("Blah blah blub", QWebSocketProtocol::VersionLatest);

    connect(ws, SIGNAL(connected()),                    this, SLOT(onConnect()));
    connect(ws, SIGNAL(disconnected()),                 this, SLOT(onDisconnect()));
    connect(ws, SIGNAL(textMessageReceived(QString)),   this, SLOT(messageReceived(QString)));
}

WsClient::wsClientStatus WsClient::status()
{
    return m_status;
}

QString WsClient::statusString()
{
    return(m_statusString);
}


void WsClient::connectRemoteToSetUp(QString hostname, quint16 port)
{
    changeStatus(WsClient::wscConnectingForSetUp, "Connecting to satellite");
    connectRemote(hostname, port);
    /* The next step is the SLOT onConnect() if the WebService connection was successful */
    /* if we have an error, the Slot onDisconnect() is called */
}

void WsClient::connectRemoteToClose(QString hostname, quint16 port)
{
    changeStatus(WsClient::wscConnectingForClose, "Closing Satellite");
    connectRemote(hostname, port);
}

void WsClient::connectRemote(QString hostname, quint16 port)
{
    QString url = "ws://" + hostname + ":" + QString::number(port);
    QUrl qurl = QUrl(url);
    ws->open(qurl);

    /* The next step is the SLOT onConnect() if the WebService connection was successful */
    /* if we have an error, the Slot onDisconnect() ist called */
}

void WsClient::connectUdpEcho(QUuid id, quint16 port, quint8 tos)
{
    QJsonObject jsonMessage;
    jsonMessage["messageType"] = "newUdpEcho";
    jsonMessage["id"] = id.toString();
    jsonMessage["port"] = port;
    jsonMessage["tos"] = tos;

    QJsonDocument jsonDoc(jsonMessage);
    QString jsonString(jsonDoc.toJson());

    qDebug() << jsonString;

    ws->sendTextMessage(jsonString);

    /* The next step is the slot messageReceived, to read the answer of Remote */
}

void WsClient::removeUdpEcho(QUuid id)
{
    QJsonObject jsonMessage;
    jsonMessage["messageType"] = "deleteUdpEcho";
    jsonMessage["id"] = id.toString();

    QJsonDocument jsonDoc(jsonMessage);
    QString jsonString(jsonDoc.toJson());

    qDebug() << jsonString;

    ws->sendTextMessage(jsonString);
    /* There is no next step, we don't get an answer if the removal worked */
}

void WsClient::onConnect()
{
    if (m_status == wscConnectingForClose) {
        // We probably just wanted to clear the UdpEcho-Server on the wanperfd side.
        // As just opening a Web Socket is enougth, we can close the connection.
        changeStatus(wscRemoteDisconnected, "Not connected");
        ws->close();
        return;
    }

    if (m_status == wscConnectingForSetUp) {
        QJsonObject jsonMessage;

        jsonMessage["messageType"] = "connect";
        // NOTE: a global constant definition for the Version would be great
        jsonMessage["version"] = "wanperf 0.2";
        QJsonDocument jsonDoc(jsonMessage);
        QString jsonString(jsonDoc.toJson());

        ws->sendTextMessage(jsonString);

        /* The next step is the slot messageReceived, to read the answer of remote */
        return;
    }

    // This should never be reached
    changeStatus(wscError, "Error: unexpected connection");
    return;
}

/* This slot parses messages received from theremote wanperfd */
void WsClient::messageReceived(QString message)
{ 
    qDebug() << "WsClient::messageReceived Message received: " << message;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());

    if (jsonDoc.isNull()) {
        qDebug() << "WsClient::messageReceived: not a valid JSON Document";
        return;
    }

    QJsonObject jsonMessage = jsonDoc.object();

    if (!jsonMessage.contains("messageType")) {
        qDebug() << "WsClient::messageReceived: not a valid Message";
        return;
    }

    QString messageType = jsonMessage["messageType"].toString();

    if (messageType == "connected") {
        if (m_status == WsClient::wscConnectingForSetUp) {
            changeStatus(WsClient::wscConnectedForSetUp, "Connected. Setting the satellite up");
            emit remoteConnectedForSetUp();
        } else {
            // TODO: This should never happen, but clean the status. We should stop generating traffic
            qDebug() << "WsClient::messageReceived unexpected connection with Status" << m_status;
            changeStatus(WsClient::wscError, "Unexpected connection");
        }
    } else if (messageType == "error") {
        errorMessage(jsonMessage);
    } else if (messageType == "udpEchoConnected") {
        if (!jsonMessage.contains("id"))
            return;
        emit udpEchoConnected(QUuid(jsonMessage["id"].toString()));
    } else {
        qDebug() << "WsClient::messageReceived: Unknown Message " << jsonMessage["messageType"].toString() << jsonMessage;
    }
}

void WsClient::addUDPEcho(quint16 port, quint8 tos)
{
    QJsonObject jsonPort;

    jsonPort["port"] = port;
    jsonPort["tos"] = tos;
    jsonPorts.append(jsonPort);
}

void WsClient::allUdpEchoConnected()
{
    changeStatus(wscGeneratingTraffic, "Generating traffic");
    ws->close();
}

void WsClient::onDisconnect()
{
    qDebug() << "Master Disconnected";
    qDebug() << ws->error();
    qDebug() << ws->errorString();

    switch (m_status) {
    case WsClient::wscConnectingForSetUp:
    case WsClient::wscConnectingForClose:
        //FIXME: emit a Status change and change Status to error
        changeStatus(WsClient::wscError, QString("Error while connecting: ") + ws->errorString());
        ws->close();
        break;
    case WsClient::wscGeneratingTraffic: // Wir schließen bevor traffic produziert wird
    case WsClient::wscRemoteDisconnected: // Wir schließen den ws nachdem wir die UdpEcho-Server im Satellite geschlossen haben
        // Alles gut, wir haben den ws selbst geschlossen.
        break;
    default:
        changeStatus(WsClient::wscError, QString("Unexpected error: ") + ws->errorString());
        ws->close();
        break;
    }

}

void WsClient::changeStatus(WsClient::wsClientStatus newStatus, QString statusString)
{
    m_status = newStatus;
    m_statusString = statusString;
    emit statusChanged();
}

void WsClient::errorMessage(QJsonObject jsonMessage)
{
    qDebug() << "WsClient::errorMessage: " << jsonMessage;
}


