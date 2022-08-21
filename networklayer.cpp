#include <QMetaType>

#include "networklayer.h"

NetworkLayer::NetworkLayer(NetworkLayer::Layer layer)
{
    m_layer = layer;
}

NetworkLayer::~NetworkLayer()
{
    // we do not wand dead pointers on the layers around.
    if (m_lowerLayer)
        m_lowerLayer->removeHigherLayer();
    // this should never occur as we cannot remove a layer which is currently encapsulated
    Q_ASSERT(m_higherLayer == NULL);
    if (m_higherLayer)
        m_higherLayer->removeLowerLayer();
}

QList<NetworkLayer::Layer> NetworkLayer::possibleSubLayers(NetworkLayer::Layer layer)
{
    QList<NetworkLayer::Layer> list;
    switch (layer) {
    case NetworkLayer::UDP:
        list.append(NetworkLayer::IP);
        break;
    case NetworkLayer::IP:
        list.append(NetworkLayer::EthernetL2);
        list.append(NetworkLayer::EthernetL2woCRC);
        list.append(NetworkLayer::GRE);
        list.append(NetworkLayer::GREWithKey);
        list.append(NetworkLayer::ESP_AES256_SHA_TUN);
        break;
    case NetworkLayer::EthernetL2woCRC:
        list.append(NetworkLayer::EthernetCRC);
        break;
    case NetworkLayer::EthernetL2:
    case NetworkLayer::EthernetCRC:
        list.append(EthernetL1);
        break;
    case NetworkLayer::EthernetL1:
        /* no sublayer */
        break;
    case NetworkLayer::GRE:
    case NetworkLayer::GREWithKey:
    case NetworkLayer::ESP_AES256_SHA_TUN:
        list.append(NetworkLayer::IP);
        break;
    default:
        break;
    }
    return list;
}

void NetworkLayer::setLowerLayer(NetworkLayer *networkLayer)
{
    if (m_lowerLayer == networkLayer) {
        // Lower layer already set
        return;
    }

    m_lowerLayer = networkLayer;
    networkLayer->setHigherLayer(this);
}

void NetworkLayer::setHigherLayer(NetworkLayer *networkLayer)
{
    if (m_higherLayer == networkLayer) {
        // Higher layer already set
        return ;
    }

    m_higherLayer = networkLayer;
    networkLayer->setLowerLayer(this);
}

void NetworkLayer::removeLowerLayer()
{
    m_lowerLayer = NULL;
}

void NetworkLayer::removeHigherLayer()
{
    m_higherLayer = NULL;
}

NetworkLayer::Layer NetworkLayer::layer()
{
    return m_layer;
}

uint NetworkLayer::setPDUSize(uint size)
{
    int tmpSDUSize;
    m_PDUSize = size;

    // Minimal PDU from Protocol
    m_PDUSize = qMax(m_PDUSize, m_minPDUsize[m_layer]);

    // Maximal PDU to avoid fragmentation
    m_PDUSize = qMin(m_PDUSize, m_maxPDUsize[m_layer]);

    switch (m_layer) {
    case NetworkLayer::ESP_AES256_SHA_TUN:
        // calculate maximal SDU Size (without padding)
        tmpSDUSize = m_PDUSize - m_overhead[m_layer];
        // if the Padding allows it, use the original SDU size
        if (tmpSDUSize - m_ESPSDUSize < 16) {
            m_SDUSize = m_ESPSDUSize;
        } else {
            m_SDUSize = tmpSDUSize;
        }
        break;
    default:
        m_SDUSize = m_PDUSize - m_overhead[m_layer];
        break;
    }

    return m_PDUSize;
}

uint NetworkLayer::setSDUSize(uint size)
{
    int paddingSize;
    int tmpSDUSize;

    switch (m_layer) {
    case NetworkLayer::ESP_AES256_SHA_TUN:
        // AES256 uses 16-Bytes blocks, so add padding accordingly
        // padding includes next header and payload length fields (1 + 1  = 2 bytes)
        paddingSize = (size + 2) % 16;
        if (paddingSize != 0) {
            paddingSize = 16 - paddingSize;
        }
        // adding padsing, SPI(4) + seq (4) + ESP Initialisatin Vector (16) +
        // payload length (1) + next Header (1) + ICV SHA-HMAC (12)
        m_PDUSize = size + paddingSize + 38;
        // Keep the SDU Size, so we can try to resore it
        m_ESPSDUSize = size;
        break;
    default:
        m_PDUSize = size + m_overhead[m_layer];
        break;
    }

    // Minimal PDU from Protocol
    m_PDUSize = qMax(m_PDUSize, m_minPDUsize[m_layer]);

    // Maximal PDU to avoid fragmentation
    m_PDUSize = qMin(m_PDUSize, m_maxPDUsize[m_layer]);

    // Now recalculate the SDU Size after the PDU Size has been adjusted
    switch (m_layer) {
    case NetworkLayer::ESP_AES256_SHA_TUN:
        // calculate maximal SDU Size (without padding)
        tmpSDUSize = m_PDUSize - m_overhead[m_layer];
        // if the Padding allows it, use the original SDU size
        if (tmpSDUSize - m_ESPSDUSize < 16) {
            m_SDUSize = m_ESPSDUSize;
        } else {
            m_SDUSize = tmpSDUSize;
        }
        break;
    default:
        m_SDUSize = m_PDUSize - m_overhead[m_layer];
        break;
    }

    return m_SDUSize;
}

uint NetworkLayer::PDUSize()
{
    return m_PDUSize;
}

uint NetworkLayer::SDUSize()
{
    return m_SDUSize;
}

QString NetworkLayer::layerName()
{
    return m_longNames[m_layer];
}

QString NetworkLayer::layerShortName()
{
    return m_shortNames[m_layer];
}
