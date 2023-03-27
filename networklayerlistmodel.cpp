#include <QDebug>

#include "networklayerlistmodel.h"

NetworkLayerListModel::NetworkLayerListModel()
{
}

int NetworkLayerListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_networklayerList.count();
}

int NetworkLayerListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return COL_COUNT;
}

QVariant NetworkLayerListModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
            case COL_DISPLAYSTAT:
                // NOTE: This is not working. Centering the checkbox needs a QItemDelegate and I do not want to spend
                // time in this for now :-)
                return Qt::AlignHCenter;
        }
    }

    NetworkLayer *n = m_networklayerList[index.row()];

    switch (index.column()) {
    case COL_NAME:
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return n->layerShortName();
        if (role == Qt::ToolTipRole)
            return n->layerName();
        break;
    case COL_DISPLAYSTAT:
        if (role == Qt::CheckStateRole) {
            if (m_displayStatsList[index.row()])
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
        break;
    default:
        if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole)
            return "not implemented";
    }

    // if we did not return anything until now, just return QVariant();
    return QVariant();
}

bool NetworkLayerListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    int row = index.row();
    if (row > m_displayStatsList.count())
        return false;

    switch (index.column()) {
        case COL_DISPLAYSTAT:
            if (role == Qt::CheckStateRole && (Qt::CheckState)value.toInt() == Qt::Checked) {
                m_displayStatsList[row] = true;
            } else {
                m_displayStatsList[row] = false;
            }
            emit dataChanged(index, index);
            return true;
            break;
    }
    return false;
}

QVariant NetworkLayerListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Vertical) {
        return section;
    }

    switch (section) {
    case COL_NAME:
        return "Layer";
    case COL_DISPLAYSTAT:
        return "Stats";
    }

    return QVariant();
}

