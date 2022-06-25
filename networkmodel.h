#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include <QtGlobal>
#include <QString>

/*!
 * \brief The NetworkModel class is used to calculate bandwidth and PDU-Size between differen OSI-Layers
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
        EthernetLayer1 = 1,
        EthernetLayer2,
        EthernetLayer2woCRC,
        CRCforEthernetLayer2,
        IPLayer,
        UDPLayer,
        GRELayer,
        GRELayerWithKey,
        DMVPNLayer,
        IPSecLayer
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

    QList<Layer> subLayers(NetworkModel::Layer layer);
    QList<Layer> WANLayers();

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

    // Specified bandwidth in bits per second. We must store the specified layer to, in order to keep the
    // bandwidth at PDU Size changes
    uint m_udpBandwidth;
    uint m_bandwidth;
    NetworkModel::Layer m_bandwidthLayer;

    // Current packets per second. Used to calculate the bandwidth
    // NOTE: Tc could be used to calculate Bc (in packets) and from it the bandwidth that realy will be sent.
    qreal m_pps;

    // Smalest and biggest UDP size in order to reach smallest (IP PDU-Lengeht 64) and biggest (IP MTU 1500) Ethernet Frame
    uint m_minUdpSize;
    uint m_maxUdpSize;


    NetworkModel::Layer inputBandwidthLayer = NetworkModel::EthernetLayer2;
};

#endif // NETWORKMODEL_H
