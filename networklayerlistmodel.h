#ifndef NETWORKLAYERLISTMODEL_H
#define NETWORKLAYERLISTMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QList>
#include <QSettings>

#include "networklayer.h"

class NetworkLayerListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    NetworkLayerListModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Own methods
    void appendLayer(NetworkLayer::Layer layer);
    void removeLastLayer();
    NetworkLayer::Layer lastLayer();
    bool isLastLayer(const QModelIndex &index);
    NetworkLayer::Layer layerAt(const QModelIndex &index);
    void fillWithLayers(QList<NetworkLayer::Layer> layerList);
    QList<NetworkLayer::Layer> layerList();
    QList<uint> layerPDUSize();
    QList<QString> layerShortNameList();
    QList<bool> displayStatList();

    NetworkLayerListModel *clone();
    void setUDPPDUSize(uint size);
    void setPDUSize(const uint row, const uint size);

    // Saving/Loading Parameter
    void saveParameter(QSettings &settings);
    void loadParameter(QSettings &settings);

private:
    QList<NetworkLayer *> m_networklayerList;
    QList<bool> m_displayStatsList;

    enum networkLayerColumns {
        COL_NAME, // 0
        COL_DISPLAYSTAT,
        // COL_COUNT has to be the last enumerator, as it is the count of columns
        COL_COUNT
    };


};

#endif // NETWORKLAYERLISTMODEL_H
