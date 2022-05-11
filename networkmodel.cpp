#include <QMetaType>
#include <QDebug>

#include "networkmodel.h"

/*!
 * \brief NetworkModel::NetworkModel
 * \param type
 *
 * Constructs a NetworkModel. The default value dor \a type is NetworkModel::EthernetWithoutVLAN
 *
 */
NetworkModel::NetworkModel()
{
    // FIXME: this should be (static) const, but I need a good cast for qMin/qMax and it is not my current prioriy
    //        to find a good cast ;-)
    // Peramble = 7, Start of Delimiter = 1, Interpacket gap = 12
    m_L1overhead = 20;
    // SRC = 6, DST = 6, Ethertype = 2, CRC = 4
    m_L2overhead = 18;
    // SRC = 6, DST = 6, Ethertype = 2
    m_L2noCRCoverhead = 14;
    // IP Header
    m_L3overhead = 20;

    // Minimal Ethethernet length = 64 bytes, minus ethernet header, minus IP header (20 Bytes)
    m_minUdpSize = 64 - 18 - 20;
    // IP MTU minus IP header
    m_maxUdpSize = 1500 - 20;

    // Initialize to something
    m_udpSize = 1000;
    m_bandwidth = 100;
    m_bandwidthLayer = NetworkModel::Layer2;
}

uint NetworkModel::setPduSize(uint size, NetworkModel::Layer layer)
{
    // Assign a value to tmp_size in order to avoid a warning.
    int tmp_size = size;

    switch(layer) {
    case NetworkModel::Layer1:
        tmp_size = size - m_L1overhead - m_L2overhead - m_L3overhead;
        break;
    case NetworkModel::Layer2:
        tmp_size = size              - m_L2overhead - m_L3overhead;
        break;
    case NetworkModel::Layer2noCRC:
        tmp_size = size         - m_L2noCRCoverhead - m_L3overhead;
        break;
    case NetworkModel::Layer3:
        tmp_size = size                             - m_L3overhead;
        break;
    case NetworkModel::Layer4:
        tmp_size = size;
        break;
    }

    if (tmp_size < 0) {
        m_udpSize = m_minUdpSize;
    } else {
        m_udpSize = tmp_size;
    }

    m_udpSize = qMax(qMin(m_udpSize, m_maxUdpSize), m_minUdpSize);

    // recalculate pps. We use the specified bandwidth in its specified layer
    m_pps = (qreal) m_bandwidth / (pduSize(m_bandwidthLayer) * 8);

    return pduSize(layer);
}

uint NetworkModel::pduSize(NetworkModel::Layer layer)
{
    switch (layer) {
    case NetworkModel::Layer1:
        return m_udpSize + m_L3overhead + m_L2overhead + m_L1overhead;
    case NetworkModel::Layer2:
        return m_udpSize + m_L3overhead + m_L2overhead;
    case NetworkModel::Layer2noCRC:
        return m_udpSize + m_L3overhead + m_L2noCRCoverhead;
    case NetworkModel::Layer3:
        return m_udpSize + m_L3overhead;
    case NetworkModel::Layer4:
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
    case NetworkModel::Layer1:
        return "Ethernet Physical Layer (L1)";
    case NetworkModel::Layer2:
        return "Ethernet Data Link Layer (L2)";
    case NetworkModel::Layer2noCRC:
        return "Ethernet Data Link Layer without CRC Filed (L2)";
    case NetworkModel::Layer3:
        return "IP Network Layer (L3)";
    case NetworkModel::Layer4:
        return "UDP Transport Layer (L4)";
    }

    // This should never be reached
    Q_ASSERT(false);
    return "";
}

QString NetworkModel::layerShortName(NetworkModel::Layer layer)
{
    switch(layer) {
    case NetworkModel::Layer1:
        return "L1";
    case NetworkModel::Layer2:
        return "L2";
    case NetworkModel::Layer2noCRC:
        return "L2noCRC";
    case NetworkModel::Layer3:
        return "L3";
    case NetworkModel::Layer4:
        return "L4";
    }

    // This should never be reached
    return "";
}


