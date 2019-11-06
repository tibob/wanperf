#include "udpsenderthread.h"
#include <QDateTime>
#include <QtEndian>

/* Status for myself:
 * - One Thread per UDPSender (Rename to UDPSenderWorker).
 * - The thread is started when setting up the satellite.
 * - We create the socket in the UDPSenderWorker
 * - Communication with the main thread with signal & slots
 * - The socket will be created an maitaned in pure C, because we don't want signals & slots here. Warning: we have a portability problem.
*/

UdpSenderThread::UdpSenderThread()
{
}

bool UdpSenderThread::setTos(quint8 tos)
{
    if (isRunning()) {
        // We don't change TOS while the Thread ist runing. First stop the thread
        stop();
        m_tos = tos;
        this->start();
    } else {
        m_tos = tos;
    }

    // FIXME: we don't need this feedback
    return true;
}

bool UdpSenderThread::setDatagramSDULength(int length)
{
    if (isRunning()) {
        // We don't change SDU Length while the Thread ist running. First stop the thread
        stop();

        m_datagramSDULength = length;

        this->start();
    } else {
        m_datagramSDULength = length;
    }

    // FIXME: we don't need this feedback
    return true;
}

bool UdpSenderThread::setPpmsec(qreal ppmsec)
{
    if (isRunning()) {
        // We don't change ppmsec while the Thread ist running. First Stop the thread
        stop();

        m_ppmsec = ppmsec;

        this->start();
    } else {
        m_ppmsec = ppmsec;
    }
    return true;
}

bool UdpSenderThread::setPort(int port)
{
    if (isRunning()) {
        // We don't change the destination port while the thread is runing
        //FIXME: this should be handled by the GUI
        return false;
    }

    m_udpPort = port;
    return true;
}

bool UdpSenderThread::setDestination(QHostAddress address)
{
    if (isRunning()) {
        // We don't change the destination address while the thread is runing
        //FIXME: this should be handled by the GUI
        return false;
    }

    m_destination = address;
    return true;
}

void UdpSenderThread::stop()
{
    if (isRunning()) {
        // We don't change ppmsec while the Thread ist running. First Stop the thread
        m_Mutex.lock();
        m_stopped = true;
        m_Mutex.unlock();

        // Make sure the thread is stoped
        this->wait();
    }
}

