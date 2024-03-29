#include "udpsenderlistmodel.h"

#include <QLocale>

UdpSenderListModel::UdpSenderListModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    // Refresh stats every second
    QTimer *statsTimer = new QTimer(this);
    connect(statsTimer, SIGNAL(timeout()), this, SLOT(updateStats()));
    statsTimer->start(1000);
}

int UdpSenderListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_udpSenderList.count();
}

int UdpSenderListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return COL_COUNT;
}

QVariant UdpSenderListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() > m_udpSenderList.count())
        return QVariant();

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
            case COL_SENDINGSTATS:
            case COL_RECEIVINGSTATS:
            case COL_WANSENDINGSTATS:
            case COL_WANRECEIVINGSTATS:
                return Qt::AlignRight;
            default:
                return Qt::AlignLeft;
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    // l is the current locale. As we have pretty long lines, l is shorter ;-)
    QLocale l = QLocale();
    // s is the current sender. As we have pretty long lines, s is shorter ;-)
    UdpSender *s = m_udpSenderList[index.row()];
    QString tmpText = "";
    int packetsSent = 0;
    qreal percent;
    int packetsNotSent = 0;
    int packetsReceived = 0;
    int packetsLost = 0;

    switch (index.column()) {
        case COL_NAME:
            return s->name();
        case COL_DSCP:
            return s->dscp();
        case COL_BANDWIDTH:
            return l.toString((qreal) s->specifiedBandwidth(m_BandwidthLayer) / m_BandwidthUnit,
                    'f', QLocale::FloatingPointShortest);
        case COL_PORT:
            return s->port();
        case COL_SIZE:
            return l.toString(s->specifiedPduSize(m_PDUSizeLayer));
        case COL_TC:
            return s->tcMsec();
        case COL_SENDINGSTATS:
            tmpText += "L1 " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::EthernetLayer1) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "L2 " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::EthernetLayer2) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "L2noCRC " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::EthernetLayer2woCRC) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "IP " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::IPLayer) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "UDP " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::UDPLayer) / m_BandwidthUnit, 'f', 2);
            return tmpText;
        case COL_RECEIVINGSTATS:
            tmpText += "L1 " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::EthernetLayer1) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "L2 " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::EthernetLayer2) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "L2noCRC " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::EthernetLayer2woCRC) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "IP " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::IPLayer) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "UDP " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::UDPLayer) / m_BandwidthUnit, 'f', 2);
            return tmpText;
        case COL_SENDINGPACKETS:
            packetsSent= s->packetsSent();
            packetsNotSent = s->packetsNotSent();
            percent = (qreal) packetsNotSent * 100 / (packetsSent + packetsNotSent);
            tmpText += "Packets sent: " + l.toString(packetsSent) + "\n";
            tmpText += "Packets not sent: " + l.toString(packetsNotSent) + "\n";
            tmpText += "Percent not sent: " + l.toString(percent) + "%\n";
            tmpText += "pps " + l.toString(s->sendingPps());
            return tmpText;
        case COL_RECEIVINGPACKETS:
            packetsSent = s->packetsSent();
            packetsReceived = s->packetsReceived();
            packetsLost = s->packetLost();
            percent = (qreal) packetsLost * 100 / packetsSent;
            tmpText += "Packets received: " + l.toString(packetsReceived) + "\n";
            tmpText += "Packets lost: " + l.toString(packetsLost) + "\n";
            tmpText += "Percent lost: " + l.toString(percent) + "%\n";
            tmpText += "pps " + l.toString(s->receivingPps());
            return tmpText;
        case COL_WANSENDINGSTATS:
            return WANSendingStats(index);
        case COL_WANRECEIVINGSTATS:
           return WANReceivingStats(index);
    }
    return QVariant();
}

QVariant UdpSenderListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Vertical) {
        return section;
    }

    switch (section) {
        case COL_NAME:
            return "Name";
        case COL_DSCP:
            return "DSCP Value";
        case COL_BANDWIDTH:
            return NetworkModel::layerShortName(m_BandwidthLayer) + " spec. Bandwidth";
        case COL_PORT:
            return "UDP Port";
        case COL_SIZE:
            return NetworkModel::layerShortName(m_PDUSizeLayer) + " spec. PDU Size";
        case COL_TC:
            return "Tc (msec)";
        case COL_SENDINGSTATS:
            return "LAN sending BW";
        case COL_RECEIVINGSTATS:
            return "LAN receiving BW";
        case COL_SENDINGPACKETS:
            return "Packets Sent";
        case COL_RECEIVINGPACKETS:
            return "Packets Received";
        case COL_WANSENDINGSTATS:
            return "WAN sending BW";
        case COL_WANRECEIVINGSTATS:
           return "WAN receiving BW";
    }
    return QVariant();
}

