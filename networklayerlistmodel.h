#ifndef NETWORKLAYERLISTMODEL_H
#define NETWORKLAYERLISTMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QList>

#include "networklayer.h"

class NetworkLayerListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    NetworkLayerListModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void appendLayer(NetworkLayer::Layer layer);
    void removeLastLayer();
    NetworkLayer::Layer lastLayer();
    NetworkLayer::Layer layerAt(const QModelIndex &index);
    void fillWithLayers(QList<NetworkLayer::Layer> layerList);
    QList<NetworkLayer::Layer> layerList();
    QList<uint> layerPDUSize();
    QList<QString> layerShortNameList();

    NetworkLayerListModel *clone();
    void setUDPPDUSize(uint size);
    void setPDUSize(const uint row, const uint size);


private:
    QList<NetworkLayer *> m_networklayerList;

    enum networkLayerColumns {
        COL_NAME, // 0
        COL_DISPLAY,
        // COL_COUNT has to be the last enumerator, as it is the count of columns
        COL_COUNT
    };


};

#endif // NETWORKLAYERLISTMODEL_H
