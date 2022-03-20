#ifndef UDPSENDERTHREAD_H
#define UDPSENDERTHREAD_H

#include <QThread>
#include <QReadWriteLock>
#include <QMutex>
#include <QUdpSocket>

class UdpSenderThread : public QThread
{
    Q_OBJECT

public:
    UdpSenderThread();

    void setTos(quint8 tos);
    void setDatagramSDULength(int length);
    void setPpmsec(qreal ppmsec);
    bool setPort(int port);
    bool setDestination(QHostAddress address);
    void stop();

signals:
    void statistics(quint64 packetsLost, quint64 packetsSent, quint64 packetsReceived);

protected:
    void run() Q_DECL_OVERRIDE;


private:
    /* Parameter for run(). Has not to be accessed via a Mutex as the parameter are only changed when thread ist not running */
    /* Defaults are set to avoid a random value */

    // This is the Payoad of udp without header.
    int m_datagramSDULength = 500;
    // Packets per milisecond to send. We work with miliseconds to reduce calculation in the sending algotithm.
    qreal m_ppmsec = 0;
    // Destination Port
    quint16 m_udpPort = 7;
    // Destination Address
    QHostAddress m_destination;
    // Type of Service
    quint8 m_tos = 0;

    /* Locker when accessing Parameter and Statistics */
    QMutex m_Mutex;
    // volatile = tell the computer this variable may change at any time
    volatile bool m_stopped = false;

};

#endif // UDPSENDERTHREAD_H
