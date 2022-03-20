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
            case COL_SENDINGRATE:
            case COL_RECEIVINGRATE:
                return Qt::AlignRight;
            default:
                return Qt::AlignLeft;
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    QLocale locale = QLocale();

    switch (index.column()) {
        case COL_NAME:
            return udpSenderList[index.row()]->name();
        case COL_DSCP:
            return udpSenderList[index.row()]->dscp();
        case COL_BANDWIDTH:
            return locale.toString((qreal) udpSenderList[index.row()]->specifiedBandwidth(m_BandwidthLayer) / m_BandwidthUnit,
                    'f', QLocale::FloatingPointShortest);
        case COL_PORT:
            return udpSenderList[index.row()]->port();
        case COL_SIZE:
            return locale.toString(udpSenderList[index.row()]->specifiedPduSize(m_PDUSizeLayer));
        case COL_SENDINGRATE:
            return locale.toString((qreal) udpSenderList[index.row()]->sendingBandwidth(m_BandwidthLayer) / m_BandwidthUnit,
                    'f', 2);
        case COL_RECEIVINGRATE:
            return locale.toString((qreal) udpSenderList[index.row()]->receivingBandwidth(m_BandwidthLayer) / m_BandwidthUnit,
                    'f', 2);
        case COL_PACKETLOST:
            return locale.toString(udpSenderList[index.row()]->packetLost());
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
        // FIXME: we need a Network Model for the whole application, not for every udpSender
            return NetworkModel::layerShortName(m_BandwidthLayer) + " specified Bandwidth";
        case COL_PORT:
            return "UDP Port";
        case COL_SIZE:
            return NetworkModel::layerShortName(m_PDUSizeLayer) + " specified PDU Size";
        case COL_SENDINGRATE:
            return "Sending bandwidth";
        case COL_RECEIVINGRATE:
            return "Receiving bandwidth";
        case COL_PACKETLOST:
            return "Packets Lost";

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
    if (index.column() == COL_SENDINGRATE
            || index.column() == COL_RECEIVINGRATE
            || index.column() == COL_PACKETLOST) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
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

        udpSenderList.insert(position, sender);
        /* Be sure that the sender has its Bandwidth stored in the same Layer als the UI
         * This is to avoid the Bandwidth to change when the PDU Size is changed *
         */
        sender->setBandwidth(sender->specifiedBandwidth(m_BandwidthLayer), m_BandwidthLayer);
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

qreal UdpSenderListModel::totalSendingBandwidth(NetworkModel::Layer layer)
{
    qreal totalBandwidth = 0;
    UdpSender *sender;
    foreach (sender, udpSenderList) {
        totalBandwidth += sender->sendingBandwidth(layer);
    }

    return totalBandwidth;
}

qreal UdpSenderListModel::totalReceivingBandwidth(NetworkModel::Layer layer)
{
    qreal totalBandwidth = 0;
    UdpSender *sender;
    foreach (sender, udpSenderList) {
        totalBandwidth += sender->receivingBandwidth(layer);
    }

    return totalBandwidth;
}

int UdpSenderListModel::totalPacketLost()
{
    int totalLost = 0;
    UdpSender *sender;
    foreach (sender, udpSenderList) {
        totalLost += sender->packetLost();
    }

    return totalLost;
}

int UdpSenderListModel::totalPpsSent()
{
    int pps = 0;
    UdpSender *sender;
    foreach (sender, udpSenderList) {
        pps += sender->sendingPps();
    }

    return pps;
}

int UdpSenderListModel::totalPpsReceived()
{
    int pps = 0;
    UdpSender *sender;
    foreach (sender, udpSenderList) {
        pps += sender->receivingPps();
    }

    return pps;
}

void UdpSenderListModel::updateStats()
{
    emit dataChanged(index(0, COL_SENDINGRATE), index(rowCount()-1, COL_PACKETLOST));
}
