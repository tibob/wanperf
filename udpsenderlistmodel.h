#ifndef UDPSENDERLISTMODEL_H
#define UDPSENDERLISTMODEL_H

#include <QAbstractTableModel>
#include <QList>

#include "udpsender.h"
#include "networkmodel.h"
#include "networklayerlistmodel.h"

class UdpSenderListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit UdpSenderListModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;

    // Layer related functions
    void setWANLayerModel(NetworkLayerListModel *WANmodel);

    void setPDUSizeLayer(NetworkModel::Layer layer);
    void setBandwidthLayer(NetworkModel::Layer layer);
    void setBandwidthUnit(int bandwidthUnit);

    void generateTraffic();
    void stopAllSender();

    void setDestinationIP(QHostAddress destinationIP);

    // Total Statistics are displayed in MainWindow and can not be private
    qreal totalSpecifiedBandwidth(NetworkModel::Layer layer);
    QString totalSendingStats();
    QString totalReceivingStats();
    QString totalPacketsStats();
    QString WANtotalReceivingStats();
    QString WANtotalSendingStats();

private:
    QString WANSendingStats(const QModelIndex &index) const;
    QString WANReceivingStats(const QModelIndex &index) const;


public slots:
    void updateStats();
    void WANLayerModelChanged();

private:
    QList<UdpSender *> m_udpSenderList;

    enum udpSenderColumns {
        COL_NAME, // 0
        COL_PORT,
        COL_BANDWIDTH,
        COL_DSCP,
        COL_SIZE,
        COL_TC,
        /* Statistics */
        COL_SENDINGSTATS,
        COL_RECEIVINGSTATS,
        COL_SENDINGPACKETS,
        COL_RECEIVINGPACKETS,
        COL_WANSENDINGSTATS,
        COL_WANRECEIVINGSTATS,
        // COL_COUNT has to be the last enumerator, as it is the count of columns
        COL_COUNT
    };

    /* Diese Werte aus der UI sind wichtig, da der Modell muss wissen, in welchen Layer oder Einheit die Daten angegeben werden */
    NetworkModel::Layer m_PDUSizeLayer;
    NetworkModel::Layer m_BandwidthLayer;
    int m_BandwidthUnit;

    NetworkLayerListModel *m_WANLayerModel = NULL;

    bool m_isGeneratingTraffic = false;

    QHostAddress m_destination;
};

#endif // UDPSENDERLISTMODEL_H
