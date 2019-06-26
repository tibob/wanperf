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
NetworkModel::NetworkModel(NetworkType type)
{
    setNetworkType(type);
}

void NetworkModel::setNetworkType(NetworkModel::NetworkType type)
{
    m_networkType = type;

    // Layer 1: Physical
    switch(type) {
    case EthernetWithoutVLAN:
    case EthernetWithVLAN:
        // Peramble = 7, Start of Delimiter = 1, Interpacket gap = 12
        L1overhead = 20;
        break;
    }

    // Layer 2: Data Link
    switch(type) {
    case EthernetWithoutVLAN:
        // SRC = 6, DST = 6, Ethertype = 2, CRC = 4
        L2overhead = 18;
        break;
    case EthernetWithVLAN:
        // SRC = 6, DST = 6, dot1q = 4, Ethertype = 2, CRC = 4
        L2overhead = 22;
        break;
    }

    // Layer 3: Network
    switch(type) {
    case EthernetWithoutVLAN:
    case EthernetWithVLAN:
        // IP Header = 20 Bytes
        // FIXME: and IPv6 ?
        L3overhead = 20;
        break;
    }

    // Minimal and Maximal L2 Frame Size
    switch(type) {
    case EthernetWithoutVLAN:
    case EthernetWithVLAN:
        minL2FrameLength = 64;
        maxL2FrameLength = 1500 + L2overhead;
        break;
    }
}

uint NetworkModel::pduSize(uint fromSize, NetworkModel::Layer fromLayer, NetworkModel::Layer toLayer)
{

    // First, we need to caclulate the Layer 2 Size because Layer 2 defines the min and max size of a Frame.
    uint tmpL2Size = 0;

    switch(fromLayer) {
    case NetworkModel::Layer1:
        tmpL2Size = fromSize - L1overhead;
        break;
    case NetworkModel::Layer2:
        tmpL2Size = fromSize;
        break;
    case NetworkModel::Layer3:
        tmpL2Size = fromSize + L2overhead;
        break;
    case NetworkModel::Layer4:
        tmpL2Size = fromSize + L2overhead + L3overhead;
        break;
    }

    // After padding or reducing the Frame size, we get the real Frame Size
    tmpL2Size = qMax(qMin(tmpL2Size, maxL2FrameLength), minL2FrameLength);

    switch (toLayer) {
    case NetworkModel::Layer1:
        return tmpL2Size + L1overhead;
    case NetworkModel::Layer2:
        return tmpL2Size;
    case NetworkModel::Layer3:
        return tmpL2Size - L2overhead;
    case NetworkModel::Layer4:
        return tmpL2Size - L2overhead - L3overhead;
    }

    /* This code is never reached */
    Q_ASSERT(true);
    return 0;
}


/*
 * PDUSize in Bytes
 * bandwidth in bits per second
 *
 * Returns the pps in packes per second
 */
qreal NetworkModel::pps(uint fromPduSize, NetworkModel::Layer pduLayer, qreal bandwidth, NetworkModel::Layer bandwidthLayer)
{
    uint pduSizeInTheBandwidthLayer = pduSize(fromPduSize, pduLayer, bandwidthLayer);

    if (pduSizeInTheBandwidthLayer == 0) {
        return 0;
    } else {
        return bandwidth / (pduSizeInTheBandwidthLayer * 8);
    }
}

qreal NetworkModel::bandwidth(qreal pps, uint fromPduSize, NetworkModel::Layer pduLayer, NetworkModel::Layer bandwidthLayer)
{
    uint bandwidthLayerPduSize = pduSize(fromPduSize, pduLayer, bandwidthLayer);

    return pps * bandwidthLayerPduSize * 8;
}

qreal NetworkModel::bandwidth(qreal fromBandwidth, NetworkModel::Layer fromBandwidthLayer,
                              uint fromPduSize, NetworkModel::Layer fromPduSizeLayer,
                              NetworkModel::Layer toBandwidthLayer)
{
    qreal l_pps = pps(fromPduSize, fromPduSizeLayer, fromBandwidth, fromBandwidthLayer);
    return bandwidth(l_pps, fromPduSize, fromPduSizeLayer, toBandwidthLayer);
}

QString NetworkModel::layerName(NetworkModel::Layer layer)
{
    switch(layer) {
    case NetworkModel::Layer1:
        return "Ethernet Physical Layer (L1)";
    case NetworkModel::Layer2:
        return "Ethernet Data Link Layer (L2)";
    case NetworkModel::Layer3:
        return "IP Network Layer (L3)";
    case NetworkModel::Layer4:
        return "UDP Transport Layer (L4)";
    }

    // This should never be reached
    Q_ASSERT("false");
    return "";
}

QString NetworkModel::layerShortName(NetworkModel::Layer layer)
{
    switch(layer) {
    case NetworkModel::Layer1:
        return "L1";
    case NetworkModel::Layer2:
        return "L2";
    case NetworkModel::Layer3:
        return "L3";
    case NetworkModel::Layer4:
        return "L4";
    }

    // This should never be reached
    return "";
}