bool UdpSenderListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (index.row() > m_udpSenderList.count())
        return false;

    if (role != Qt::EditRole) {
        return false;
    }

    QLocale locale;
    QString stringValue = value.toString();

    switch (index.column()) {
        case COL_NAME:
            m_udpSenderList[index.row()]->setName(value.toString());
            emit dataChanged(index, index);
            return true;
            break;
        case COL_DSCP:
            m_udpSenderList[index.row()]->setDscp(value.toUInt());
            emit dataChanged(index, index);
            return true;
            break;
        case COL_BANDWIDTH:
            m_udpSenderList[index.row()]->setBandwidth(locale.toDouble(stringValue) * m_BandwidthUnit, m_BandwidthLayer);
            emit dataChanged(index, index);
            return true;
            break;
        case COL_PORT:
            m_udpSenderList[index.row()]->setPort(value.toUInt());
            emit dataChanged(index, index);
            return true;
            break;
        case COL_SIZE:
            m_udpSenderList[index.row()]->setPduSize(locale.toUInt(stringValue), m_PDUSizeLayer);
            emit dataChanged(index, index);
            return true;
            break;
        case COL_TC:
            m_udpSenderList[index.row()]->setTcMsec(value.toUInt());
            emit dataChanged(index, index);
            return true;
            break;
    }

    return false;
}

Qt::ItemFlags UdpSenderListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    // These columns are not editable
    if (index.column() == COL_SENDINGSTATS
            || index.column() == COL_RECEIVINGSTATS
            || index.column() == COL_SENDINGPACKETS
            || index.column() == COL_RECEIVINGPACKETS
            || index.column() == COL_WANSENDINGSTATS
            || index.column() == COL_WANRECEIVINGSTATS
            ) {
        return Qt::ItemIsSelectable;
    }

    if (m_isGeneratingTraffic) {
        if (index.column() == COL_PORT) {
            // These Columns can not be edited while generating trafic
            return  Qt::ItemIsSelectable;
        }
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool UdpSenderListModel::insertRows(int position, int rows, const QModelIndex &/* index */)
{
    UdpSender *sender;

    if (position < 0 || position > m_udpSenderList.count())
        return false;

    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; row++) {
        sender = new UdpSender();
        sender->setDestination(m_destination);
        sender->setWANLayerModel(m_WANLayerModel);

        m_udpSenderList.insert(position, sender);
        if (m_isGeneratingTraffic) {
            sender->startTraffic();
        }
    }

    endInsertRows();
    return true;
}

bool UdpSenderListModel::removeRows(int position, int rows, const QModelIndex &/* index */)
{
    UdpSender *sender;

    if (position < 0 || position > m_udpSenderList.count())
        return false;

    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; ++row) {
        sender = m_udpSenderList.takeAt(position);
        delete sender;
    }

    endRemoveRows();
    return true;
}

void UdpSenderListModel::setWANLayerModel(NetworkLayerListModel *WANmodel)
{
    m_WANLayerModel = WANmodel;

    UdpSender *sender;
    foreach (sender, m_udpSenderList) {
        sender->setWANLayerModel(WANmodel);
    }
}

void UdpSenderListModel::WANLayerModelChanged()
{
    UdpSender *sender;
    foreach (sender, m_udpSenderList) {
        sender->updateWANLayerModel(m_WANLayerModel);
    }
}

void UdpSenderListModel::setPDUSizeLayer(NetworkModel::Layer layer)
{
    m_PDUSizeLayer = layer;
    emit headerDataChanged(Qt::Horizontal, 0, columnCount());
    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));
}

void UdpSenderListModel::setBandwidthLayer(NetworkModel::Layer layer)
{
    m_BandwidthLayer = layer;

    /* We need to store the new Bandwidth corresponding to the new layer
       If we do not do this, the udpSender stil has the old layer stored, and
       each time we change the PDU Size, the Bandwidth will be modified.
       */
    UdpSender *sender;
    foreach (sender, m_udpSenderList) {
        sender->setBandwidth(sender->specifiedBandwidth(m_BandwidthLayer), m_BandwidthLayer);
    }

    emit headerDataChanged(Qt::Horizontal, 0, columnCount());
    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));}

void UdpSenderListModel::setBandwidthUnit(int bandwidthUnit)
{
    m_BandwidthUnit = bandwidthUnit;

    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));
}

void UdpSenderListModel::stopAllSender()
{
    UdpSender *sender;
    foreach (sender, m_udpSenderList) {
        sender->stopTraffic();
    }
    m_isGeneratingTraffic = false;
}

void UdpSenderListModel::generateTraffic()
{
    UdpSender *sender;

    // Refresh the Table
    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));

    /* Starts to generate Traffic */
    foreach (sender, m_udpSenderList) {
        sender->startTraffic();
    }
    m_isGeneratingTraffic = true;
}

