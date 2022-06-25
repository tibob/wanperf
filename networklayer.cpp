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
        m_higherLayer->removeLoweLayer();
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
        list.append(NetworkLayer::IPSec);
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
    case NetworkLayer::IPSec:
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

void NetworkLayer::removeLoweLayer()
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
    uint layerPDUSize;
    uint layerSDUSize;
    uint higherLayerPDUSize;

    layerPDUSize = size;

    // Minimal PDU from Protocol
    layerPDUSize = qMax(layerPDUSize, m_minPDUsize[m_layer]);

    // Maximal PDU to avoid fragmentation
    layerPDUSize = qMin(layerPDUSize, m_maxPDUsize[m_layer]);


    if (m_higherLayer) {
        // If we have an higer layer (that we encapsulate), our SDU is its PDU, so we calculate it
        switch (m_layer) {
        case NetworkLayer::IPSec:
            // FIXME: calculate padding accurately
            layerSDUSize = layerPDUSize - m_overhead[m_layer];
            break;
        default:
            layerSDUSize = layerPDUSize - m_overhead[m_layer];
            break;
        }

        // Wen we set the PDU of the higher Layer, it may adjust it, so we need to recalculate our PDU Size
        higherLayerPDUSize = m_higherLayer->setPDUSize(layerSDUSize);
        switch (m_layer) {
        case NetworkLayer::IPSec:
            // FIXME: calculate padding accurately
            layerPDUSize =  higherLayerPDUSize + m_overhead[m_layer];
            break;
        default:
            layerPDUSize =  higherLayerPDUSize + m_overhead[m_layer];
            break;
        }
    }

    return layerPDUSize;
}

uint NetworkLayer::PDUSize()
{
    return m_PDUSize;
}

QString NetworkLayer::layerName()
{
    return m_longNames[m_layer];
}

QString NetworkLayer::layerShortName()
{
    return m_shortNames[m_layer];
}
