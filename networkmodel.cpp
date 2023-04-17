#include <QMetaType>
#include <QDebug>

#include "networkmodel.h"

/*!
 * \brief NetworkModel::NetworkModel
 * \param type
 *
 * Constructs a NetworkModel. The default value dor \a type is NetworkModel::EthernetWithoutVLAN
 *
 * FIXME: This is used by the LAN Model an will be replaced by networklayerlistmodel in the future
 *
 */
NetworkModel::NetworkModel()
{
    // Initialize to something
    m_udpSize = 1000;
    m_bandwidth = 100;
    m_bandwidthLayer = NetworkModel::EthernetLayer2;
}

void NetworkModel::setPduSize(uint size, NetworkModel::Layer layer)
{
    // Assign a value to tmp_size in order to avoid a warning.
    int tmp_size = size;

    switch(layer) {
    case NetworkModel::EthernetLayer1:
        tmp_size = size - m_L1overhead - m_L2overhead - m_L3overhead;
        break;
    case NetworkModel::EthernetLayer2:
        tmp_size = size              - m_L2overhead - m_L3overhead;
        break;
    case NetworkModel::EthernetLayer2woCRC:
        tmp_size = size         - m_L2noCRCoverhead - m_L3overhead;
        break;
    case NetworkModel::IPLayer:
        tmp_size = size                             - m_L3overhead;
        break;
    case NetworkModel::UDPLayer:
        tmp_size = size;
        break;
    default:
        /* we do not set the PDU Size for these Layers */
        return;
    }

    if (tmp_size < 0) {
        m_udpSize = m_minUdpSize;
    } else {
        m_udpSize = tmp_size;
    }

    m_udpSize = qMax(qMin(m_udpSize, m_maxUdpSize), m_minUdpSize);

    // recalculate pps. We use the specified bandwidth in its specified layer
    m_pps = (qreal) m_bandwidth / (pduSize(m_bandwidthLayer) * 8);
}

uint NetworkModel::pduSize(NetworkModel::Layer layer)
{
    switch (layer) {
    case NetworkModel::EthernetLayer1:
        return m_udpSize + m_L3overhead + m_L2overhead + m_L1overhead;
    case NetworkModel::EthernetLayer2:
        return m_udpSize + m_L3overhead + m_L2overhead;
    case NetworkModel::EthernetLayer2woCRC:
        return m_udpSize + m_L3overhead + m_L2noCRCoverhead;
    case NetworkModel::IPLayer:
        return m_udpSize + m_L3overhead;
    case NetworkModel::UDPLayer:
        return m_udpSize;
    default:
        // we do calculate the PDU Size for other Layers
        return m_udpSize;
    }

    /* This code is never reached but the compiler wants it*/
    Q_ASSERT(true);

    return m_udpSize;
}

void NetworkModel::setBandwidth(uint newBandwidth, NetworkModel::Layer layer)
{
    /* We never set m_udpSize to 0 */
    Q_ASSERT(m_udpSize != 0);

    m_pps = (qreal) newBandwidth / (pduSize(layer) * 8);
    // we round to the nearest integer. If we would not use qRound, it would round down
    m_bandwidth = newBandwidth;
    m_bandwidthLayer = layer;

    return;
}

uint NetworkModel::bandwidth(NetworkModel::Layer layer)
{
    // we round to the nearest integer. If we would not use qRound, it would round down
    return qRound(m_pps * pduSize(layer) * 8);
}

uint NetworkModel::pps2bandwidth(qreal pps, NetworkModel::Layer layer)
{
    return pps * pduSize(layer) * 8;
}

qreal NetworkModel::pps()
{
    return m_pps;
}

QString NetworkModel::layerName(NetworkModel::Layer layer)
{
    switch(layer) {
    case NetworkModel::EthernetLayer1:
        return "Ethernet Physical Layer (L1)";
    case NetworkModel::EthernetLayer2:
        return "Ethernet Data Link Layer (L2)";
    case NetworkModel::EthernetLayer2woCRC:
        return "Ethernet Data Link Layer without CRC Filed (L2)";
    case NetworkModel::IPLayer:
        return "IP Network Layer (L3)";
    case NetworkModel::UDPLayer:
        return "UDP Transport Layer (L4)";
    default:
        return "FIXME";
    }

    // This should never be reached
    Q_ASSERT(false);
    return "";
}

QString NetworkModel::layerShortName(NetworkModel::Layer layer)
{
    QString shortname;
    switch(layer) {
    case EthernetLayer1:
        shortname = "EthernetL1";
        break;
    case EthernetLayer2:
        shortname = "EthernetL2";
        break;
    case EthernetLayer2woCRC:
        shortname = "L2noCRC";
        break;
    case IPLayer:
        shortname = "IP";
        break;
    case UDPLayer:
        shortname = "UDP";
        break;
    }

    return shortname;
}

