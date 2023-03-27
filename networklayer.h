#ifndef NETWORKLAYER_H
#define NETWORKLAYER_H

#include <QtGlobal>
#include <QList>
#include <QMap>

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
        ESP_AES256_SHA_TUN,
        LAYER_COUNT // Used to know how much layers we have
    };

    NetworkLayer(NetworkLayer::Layer layer);

    static QList<NetworkLayer::Layer> possibleSubLayers(NetworkLayer::Layer layer);
    bool hasPossibleSublayer(NetworkLayer::Layer layer);

    NetworkLayer::Layer layer();
    static NetworkLayer::Layer shortname2Layer(QString shortName);

    uint setPDUSize(uint size);
    uint setSDUSize(uint SDUSize);
    uint PDUSize();
    uint SDUSize();

    QString layerName();
    QString layerShortName();


private:
    NetworkLayer::Layer m_layer;

    // SUBLAYERS defines which layers can encapsulate one another
    static inline const QMap<NetworkLayer::Layer, QList<NetworkLayer::Layer>> SUBLAYERS {
        {NetworkLayer::UDP,         {NetworkLayer::IP}},
        {NetworkLayer::IP,          {NetworkLayer::EthernetL2, NetworkLayer::EthernetL2woCRC,
                                     NetworkLayer::GRE, NetworkLayer::GREWithKey,
                                     NetworkLayer::ESP_AES256_SHA_TUN}},
        {NetworkLayer::EthernetL2woCRC, {NetworkLayer::EthernetCRC}},
        {NetworkLayer::EthernetCRC, {EthernetL1}},
        {NetworkLayer::EthernetL2,  {EthernetL1}},
        {EthernetL1,                {}},
        {NetworkLayer::GRE,         {NetworkLayer::IP}},
        {NetworkLayer::GREWithKey,  {NetworkLayer::IP}},
        {NetworkLayer::ESP_AES256_SHA_TUN, {NetworkLayer::IP}}
    };

    static inline const uint m_overhead[] = {
        20, // EthL1
        18, // EthL2
        14, // EthL2 without CRC
        4,  // CRC for Eth L2
        20, // IP
        8,  // UDP
        4,  // GRE
        8,  // GRE with optional Key
        38  // ESP has to be calculated because of padding, 38 is the overhead without padding
    };

    // If defined, take the minimum size for the protocol. If not, take the header size.
    static inline const uint m_minPDUsize[] = {
        84, // EthL1 - derived from EthL2 + 20 overhead
        64, // EthL2 - defined by IEE802.3 - 512 bit slot time for collision detection
        60, // EthL2 without CRC - derived from EthL2 without CRC/FCS
        64, // CRC for Eth L2 - defined by IEE802.3
        20, // IP - IP without payload
        24, // UDP - UDP hedaer + 2x 8 Bytes for timestamp and seq. Number.
        4,  // GRE
        8,  // GRE with optional Key
        38  // ESP has to be calculated because of padding, 38 is the overhead without padding
    };

    // We mean here the maximal PDU size in order to avoid fragmentation.
    static inline const uint m_maxPDUsize[] = {
        1600, // EthL1 - we do not which EthL2 we have (802.1q = EthL2 + 4 Bytes; Cisco Trustsec...)
        1518, // EthL2 - 1500 Eth2-Payload + 18 overhead
        1514, // EthL2 without CRC - derived from EthL2 without CRC/FCS
        1518, // CRC for Eth L2: 1500 Eth2-Payload + 18 overhead
        1500, // IP - IP MTU from IEE802.3 (without jumbo frames)
        1500, // UDP - no limit (we don't know what transports us)
        1500, // GRE - no limit (we don't know what transports us)
        1500, // GREwKey - no limit (we don't know what transports us)
        1500  // ESP - no limit (we don't know what transports us)
    };

    static inline const char* const m_shortNames[] = {
        "Eth L1",
        "Eth L2",
        "Eth L2 w/o CRC",
        "Eth L2 (CRC)",
        "IP",
        "UDP",
        "GRE",
        "GRE+key",
        "ESP AES256+SHA Tun."
    };

    static inline const char* const m_longNames[] = {
        "Ethernet Layer1",
        "Ethernet Layer2",
        "Ethernet Layer2 without CRC field",
        "Ethernet Layer2 with CRC field",
        "IP",
        "UDP",
        "GRE",
        "GRE with tunnel key",
        "IPSec ESP Tunnel Mode AES256 & SHA-HMAC"
    };


    uint m_PDUSize;
    uint m_SDUSize;

    /* As ESP adds a padding, we try to keep the ordiginal SDU stored */
    uint m_ESPSDUSize = 0;
};

#endif // NETWORKLAYER_H
