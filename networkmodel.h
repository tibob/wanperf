#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include <QtGlobal>
#include <QString>
#include <QStringList>

/*!
 * \brief The NetworkModel class is used to calculate bandwidth and PDU-Size between differen OSI-Layers
 *
 * FIXME: This is used by the LAN Model an will be replaced by networklayerlistmodel in the future
 *
 * Actually the NetworkModel class is very static.
 * In the future, the NetworkModel may become dynamic in order to allow the user to specify his own Network Model with a lot of
 * different Layers (Example: UDP over IP over Ethernet over MPLS over GRE over IPSec over IPv4 over Ethernet)
 *
 */

class NetworkModel
{
public:
    NetworkModel();

    enum Layer {
        EthernetLayer1 = 0,
        EthernetLayer2,
        EthernetLayer2woCRC,
        IPLayer,
        UDPLayer
    };

    enum BandwidthUnit {
        bps,
        kbps,
        mbps
    };

    void setPduSize(uint size, NetworkModel::Layer layer);
    uint pduSize(NetworkModel::Layer layer);

    void setBandwidth(uint newBandwidth, NetworkModel::Layer layer);
    uint bandwidth(NetworkModel::Layer layer);

    // Used to calculate the Bandwidth for Statistics
    uint pps2bandwidth(qreal pps, NetworkModel::Layer layer);

    qreal pps();

    static QString layerName(NetworkModel::Layer layer);
    static QString layerShortName(NetworkModel::Layer layer);

private:
    // We need to declare the static variables als consexpr because we use it in qMin wich passes its arguments as
    // a reference. C++17 makes an inline variable of it, so we don't get an error at compilation time

    // Peramble = 7, Start of Delimiter = 1, Interpacket gap = 12
    static constexpr uint m_L1overhead = 20;
    // SRC = 6, DST = 6, Ethertype = 2, CRC = 4
    static constexpr uint m_L2overhead = 18;
    // SRC = 6, DST = 6, Ethertype = 2
    static constexpr uint m_L2noCRCoverhead = 14;
    // IP Header
    static constexpr uint m_L3overhead = 20;

    // Current udp Size & bandwith. These are the reference for converting into other Layers
    uint m_udpSize;

    // Specified bandwidth in bits per second. We must store the specified layer to, in order to keep the
    // bandwidth at PDU Size changes
    uint m_udpBandwidth;
    uint m_bandwidth;
    NetworkModel::Layer m_bandwidthLayer;

    // Current packets per second. Used to calculate the bandwidth
    // NOTE: Tc could be used to calculate Bc (in packets) and from it the bandwidth that realy will be sent.
    qreal m_pps;

    // Smalest and biggest UDP size in order to reach smallest (IP PDU-Lengeht 64) and biggest (IP MTU 1500)
    // Ethernet Frame

    // Minimal Ethernet length = 64 bytes, minus ethernet header, minus IP header (20 Bytes)
    static constexpr uint m_minUdpSize = 64 - 18 - 20;
    // IP MTU minus IP header
    static constexpr uint m_maxUdpSize = 1500 - 20;
};

#endif // NETWORKMODEL_H
