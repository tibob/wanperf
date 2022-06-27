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

    // NOTE: in a future Version, we need to choose which columns to display
    return 1;
}

QVariant NetworkLayerListModel::data(const QModelIndex &index, int role) const
{
    NetworkLayer *n = m_networklayerList[index.row()];

    switch (index.column()) {
    case COL_NAME:
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return n->layerShortName();
        if (role == Qt::ToolTipRole)
            return n->layerName();
        break;
    default:
        if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole)
            return "not implemented";
    }

    // if we did not return anything until now, just return QVariant();
    return QVariant();
}

void NetworkLayerListModel::appendLayer(NetworkLayer::Layer layer)
{
    NetworkLayer *nl;
    nl = new NetworkLayer(layer);
    int insertedRow = m_networklayerList.count();

    beginInsertRows(QModelIndex(), insertedRow, insertedRow);
    m_networklayerList.append(nl);

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

    foreach (networkLayer, m_networklayerList) {
        layerList.append(networkLayer->layer());
    }

    return layerList;
}

QList<uint> NetworkLayerListModel::layerPDUSize()
{
    NetworkLayer *networkLayer;
    QList<uint> list;

    foreach (networkLayer, m_networklayerList) {
        list.append(networkLayer->PDUSize());
    }

    return list;
}

// Duplicates the current model. As we have complex Structure (and QOBJECT), we can't just copy it
// The caller must ensure the memory is cleared.
NetworkLayerListModel *NetworkLayerListModel::clone()
{
    NetworkLayerListModel *model = new NetworkLayerListModel();

    model->fillWithLayers(this->layerList());

    return model;
}

void NetworkLayerListModel::setUDPPDUSize(uint size)
{
    m_networklayerList[0]->setPDUSize(size);
}

