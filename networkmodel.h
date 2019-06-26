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
        Layer3,
        Layer4
    };

    enum BandwidthUnit {
        bps,
        kbps,
        mbps
    };

    explicit NetworkModel(NetworkType type = NetworkType::EthernetWithoutVLAN);


    void setNetworkType(NetworkModel::NetworkType type);

    uint pduSize(uint fromSize, NetworkModel::Layer fromLayer, NetworkModel::Layer toLayer);
    qreal pps(uint fromPduSize, NetworkModel::Layer pduLayer, qreal bandwidth, NetworkModel::Layer bandwidthLayer);
    qreal bandwidth(qreal pps, uint fromPduSize, NetworkModel::Layer pduLayer, NetworkModel::Layer bandwidthLayer);
    qreal bandwidth(qreal fromBandwidth, NetworkModel::Layer fromBandwidthLayer,
                    uint fromPduSize, NetworkModel::Layer fromPduSizeLayer,
                    NetworkModel::Layer toBandwidthLayer);

    static QString layerName(NetworkModel::Layer layer);
    static QString layerShortName(NetworkModel::Layer layer);


private:
    NetworkModel::NetworkType m_networkType;
    uint L1overhead;
    uint L2overhead;
    uint L3overhead;

    uint minL2FrameLength;
    uint maxL2FrameLength;

    uint m_L1FrameSize;
    uint m_L2FrameSize;
    uint m_L3PacketSize;
    uint m_L4DatagramSize;

    NetworkModel::Layer inputBandwidthLayer = NetworkModel::Layer2;
};

#endif // NETWORKMODEL_H