Qt::ItemFlags NetworkLayerListModel::flags(const QModelIndex &index) const
{
    if (index.column() == COL_DISPLAYSTAT) {
        return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void NetworkLayerListModel::appendLayer(NetworkLayer::Layer layer)
{
    NetworkLayer *nl;
    nl = new NetworkLayer(layer);
    int insertedRow = m_networklayerList.count();

    beginInsertRows(QModelIndex(), insertedRow, insertedRow);
    m_networklayerList.append(nl);
    m_displayStatsList.append(true);

    endInsertRows();
}

void NetworkLayerListModel::removeLastLayer()
{
    NetworkLayer *nl;
    int lastRow = m_networklayerList.count() - 1;
    if (lastRow <= 0)
        /* we do not remove the last Layer */
        return;

    beginRemoveRows(QModelIndex(), lastRow, lastRow);
    nl = m_networklayerList.takeLast();
    delete nl;
    m_displayStatsList.removeLast();

    endRemoveRows();
}

NetworkLayer::Layer NetworkLayerListModel::lastLayer()
{
    if (m_networklayerList.isEmpty()) {
        /* this should never happen */
        Q_ASSERT(false);
        return NetworkLayer::UDP;
    }
    return m_networklayerList.last()->layer();
}

bool NetworkLayerListModel::isLastLayer(const QModelIndex &index)
{
    if (index.row() == m_networklayerList.count() - 1) {
        return true;
    } else {
        return false;
    }
}

NetworkLayer::Layer NetworkLayerListModel::layerAt(const QModelIndex &index)
{
    NetworkLayer *n = m_networklayerList[index.row()];
    return n->layer();
}

void NetworkLayerListModel::fillWithLayers(QList<NetworkLayer::Layer> layerList)
{
    NetworkLayer::Layer layer;

    int lastRow = m_networklayerList.count() - 1;
    if (lastRow >= 0) {
        // We have something to delete
        beginRemoveRows(QModelIndex(), 0, lastRow);
        // First clear current List
        qDeleteAll(m_networklayerList.begin(), m_networklayerList.end());
        m_networklayerList.clear();
        m_displayStatsList.clear();
        endRemoveRows();
    }

    foreach (layer, layerList) {
        appendLayer(layer);
    }
}

QList<NetworkLayer::Layer> NetworkLayerListModel::layerList()
{
    NetworkLayer *networkLayer;
    QList<NetworkLayer::Layer> layerList;

    if (m_networklayerList.isEmpty()) {
        return layerList;
    }

    foreach (networkLayer, m_networklayerList) {
        layerList.append(networkLayer->layer());
    }

    return layerList;
}

QList<uint> NetworkLayerListModel::layerPDUSize()
{
    NetworkLayer *networkLayer;
    QList<uint> list;

    if (m_networklayerList.isEmpty()) {
        return list;
    }

    foreach (networkLayer, m_networklayerList) {
        list.append(networkLayer->PDUSize());
    }

    return list;
}

QList<QString> NetworkLayerListModel::layerShortNameList()
{
    NetworkLayer *networkLayer = NULL;
    QList<QString> list;

    if (m_networklayerList.isEmpty()) {
        return list;
    }

    foreach (networkLayer, m_networklayerList) {
        list.append(networkLayer->layerShortName());
    }
    return list;
}

QList<bool> NetworkLayerListModel::displayStatList()
{
    return m_displayStatsList;
}

// Duplicates the current model. As we have a complex structure (and QOBJECT), we can't just copy it
// The caller must ensure the memory is cleared.
NetworkLayerListModel *NetworkLayerListModel::clone()
{
    NetworkLayerListModel *model = new NetworkLayerListModel();

    model->fillWithLayers(this->layerList());
    // we do not copy the m_displayStatsList as it is not used in udpsender for stats.

    return model;
}

void NetworkLayerListModel::setUDPPDUSize(uint size)
{
    setPDUSize(0, size);
}

void NetworkLayerListModel::setPDUSize(const uint row, const uint size)
{
    const int maxrows = m_networklayerList.count() - 1;
    if (static_cast<int>(row) > maxrows) {
        return;
    }

    int i;
    uint SDUSizePreviousLayer = size;

    // Go all layers up to the highest layer
    for (i = row; i >= 0; i--) {
        m_networklayerList[i]->setPDUSize(SDUSizePreviousLayer);
        SDUSizePreviousLayer = m_networklayerList[i]->SDUSize();
    }

    // Now go all layers down to the lowest layer
    uint PDUSizePreviousLayer = m_networklayerList[0]->PDUSize();
    for (i = 1; i <= maxrows; i++) {
        m_networklayerList[i]->setSDUSize(PDUSizePreviousLayer);
        PDUSizePreviousLayer = m_networklayerList[i]->PDUSize();
    }

    // Finaly, as we may have a change in PDUs, wie go all layers up again
    SDUSizePreviousLayer = m_networklayerList[maxrows]->SDUSize();
    for (i = maxrows - 1; i>= 0; i--) {
        m_networklayerList[i]->setPDUSize(SDUSizePreviousLayer);
        SDUSizePreviousLayer = m_networklayerList[i]->SDUSize();
    }
}

/** Saves the networklayer list to settings
 *
 * we save the short name of the layers and not its enum value so that the stats are human readable
 */
void NetworkLayerListModel::saveParameter(QSettings &settings)
{
    int row;
    const int rowCount = m_networklayerList.count();
    NetworkLayer *layer;

    // NOTE: as soon we want to user NetworkLayerListModel for the LAN Layers, we need to pass the settings section
    // as an argument
    settings.beginWriteArray("WAN-Layers");

    for (row = 0; row < rowCount ; row++) {
        settings.setArrayIndex(row);
        layer = m_networklayerList[row];

        settings.setValue("name", layer->layerShortName());
        settings.setValue("stats", m_displayStatsList[row]);
    }

    settings.endArray();
}

/** Loads the networklayer list from settings
 *
 * As we saved the short name of the layers, we will interupt loading the layers as soon as a short name is unknown
 */
void NetworkLayerListModel::loadParameter(QSettings &settings)
{
    int row;
    NetworkLayer *layer, *previousLayer;
    NetworkLayer::Layer layerID;
    QString layerName;
    bool displayStats;

    // Tell the model that we will change all the data
    beginResetModel();

    // Delete all items from memory
    qDeleteAll(m_networklayerList.begin(), m_networklayerList.end());
    // then remove them from the list
    m_networklayerList.clear();

    // a QList<bool> is easier to clean ;-)
    m_displayStatsList.clear();

    // NOTE: as soon we want to user NetworkLayerListModel for the LAN Layers, we need to pass the settings section
    // as an argument
    const int rowCount = settings.beginReadArray("WAN-Layers");

    for (row = 0; row < rowCount ; row++) {
        settings.setArrayIndex(row);

        layerName = settings.value("name", "").toString();
        displayStats = settings.value("stats", false).toBool();

        layerID = NetworkLayer::shortname2Layer(layerName);
        if (row == 0) {
            if (layerID != NetworkLayer::UDP) {
                // The first layer must always be UDP. Insert it an break
                layer = new NetworkLayer(NetworkLayer::UDP);
                m_networklayerList.append(layer);
                m_displayStatsList.append(true);
                break;
            }
        } else {
            // For rows > 0, check if this is an acceptable Layer. If not, break
            if (!previousLayer->hasPossibleSublayer(layerID)) {
                break;
            }
        }
        layer = new NetworkLayer(layerID);
        m_networklayerList.append(layer);
        m_displayStatsList.append(displayStats);
        previousLayer = layer;
    }

    // If there was a problem with the project, at least insert an UDP Layer
    if (m_networklayerList.count() == 0) {
        layer = new NetworkLayer(NetworkLayer::UDP);
        m_networklayerList.append(layer);
        m_displayStatsList.append(true);
    }

    settings.endArray();

    // Tell the model that we are done with changing data
    endResetModel();
}