void UdpSenderListModel::setDestinationIP(QHostAddress destinationIP)
{
    m_destination = destinationIP;

    UdpSender *sender;
    foreach (sender, m_udpSenderList) {
        sender->setDestination(destinationIP);
    }
}

qreal UdpSenderListModel::totalSpecifiedBandwidth(NetworkModel::Layer layer)
{
    qreal totalBandwidth = 0;
    UdpSender *sender;
    foreach (sender, m_udpSenderList) {
        totalBandwidth += sender->specifiedBandwidth(layer);
    }

    return totalBandwidth;
}

QString UdpSenderListModel::totalSendingStats()
{
    QString tmpText = "";
    UdpSender *s;
    QLocale l;
    qreal bw;
    int pps;

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::EthernetLayer1);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L1 " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::EthernetLayer2);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L2 " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::EthernetLayer2woCRC);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L2noCRC " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::IPLayer);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "IP " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::UDPLayer);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "UDP " + l.toString(bw, 'f', 2) + "\n";

    pps = 0;
    foreach (s, m_udpSenderList) {
        pps += s->sendingPps();
    }
    tmpText += "pps " + l.toString(pps);


    return tmpText;
}

QString UdpSenderListModel::totalReceivingStats()
{
    QString tmpText = "";
    UdpSender *s;
    QLocale l;
    qreal bw;
    int pps;

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::EthernetLayer1);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L1 " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::EthernetLayer2);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L2 " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::EthernetLayer2woCRC);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L2noCRC " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::IPLayer);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "IP " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, m_udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::UDPLayer);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "UDP " + l.toString(bw, 'f', 2) + "\n";

    pps = 0;
    foreach (s, m_udpSenderList) {
        pps += s->receivingPps();
    }
    tmpText += "pps " + l.toString(pps);


    return tmpText;
}

QString UdpSenderListModel::totalPacketsStats()
{
    QString tmpText = "";
    UdpSender *s;
    QLocale l;

    int packetsSent = 0;
    foreach (s, m_udpSenderList) {
        packetsSent += s->packetsSent();
    }

    int packetsReceived = 0;
    foreach (s, m_udpSenderList) {
        packetsReceived += s->packetsReceived();
    }

    int packetsLost = 0;
    foreach (s, m_udpSenderList) {
        packetsLost += s->packetLost();
    }

    qreal percent = (qreal) packetsLost * 100 / packetsSent;

    tmpText += "Packets sent: " + l.toString(packetsSent) + "\n";
    tmpText += "Packets received: " + l.toString(packetsReceived) + "\n";
    tmpText += "Packets lost: " + l.toString(packetsLost) + "\n";
    tmpText += "Percent lost: " + l.toString(percent) + "%";
    return tmpText;
}

QString UdpSenderListModel::WANSendingStats(const QModelIndex &index) const
{
    UdpSender *s = m_udpSenderList[index.row()];
    QString tmpText = "";
    bool newlineNeeded = false;

    if (m_WANLayerModel == NULL) {
        return "WAN Model not initialised";
    }

    QList <int> bwList = s->WANsendingBandwidth();
    QList <QString> nameList = m_WANLayerModel->layerShortNameList();
    QList <bool> displayStatList = m_WANLayerModel->displayStatList();

    int count = bwList.count();
    if (count != nameList.count() || count != displayStatList.count()) {
        return "Error in WAN model";
    }

    QLocale l = QLocale();
    int i;

    for (i = 0; i < count; i++) {
        if (!displayStatList[i]) {
            continue;
        }
        if (newlineNeeded) {
            tmpText += "\n";
        }
        tmpText += nameList[i] + " ";
        tmpText += l.toString((qreal) bwList[i] / m_BandwidthUnit, 'f', 2);
        newlineNeeded = true;
    }

    return tmpText;
}

QString UdpSenderListModel::WANReceivingStats(const QModelIndex &index) const
{
    UdpSender *s = m_udpSenderList[index.row()];
    QString tmpText = "";
    bool newlineNeeded = false;

    if (m_WANLayerModel == NULL) {
        return "WAN Model not initialised";
    }

    QList <int> bwList = s->WANreceivingBandwidth();
    QList <QString> nameList = m_WANLayerModel->layerShortNameList();
    QList <bool> displayStatList = m_WANLayerModel->displayStatList();

    int count = bwList.count();
    if (count != nameList.count() || count != displayStatList.count()) {
        return "Error in WAN model";
    }

    QLocale l = QLocale();
    int i;

    for (i = 0; i < count; i++) {
        if (!displayStatList[i]) {
            continue;
        }
        if (newlineNeeded) {
            tmpText += "\n";
        }
        tmpText += nameList[i] + " ";
        tmpText += l.toString((qreal) bwList[i] / m_BandwidthUnit, 'f', 2);
        newlineNeeded = true;
    }

    return tmpText;
}

