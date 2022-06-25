#ifndef NETWORKLAYER_H
#define NETWORKLAYER_H

#include <QtGlobal>
#include <QList>

class NetworkLayer
{
public:
    enum Layer {
        EthernetL1, // 0
        EthernetL2,
        EthernetL2woCRC,
        EthernetCRC,
        IP,
        UDP,
        GRE,
        GREWithKey,
        IPSec,
        LAYER_COUNT // Used to know how much layer we have
    };

    NetworkLayer(NetworkLayer::Layer layer);
    ~NetworkLayer();

    static QList<NetworkLayer::Layer> possibleSubLayers(NetworkLayer::Layer layer);
    void setLowerLayer(NetworkLayer *networkLayer);
    void setHigherLayer(NetworkLayer *networkLayer);
    void removeLoweLayer();
    void removeHigherLayer();
    NetworkLayer::Layer layer();

    uint setPDUSize(uint size);
    uint PDUSize();

    QString layerName();
    QString layerShortName();


private:
    NetworkLayer::Layer m_layer;
    NetworkLayer *m_lowerLayer = NULL;
    NetworkLayer *m_higherLayer = NULL;

    const uint m_overhead[LAYER_COUNT] = {
        20, // EthL1
        18, // EthL2
        14, // EthL2 without CRC
        4,  // CRC for Eth L2
        20, // IP
        8,  // UDP
        4,  // GRE
        8,  // GRE with optional Key
        56  // FIXME: IPSec has to be calculated because of padding
    };

    // If defined, take the minimum size for the protocol. If not, take the header size.
    const uint m_minPDUsize[LAYER_COUNT] = {
        84, // EthL1 - derived from EthL2 + 20 overhead
        64, // EthL2 - defined by IEE802.3 - 512 bit slot time for collision detection
        60, // EthL2 without CRC - derived from EthL2 without CRC/FCS
        64, // CRC for Eth L2 - defined by IEE802.3
        20, // IP - IP without payload
        24, // UDP - UDP hedaer + 2x 8 Bytes for timestamp and seq. Number.
        4,  // GRE
        8,  // GRE with optional Key
        56  // FIXME: IPSec has to be calculated because of padding
    };

    // We mean here the maximal PDU size in order to avoid fragmentation.
    const uint m_maxPDUsize[LAYER_COUNT] = {
        1600, // EthL1 - we do not which EthL2 we have (802.1q = EthL2 + 4 Bytes; Cisco Trustsec...)
        1518, // EthL2 - 1500 Eth2-Payload + 18 overhead
        1514, // EthL2 without CRC - derived from EthL2 without CRC/FCS
        1518, // CRC for Eth L2: 1500 Eth2-Payload + 18 overhead
        1500, // IP - IP MTU from IEE802.3 (without jumbo frames)
        1500, // UDP - no limit (we don't know what transports us)
        1500, // GRE - no limit (we don't know what transports us)
        1500, // GRE with optional Key
        1500  // IPSec has to be calculated because of padding
    };

    const char* m_shortNames[LAYER_COUNT] = {
        "Eth L1",
        "Eth L2",
        "Eth L2 w/o CRC",
        "Eth L2 (CRC)",
        "IP",
        "UDP",
        "GRE",
        "GRE+key",
        "IPSec"
    };

    const char* m_longNames[LAYER_COUNT] = {
        "Ethernet Layer1",
        "Ethernet Layer2",
        "Ethernet Layer2 without CRC field",
        "Ethernet Layer2 with CRC field",
        "IP",
        "UDP",
        "GRE",
        "GRE with tunnel key",
        "IPSec"
    };


    uint m_PDUSize;

};

#endif // NETWORKLAYER_H
