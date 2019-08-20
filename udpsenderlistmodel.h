#ifndef UDPSENDERLISTMODEL_H
#define UDPSENDERLISTMODEL_H

#include <QAbstractTableModel>
#include <QList>

#include "udpsender.h"
#include "networkmodel.h"

class UdpSenderListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit UdpSenderListModel(QObject *parent = 0);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());

    void setPDUSizeLayer(NetworkModel::Layer layer);
    void setBandwidthLayer(NetworkModel::Layer layer);
    void setBandwidthUnit(int bandwidthUnit);

    void setWsClient(WsClient *wsClient);

    void stopAllSender();

public slots:
    // Connected to the UdpSender::dataChanged, so that an update of the UI occurs.
    // It will start sending Traffic when all udpSender are connected
    void connectionStatusChanged();

    void updateStats();

private:
    QList<UdpSender *> udpSenderList;

    enum udpSenderColumns {
        COL_NAME, // 0
        COL_PORT,
        COL_BANDWIDTH,
        COL_DSCP,
        COL_SIZE,
        COL_STATUS,
        /* Statistics */
        COL_SENDINGRATE,
        COL_RECEIVINGRATE,
        COL_PACKETLOST,
        // COL_COUNT has to be the last enumerator, as it is the count of columns
        COL_COUNT
    };

    NetworkModel m_networkModel;

    /* Diese Werte aus der UI sind wichtig, da der Modell muss wissen, in welchen Layer oder Einheit die Daten angegeben werden */
    NetworkModel::Layer m_PDUSizeLayer;
    NetworkModel::Layer m_BandwidthLayer;
    int m_BandwidthUnit;

    // Communication with remote
    WsClient *m_wsClient = NULL;
};

#endif // UDPSENDERLISTMODEL_H
