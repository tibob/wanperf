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

    return udpSenderList.count();
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

    if (index.row() > udpSenderList.count())
        return QVariant();

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
            case COL_SENDINGSTATS:
            case COL_RECEIVINGSTATS:
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
    UdpSender *s = udpSenderList[index.row()];
    QString tmpText = "";

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
        case COL_SENDINGSTATS:
            tmpText += "L1 " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::Layer1) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "L2 " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::Layer2) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "L2noCRC " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::Layer2noCRC) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "IP " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::Layer3) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "UDP " +
              l.toString((qreal) s->sendingBandwidth(NetworkModel::Layer4) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "pps " + l.toString(s->sendingPps());
            return tmpText;
        case COL_RECEIVINGSTATS:
            tmpText += "L1 " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::Layer1) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "L2 " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::Layer2) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "L2noCRC " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::Layer2noCRC) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "IP " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::Layer3) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "UDP " +
              l.toString((qreal) s->receivingBandwidth(NetworkModel::Layer4) / m_BandwidthUnit, 'f', 2) + "\n";
            tmpText += "pps " + l.toString(s->receivingPps());
            return tmpText;
        case COL_PACKETS:
            int packetsSent= s->packetsSent();
            int packetsReceived= s->packetsReceived();
            int packetsLost = s->packetLost();
            qreal percent = (qreal) packetsLost * 100 / packetsSent;
            tmpText += "Packets sent: " + l.toString(packetsSent) + "\n";
            tmpText += "Packets received: " + l.toString(packetsReceived) + "\n";
            tmpText += "Packets lost: " + l.toString(packetsLost) + "\n";
            tmpText += "Percent: " + l.toString(percent) + "%";
            return tmpText;
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
            return NetworkModel::layerShortName(m_BandwidthLayer) + " specified Bandwidth";
        case COL_PORT:
            return "UDP Port";
        case COL_SIZE:
            return NetworkModel::layerShortName(m_PDUSizeLayer) + " specified PDU Size";
        case COL_SENDINGSTATS:
            return "Sending bandwidth";
        case COL_RECEIVINGSTATS:
            return "Receiving bandwidth";
        case COL_PACKETS:
            return "Packets";

    }
    return QVariant();
}

bool UdpSenderListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (index.row() > udpSenderList.count())
        return false;

    if (role != Qt::EditRole) {
        return false;
    }

    QLocale locale;
    QString stringValue = value.toString();

    switch (index.column()) {
        case COL_NAME:
            udpSenderList[index.row()]->setName(value.toString());
            emit dataChanged(index, index);
            return true;
            break;
        case COL_DSCP:
            udpSenderList[index.row()]->setDscp(value.toUInt());
            emit dataChanged(index, index);
            return true;
            break;
        case COL_BANDWIDTH:
            udpSenderList[index.row()]->setBandwidth(locale.toDouble(stringValue) * m_BandwidthUnit, m_BandwidthLayer);
            emit dataChanged(index, index);
            return true;
            break;
        case COL_PORT:
            udpSenderList[index.row()]->setPort(value.toUInt());
            emit dataChanged(index, index);
            return true;
            break;
        case COL_SIZE:
            udpSenderList[index.row()]->setPduSize(locale.toUInt(stringValue), m_PDUSizeLayer);
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
            || index.column() == COL_PACKETS) {
        return Qt::ItemIsSelectable;
    }

    if (m_isGeneratingTraffic) {
        if (index.column() == COL_DSCP || index.column() == COL_PORT) {
            // These Columns can not be edited while generating Trafic
            return  Qt::ItemIsSelectable;
        }
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool UdpSenderListModel::insertRows(int position, int rows, const QModelIndex &/* index */)
{
    UdpSender *sender;

    if (position < 0 || position > udpSenderList.count())
        return false;

    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; row++) {
        sender = new UdpSender();
        sender->setDestination(m_destination);

        udpSenderList.insert(position, sender);
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

    if (position < 0 || position > udpSenderList.count())
        return false;

    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; ++row) {
        sender = udpSenderList.takeAt(position);
        delete sender;
    }

    endRemoveRows();
    return true;
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
    foreach (sender, udpSenderList) {
        sender->setBandwidth(sender->specifiedBandwidth(m_BandwidthLayer), m_BandwidthLayer);
    }

    emit headerDataChanged(Qt::Horizontal, 0, columnCount());
    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));}

void UdpSenderListModel::setBandwidthUnit(int bandwidthUnit)
{
    m_BandwidthUnit = bandwidthUnit;

    // FIXME: I'd like to have the BandwidthUnit in the Header, but we have to pass
    // a Networkmodel::BandwidthUnit, so that we can get the name an the value from the Networkmodel
    //     emit headerDataChanged(Qt::Horizontal, 0, columnCount());

    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));
}

void UdpSenderListModel::stopAllSender()
{
    UdpSender *sender;
    foreach (sender, udpSenderList) {
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
    foreach (sender, udpSenderList) {
        sender->startTraffic();
    }
    m_isGeneratingTraffic = true;
}

void UdpSenderListModel::setDestinationIP(QHostAddress destinationIP)
{
    m_destination = destinationIP;

    UdpSender *sender;
    foreach (sender, udpSenderList) {
        sender->setDestination(destinationIP);
    }
}

qreal UdpSenderListModel::totalSpecifiedBandwidth(NetworkModel::Layer layer)
{
    qreal totalBandwidth = 0;
    UdpSender *sender;
    foreach (sender, udpSenderList) {
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
    foreach (s, udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::Layer1);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L1 " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::Layer2);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L2 " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::Layer2noCRC);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L2noCRC " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::Layer3);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "IP " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, udpSenderList) {
        bw += s->sendingBandwidth(NetworkModel::Layer4);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "UDP " + l.toString(bw, 'f', 2) + "\n";

    pps = 0;
    foreach (s, udpSenderList) {
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
    foreach (s, udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::Layer1);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L1 " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::Layer2);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L2 " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::Layer2noCRC);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "L2noCRC " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::Layer3);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "IP " + l.toString(bw, 'f', 2) + "\n";

    bw = 0;
    foreach (s, udpSenderList) {
        bw += s->receivingBandwidth(NetworkModel::Layer4);
    }
    bw = bw / m_BandwidthUnit;
    tmpText += "UDP " + l.toString(bw, 'f', 2) + "\n";

    pps = 0;
    foreach (s, udpSenderList) {
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
    foreach (s, udpSenderList) {
        packetsSent += s->packetsSent();
    }

    int packetsReceived = 0;
    foreach (s, udpSenderList) {
        packetsReceived += s->packetsReceived();
    }

    int packetsLost = 0;
    foreach (s, udpSenderList) {
        packetsLost += s->packetLost();
    }

    qreal percent = (qreal) packetsLost * 100 / packetsSent;

    tmpText += "Packets sent: " + l.toString(packetsSent) + "\n";
    tmpText += "Packets received: " + l.toString(packetsReceived) + "\n";
    tmpText += "Packets lost: " + l.toString(packetsLost) + "\n";
    tmpText += "Percent: " + l.toString(percent) + "%";
    return tmpText;
}

void UdpSenderListModel::updateStats()
{
    emit dataChanged(index(0, COL_SENDINGSTATS), index(rowCount()-1, COL_PACKETS));
}
