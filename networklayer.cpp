#include <QMetaType>

#include "networklayer.h"

NetworkLayer::NetworkLayer(NetworkLayer::Layer layer)
{
    m_layer = layer;
}

QList<NetworkLayer::Layer> NetworkLayer::possibleSubLayers(NetworkLayer::Layer layer)
{
    Q_ASSERT(layer >= 0);
    Q_ASSERT(layer < LAYER_COUNT);
    return SUBLAYERS[layer];
}

/* Returns true if layer ist a possible Sublayer of this class */
bool NetworkLayer::hasPossibleSublayer(NetworkLayer::Layer layer)
{
    QList<NetworkLayer::Layer> sublayers = possibleSubLayers(m_layer);
    NetworkLayer::Layer sublayer;

    foreach(sublayer, sublayers) {
        if (layer == sublayer) {
            return true;
        }
    }

    return false;
}

NetworkLayer::Layer NetworkLayer::layer()
{
    return m_layer;
}

/* Get the LayerID from its shortname */
NetworkLayer::Layer NetworkLayer::shortname2Layer(QString shortName)
{
    int i;

    for (i = 0; i < LAYER_COUNT; i++) {
        if (shortName == m_shortNames[i]) {
            return static_cast<NetworkLayer::Layer>(i);
        }
    }

    return static_cast<NetworkLayer::Layer>(-1);
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