QString UdpSenderListModel::WANtotalReceivingStats()
{
    UdpSender *s;
    QList <QString> nameList = m_WANLayerModel->layerShortNameList();
    QList <bool> displayStatList = m_WANLayerModel->displayStatList();
    QList <int> bwListTotal;
    QList <int> bwList;
    int i;

    int count = nameList.count();
    if (count != displayStatList.count()) {
        return "Error in WAN model";
    }

    for (i = 0; i < count; i++) {
        bwListTotal.append(0);
    }

    foreach(s, m_udpSenderList) {
         bwList = s->WANreceivingBandwidth();
         if (count != bwList.count()) {
             return "Error in WAN model";
         }
         for (i = 0; i < count; i++) {
             if (!displayStatList[i]) {
                 continue;
             }
             bwListTotal[i] += bwList[i];
         }
    }

    QString tmpText = "";
    bool newlineNeeded = false;
    QLocale l = QLocale();
    for (i = 0; i < count; i++) {
        if (!displayStatList[i]) {
            continue;
        }
        if (newlineNeeded) {
            tmpText += "\n";
        }
        tmpText += nameList[i] + " ";
        tmpText += l.toString((qreal) bwListTotal[i] / m_BandwidthUnit, 'f', 2);
        newlineNeeded = true;
    }

    return tmpText;
}

QString UdpSenderListModel::WANtotalSendingStats()
{
    UdpSender *s;
    QList <QString> nameList = m_WANLayerModel->layerShortNameList();
    QList <bool> displayStatList = m_WANLayerModel->displayStatList();
    QList <int> bwListTotal;
    QList <int> bwList;
    int i;

    int count = nameList.count();
    if (count != displayStatList.count()) {
        return "Error in WAN model";
    }

    for (i = 0; i < count; i++) {
        bwListTotal.append(0);
    }

    foreach(s, m_udpSenderList) {
         bwList = s->WANsendingBandwidth();
         if (count != bwList.count()) {
             return "Error in WAN model";
         }
         for (i = 0; i < count; i++) {
             if (!displayStatList[i]) {
                 continue;
             }
             bwListTotal[i] += bwList[i];
         }
    }

    QString tmpText = "";
    bool newlineNeeded = false;
    QLocale l = QLocale();
    for (i = 0; i < count; i++) {
        if (!displayStatList[i]) {
            continue;
        }
        if (newlineNeeded) {
            tmpText += "\n";
        }
        tmpText += nameList[i] + " ";
        tmpText += l.toString((qreal) bwListTotal[i] / m_BandwidthUnit, 'f', 2);
        newlineNeeded = true;
    }

    return tmpText;
}

void UdpSenderListModel::saveParameter(QSettings &settings)
{
    int row;
    const int rowCount = m_udpSenderList.count();
    UdpSender *sender;

    settings.beginWriteArray("Flows");

    for (row = 0; row < rowCount ; row++) {
        settings.setArrayIndex(row);
        sender = m_udpSenderList[row];

        settings.setValue("name", sender->name());
        settings.setValue("bandwidth", sender->specifiedBandwidth(m_BandwidthLayer));
        settings.setValue("dscp", sender->dscp());
        settings.setValue("size", sender->specifiedPduSize(m_PDUSizeLayer));
        settings.setValue("tc", sender->tcMsec());
    }

    settings.endArray();
}

void UdpSenderListModel::loadParameter(QSettings &settings)
{
    int row;
    UdpSender *sender;

    // Tell the model that we will change all the data
    beginResetModel();

    // Delete all items from memory
    qDeleteAll(m_udpSenderList.begin(), m_udpSenderList.end());
    // then remove them from the list
    m_udpSenderList.clear();


    const int rowCount = settings.beginReadArray("Flows");

    for (row = 0; row < rowCount ; row++) {
        settings.setArrayIndex(row);

        sender = new UdpSender();
        sender->setDestination(m_destination);
        sender->setWANLayerModel(m_WANLayerModel);

        sender->setName(settings.value("name").toString());
        sender->setBandwidth(settings.value("bandwidth").toUInt(), m_BandwidthLayer);
        sender->setDscp(settings.value("dscp").toUInt());
        sender->setPduSize(settings.value("size").toUInt(), m_BandwidthLayer);
        sender->setTcMsec(settings.value("tc").toUInt());

        m_udpSenderList.append(sender);
    }

     settings.endArray();

    // Tell the model that we are done with changing data
    endResetModel();
}

void UdpSenderListModel::updateStats()
{
    emit dataChanged(index(0, COL_SENDINGSTATS), index(rowCount()-1, COL_RECEIVINGPACKETS));
}

