#include "udpsenderthread.h"
#include <QDateTime>
#include <QtEndian>
#include <QtGlobal>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

UdpSenderThread::UdpSenderThread()
{
}

void UdpSenderThread::setTos(quint8 tos)
{
    if (isRunning()) {
        // We don't change TOS while the Thread ist runing. First stop the thread
        stop();
        m_tos = tos;
        this->start();
    } else {
        m_tos = tos;
    }
}

void UdpSenderThread::setDatagramSDULength(int length)
{
    if (isRunning()) {
        // We don't change SDU Length while the Thread ist running. First stop the thread
        stop();

        m_datagramSDULength = length;

        this->start();
    } else {
        m_datagramSDULength = length;
    }
}

void UdpSenderThread::setPpmsec(qreal ppmsec)
{
    if (isRunning()) {
        // We don't change ppmsec while the Thread ist running. First Stop the thread
        stop();

        m_ppmsec = ppmsec;

        this->start();
    } else {
        m_ppmsec = ppmsec;
    }
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
 * All thread-internal variables are prefixed with t_
 * All shared variables (members of the object) are prefixed with m_
 *
 * This is a quite monolithic Thread. I avoided function calls in order to save
 * CPU cycles. I'm not sure this is a good idea, the code is difficult to read.
 *
 * The stats are sent with an emited signal, which is thread-safe.
 *
 * FIXME: this is Linux-Only code.
 * Hints for Sckets with Linux & Windows:
 * https://handsonnetworkprogramming.com/articles/socket-error-message-text/
 */

void UdpSenderThread::run()
{
    /********************************************************************
    * Create a Socket and bind it
    *********************************************************************/
    int t_udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (t_udpSocket < 0) {
        qDebug() << "UdpSenderThread::run: could not open the socket";
    }

    m_Mutex.lock();
    int t_result = setsockopt(t_udpSocket, IPPROTO_IP, IP_TOS,
                              (char *)&m_tos, sizeof (m_tos));
    if (t_result < 0) {
        qDebug() << "UdpSenderThread::run: could not set TOS";
        close(t_udpSocket);
        return;
    }
    m_Mutex.unlock();

    /* We do not specify from which port we send, and bind to any interface or
     * any IP on the computer
     * We bind in order to receive the response from the satellite (udpecho)
     */
    struct sockaddr_in t_myAddress;
    memset(&t_myAddress, 0, sizeof(struct sockaddr_in));
    t_myAddress.sin_family = AF_INET;
    t_myAddress.sin_addr.s_addr = INADDR_ANY;
    t_result = bind(t_udpSocket,
                    (struct sockaddr *)&t_myAddress, sizeof (t_myAddress));
    if (t_result < 0) {
        qDebug() << "UdpSenderThread::run: could not bind the socket";
        close(t_udpSocket);
        return;
    }

    /* We need the socket to be non blocking, so that we can send while no
     * packets to write and vice versa */
    t_result = fcntl(t_udpSocket, F_SETFL, O_NONBLOCK);
    if (t_result < 0) {
        qDebug() << "UdpSenderThread::run: could not set the socket non-blocking";
        close(t_udpSocket);
        return;
    }

    /********************************************************************
    * Create the address structures to reach the satellite (udpecho)
    *********************************************************************/
    struct sockaddr_in t_destAddress;
    memset(&t_destAddress, 0, sizeof(struct sockaddr_in));
    t_destAddress.sin_family = AF_INET;
    m_Mutex.lock();
    t_destAddress.sin_addr.s_addr=htonl(m_destination.toIPv4Address());
    t_destAddress.sin_port=htons(m_udpPort);
    m_Mutex.unlock();
    const socklen_t t_destAddressLen = sizeof (t_destAddress);


    /********************************************************************
    * Now initialise many variables
    *********************************************************************/
    // Current Time, ckecked frequently
    qint64 t_msecNow = QDateTime::currentMSecsSinceEpoch();
    // time differences
    qint64 t_msecDelta = 0;


    // Tc (Time Commited): Time interval in which to send the packets
    const qint64 t_msecTc = 100;
    // Bc (Burst Commited): Packets to send per Time interval
    m_Mutex.lock();
    qint64 t_packetsBc = t_msecTc * m_ppmsec;
    m_Mutex.unlock();

    /* keep track how much packets we have to send */
    int t_packetBucket = t_packetsBc;

    // used to check the response from recv() and sendto()
    ssize_t t_packetSize;

    /* keep track when to refill the bucket */
    qint64 t_msecNextRefill = t_msecNow + t_msecTc;

    m_Mutex.lock();
    const int t_datagramSDULength = m_datagramSDULength;
    m_Mutex.unlock();

    /* Our Datagram payloads for sending and receiving */
    char t_datagramSend[t_datagramSDULength];
    memset(t_datagramSend, 0, t_datagramSDULength);

    char t_datagramReceive[t_datagramSDULength];
    memset(t_datagramReceive, 0, t_datagramSDULength);

    /* We keep track of the count of packets sended in order to detect packet loss
     * We store the t_sendingCounter directly into the datagram to be send 7 received,
     * so we do not have to copy them to/from there.
    */
    quint64 *t_sendingCounter = reinterpret_cast<quint64 *>(t_datagramSend + 8);
    quint64 *t_returnedCounter = reinterpret_cast<quint64 *>(t_datagramReceive + 8);

    // Packet counter we are waitung for
    quint64 t_counterAwaited = 0;
    // delta between awaited counter and received counter
    int t_counterDelta;

    const int t_chunkSize = 10;
    int t_chunkCounter;

    // Timestamp in the returned UDP datagram
    qint64 t_msecReturned;


    // Stats
    int t_statsPacketsReceived = 0;
    int t_statsPacketsLost = 0;
    quint64 t_statsCummuledLatency = 0;


    const qint64 t_statsReportInterval = 1000;
    // Report stats before next Tc
    qint64 t_statNextTime = t_msecNow + t_statsReportInterval - 5;

    qint64 t_latency;

    /*****************************************************************
     * with poll we can check if the socket can be read or written to
     * We need two different structures: one only for reading and one for reading and writing.
     */
    struct pollfd t_pollRead;
    t_pollRead.fd = t_udpSocket;
    t_pollRead.events = POLLIN;

    struct pollfd t_pollReadWrite;
    t_pollReadWrite.fd = t_udpSocket;
    t_pollReadWrite.events = POLLIN|POLLOUT;


    /********************************************************************
    * This is our thread loop. It last forever and will be broken when m_stoped ist set to true.
    * TODO: Check if we can just kill the thread insted of setting m_stopped to true?
    *********************************************************************/
    forever {
        // Stop the thread when m_stopped is set to true from the main thread.
        // FIXME: is bool atomic? Ca we use no mutex here?
        m_Mutex.lock();
        if (m_stopped) {
            m_stopped = false;
            m_Mutex.unlock();
            /* Break outside the forever loop */
            break;
        }
        m_Mutex.unlock();

        /********************************************************************
        * First step: receive as much packets as possible, in order to clear the buffers.
        * We begin with receiving becaue we do not want packet drops comming from overfull receiving buffers.
        * With the chunk, we alternate sending and reciving. But we give reciving a bigger chunk, because we do
        * not want to have packet loss because of to small buffers.
        * As soon as there is nothing to receive, recv will return -1 and we go on to second step.
        ********************************************************************/
        t_chunkCounter = 5 * t_chunkSize;
        while (t_chunkCounter > 0) {
            t_packetSize = recv(t_udpSocket, &t_datagramReceive, t_datagramSDULength, 0);
            if (t_packetSize < 0) {
                /* Error or buffers full (EAGAIN or EWOULDBLOCK) => stop
                 * sending for now and go to next step */
                break;
            }
            t_msecReturned = qFromBigEndian<quint64>(reinterpret_cast<const uchar *>(&t_datagramReceive));

            t_msecNow = QDateTime::currentMSecsSinceEpoch();
            t_latency = t_msecNow - t_msecReturned;

            t_counterDelta = *t_returnedCounter - t_counterAwaited;
            if (t_counterDelta == 0) {
                // This is the awaited Paket
                t_counterAwaited++;
                t_statsPacketsReceived++;
                t_statsCummuledLatency += t_latency;
            } else if (t_counterDelta > 0) {
                // One packet was received, but packets inbetween have been lost
                t_statsPacketsLost = t_statsPacketsLost + (t_counterDelta);
                t_statsPacketsReceived++;
                t_statsCummuledLatency += t_latency;
                t_counterAwaited = *t_returnedCounter + 1;
            } else {
                // We received a counter which is smaller as the awaited counter
                // We have a Packet duplication or a reordered packet => we ignore it
                qDebug("Dup or reordered!");
            }
            t_chunkCounter--;
        }

        /********************************************************************
        * Second step: emit stats (once per second)
        * We do this befor we send new packets and hope to get better stats
        * The problem is that the stats vary in time
        *********************************************************************/
        t_msecNow = QDateTime::currentMSecsSinceEpoch();
        if (t_statNextTime < t_msecNow) {

            // The signal will be send to the main thread, this is qt magic and is thread-safe :-)
            // FIXME: Latency not sended
            emit statistics(t_statsPacketsLost, *t_sendingCounter, t_statsPacketsReceived);

            // Reset stat counter for next period.
            // TODO: - when the stats a in synch with Tc, they slightly change in time
//            t_statNextTime = t_msecNow + t_statsReportInterval;
            t_statNextTime += t_statsReportInterval;
        }

        /********************************************************************
        * Third step: send as much packets as possible
        * If the buffers are full, we get a negative result from sendto
        *********************************************************************/
        t_msecNow = QDateTime::currentMSecsSinceEpoch();
        // Do we need to refill our Bucket?
        if (t_msecNextRefill <= t_msecNow) {
            t_msecNextRefill += t_msecTc;
            t_packetBucket = t_packetsBc;
        }

        t_chunkCounter = qMin(t_packetBucket, t_chunkSize);
        while (t_chunkCounter > 0) {
            t_msecNow = QDateTime::currentMSecsSinceEpoch();
            qToBigEndian(t_msecNow, reinterpret_cast<uchar *>(t_datagramSend));
            t_packetSize = sendto(t_udpSocket, &t_datagramSend, t_datagramSDULength, 0,
                                 (struct sockaddr *)&t_destAddress, t_destAddressLen);
            if (t_packetSize < 0) {
                /* Error or buffers full (EAGAIN or EWOULDBLOCK) => stop sending for now */
                break;
            }
            *t_sendingCounter = *t_sendingCounter + 1;
            // NOTE: Maybe optimisation here (only decerease packetBucker an run the while against a limit)
            t_packetBucket--;
            t_chunkCounter--;
        }




        /*****************************************/
        // Sleep for a while. We use a very slow sleep time as linux has very small udp recieve buffers
        // FIXME: use bigger buffers when available an inform user how to improve performance.
//        msleep(1);

        // FIXME - Listen to the socket until a packet is received, I need to send an a packet ca be send or timeout (next Tc-Time)

        t_msecNow = QDateTime::currentMSecsSinceEpoch();
        // wait until we refill our bucket or we have to send stats
        if (t_msecNextRefill < t_statNextTime) {
            t_msecDelta = t_msecNextRefill - t_msecNow;
        } else {
            t_msecDelta = t_statNextTime - t_msecNow;
        }
        if (t_msecDelta > 0) {
            if (t_packetBucket >0) {
                // we still have something to send
                poll(&t_pollReadWrite, 1, t_msecDelta);
            } else {
                // we just wait for udp echos
                poll(&t_pollRead, 1, t_msecDelta);
            }
        }
    }

    // Ending the thread - close socket
    close(t_udpSocket);
}
