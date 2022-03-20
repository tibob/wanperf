#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include <QtGlobal>
#include <QString>

/*!
 * \brief The NetworkModel class is used to calculate bandwidth and PDU-Size between differen OSI-Layers
 *
 * Actually the NetworkModel class is very static an has two Networkmodells: Ethernet without VLAN and Ethernet with VLAN (dot1q).
 * In the future, the NetworkModel may become dynamic in order to allow the user to specify his own Network Model with a lot of
 * different Layers (UDP over IP over Ethernet over MPLS over GRE over IPSec over IPv4 over GRE over IPv6 over Frame Relay and so on :-) )
 *
 */

class NetworkModel
{
public:
    NetworkModel();

    /*!
     * \brief The NetworkType enum
     *
     * This enum defines the Network Type that will be used for the Model
     *
     * \value EthernetWithoutVLAN  A simple Ethernet Network without do1q VLAN tags
     * \value EthernetWithVLAN     An Ethernet Network with dot1q VLAN tags
     */
    enum NetworkType {
        EthernetWithoutVLAN,
        EthernetWithVLAN
    };

    enum Layer {
        Layer1 = 1,
        Layer2,
        Layer2noCRC,
        Layer3,
        Layer4,
    };

    enum BandwidthUnit {
        bps,
        kbps,
        mbps
    };

    uint setPduSize(uint size, NetworkModel::Layer layer);
    uint pduSize(NetworkModel::Layer layer);

    uint setBandwidth(uint newBandwidth, NetworkModel::Layer layer);
    uint bandwidth(NetworkModel::Layer layer);

    // Used to calculate the Bandwidth for Statistics
    uint pps2bandwidth(qreal pps, NetworkModel::Layer layer);

    qreal pps();

    static QString layerName(NetworkModel::Layer layer);
    static QString layerShortName(NetworkModel::Layer layer);


private:
    // Peramble = 7, Start of Delimiter = 1, Interpacket gap = 12
    uint m_L1overhead;
    // SRC = 6, DST = 6, Ethertype = 2, CRC = 4
    uint m_L2overhead;
    // SRC = 6, DST = 6, Ethertype = 2
    uint m_L2noCRCoverhead;
    // IP Header
    uint m_L3overhead;

    // Current udp Size & bandwith. These are the reference for converting into other Layers
    uint m_udpSize;
    // Bandwidth in bits per second
    uint m_udpBandwidth;
    // Current packets per second. Used to calculate the bandwidth
    // TODO: Tc should be used to calculate Bc (in packets) and from it the Bandwidth.
    // We must communicate Tc to the thread
    qreal m_pps;

    // Smalest and Biggest udp Size in order to reach smallest (64) and biggest (MTU 1500) Ethernet Frame
    uint m_minUdpSize;
    uint m_maxUdpSize;


    NetworkModel::Layer inputBandwidthLayer = NetworkModel::Layer2;
};

#endif // NETWORKMODEL_H