/*
 * I want this to be very fast, so we will read the parameters once with a Mutex.
 * If the parameter are changed, the main process has to restart the thread.
 *
*/
void UdpSenderThread::run()
{
    QUdpSocket t_udpSocket;

    // NOTE: This is a workaround so that DSCP-Values are used => force IPv4
    // Qt curentliy does not support DSCP-Values in IPv6 nor when we are using QAbstractSocket::AnyIPProtocol
    // In the future: Toggle between IPv4 and IPv6 in UI
    if (!t_udpSocket.bind(QHostAddress(QHostAddress::AnyIPv4))) {
//    if (!udpSocket.bind(QHostAddress(QHostAddress::Any))) {
        qDebug() << "UdpSender::UdpSender! Could not bind the socket!";
        qDebug() << t_udpSocket.error();
        qDebug() << t_udpSocket.errorString();
        return;
    }

    qDebug() << t_udpSocket.localAddress();
    qDebug() << t_udpSocket.localPort();
    //FIXME: m_tos should be protected with a mutex, even if m_tos will never be accessed while the thread ist running
    t_udpSocket.setSocketOption(QAbstractSocket::TypeOfServiceOption, m_tos);

    qint64 t_msecNow = QDateTime::currentMSecsSinceEpoch();
    qint64 t_msecLast = t_msecNow;
    qint64 t_msecDelta = 0;
    qreal t_packets2Send = 0;

    quint64 t_sendingCounter = 0;

    qreal t_ppmsec;
    int t_datagramSDULength;
    char t_datagram[1500];

    // Variables for reading pending Datagrams
    QHostAddress t_sender;
    quint16 t_senderPort;
    quint64 t_receivedCounter;
    qint64 t_msecSend;

    quint64 t_awaitedCounter = 0;
    quint64 t_PacketsReceived = 0;
    quint64 t_PacketsLost = 0;
    quint64 t_CummuledLatency = 0;

    // Stats
    qreal t_L4BandwidthSend;   
    qreal t_L4BandwidthReceived;
    int t_ppsSent = 0;
    int t_ppsReceived = 0;
    qint64 t_statLastTime = t_msecNow;
    qint64 t_statsTimeDelta = 0;
    quint64 t_statL4BytesReceived = 0;
    quint64 t_statL4BytesSend = 0;

    qint64 datagramSize;

    qint64 latency;

    /* We access the parameter at the begining of the Thread.
     * If the parameter are changed, the caller must ensure that the tread ist restarted

      We don't need a mutex, as the variable is not changed while the thread is running
    */
    t_datagramSDULength = m_datagramSDULength;
    t_ppmsec = m_ppmsec;
    qDebug() << t_ppmsec;

    forever {
        // Stop the thread when m_stopped is set to true from the main thread.
        m_Mutex.lock();
        if (m_stopped) {
            qDebug() << "Stopping the Thread";
            m_stopped = false;
            m_Mutex.unlock();
            break;
        }
        m_Mutex.unlock();

        // First, send pending datagrams
        t_msecNow = QDateTime::currentMSecsSinceEpoch();
        t_msecDelta = t_msecNow - t_msecLast;

        t_packets2Send = t_msecDelta * t_ppmsec + t_packets2Send;

        // we never exceed the bandwidth: only send when packets2Send > 1
        while (t_packets2Send > 1) {
            // Save Timestamp
            qToBigEndian(t_msecNow, reinterpret_cast<uchar *>(t_datagram));

            // Save Counter
            qToBigEndian(t_sendingCounter, reinterpret_cast<uchar *>(t_datagram + 8));

            t_udpSocket.writeDatagram(t_datagram, t_datagramSDULength, m_destination, m_udpPort);

            t_sendingCounter++;

            t_packets2Send--;

            // Stats
            t_statL4BytesSend += t_datagramSDULength + 8; // PDU = Header (8 Bytes) + SDU;
            t_ppsSent++;
        }

        t_msecLast = t_msecNow;

        /*****************************************/
        // Then read pending datagrams
        datagramSize = t_udpSocket.pendingDatagramSize();
        while (datagramSize != -1) {
            //We just need the first 16 bytes, read timestamp & counter.
            t_udpSocket.readDatagram(t_datagram, 16, &t_sender, &t_senderPort);
            t_msecSend = qFromBigEndian<quint64>(reinterpret_cast<const uchar *>(t_datagram));
            t_receivedCounter = qFromBigEndian<quint64>(reinterpret_cast<const uchar *>(t_datagram + 8));

            // Stats calculation
            latency = t_msecNow - t_msecSend;


            // We only work wih latency < 2 sec
            if (latency >=0 && latency < 2000
                    //, the datagram Size must be the same as sending
                    && datagramSize == t_datagramSDULength
                    // and the received Counter must me smaller as the current sending counter
                    && t_receivedCounter < t_sendingCounter) {
                if (t_receivedCounter == t_awaitedCounter) {
                    // This is the awaited Paket
                    t_awaitedCounter++;
                    t_PacketsReceived++;
                    t_ppsReceived++;
                    t_CummuledLatency += latency;
                    t_statL4BytesReceived += t_datagramSDULength + 8; // PDU = Header (8 Bytes) + SDU
                } else if (t_receivedCounter > t_awaitedCounter) {
                    QString hexReceived, hexAwaited, hexSending;
                    hexReceived.setNum(t_receivedCounter, 16);
                    hexAwaited.setNum(t_awaitedCounter, 16);
                    hexSending.setNum(t_sendingCounter, 16);
                    qDebug() << "Loss at " << t_receivedCounter << "Awaited " << t_awaitedCounter << " Current" << t_sendingCounter;
                    qDebug() << "Loss at " << hexReceived << "Awaited " << hexAwaited << " Current" << hexSending;

                    // One correct packet received, but Packet loss !
                    t_PacketsLost += t_receivedCounter - t_awaitedCounter;
                    t_PacketsReceived++;
                    t_CummuledLatency += latency;
                    t_statL4BytesReceived += t_datagramSDULength + 8; // PDU = Header (8 Bytes) + SDU
                    t_awaitedCounter = t_receivedCounter + 1;

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
            datagramSize = t_udpSocket.pendingDatagramSize();
        }

        /*****************************************/
        // TODO: Emit Stats when needed (every second)
        t_statsTimeDelta = t_msecNow - t_statLastTime;
        if (t_statsTimeDelta > 1000) {
            t_L4BandwidthSend = t_statL4BytesSend * 8 / t_statsTimeDelta * 1000;
            t_L4BandwidthReceived = t_statL4BytesReceived * 8 / t_statsTimeDelta * 1000;

            // The signal will be send to the main thread, this is qt magic :-)
            emit statistics(t_L4BandwidthSend, t_L4BandwidthReceived, t_PacketsLost, t_ppsSent, t_ppsReceived);

            // Reset stat counter for next period
            t_statL4BytesSend = 0;
            t_statL4BytesReceived = 0;
            t_ppsSent = 0;
            t_ppsReceived = 0;
            t_statLastTime = t_msecNow;
        }

        /*****************************************/
        // Sleep for a while. We use a very slow sleep time as linux has very small udp recieve buffers
        // FIXME: use bigger buffers when available an inform user how to improve performance.
//        msleep(1);
        usleep(10);

    }
    // Clean Statistics when ending thread
    emit statistics(0, 0, 0, 0, 0);
}
