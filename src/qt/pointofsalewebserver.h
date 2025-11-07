// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_POINTOFSALEWEBSERVER_H
#define BITCOIN_QT_POINTOFSALEWEBSERVER_H

#include "pointofsaledb.h"

#include "amount.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QMutex>
#include <QTimer>

class WalletModel;
class PointOfSaleDB;
class TransactionTableModel;

class PointOfSaleWebServer : public QObject
{
    Q_OBJECT

public:
    explicit PointOfSaleWebServer(PointOfSaleDB* db, WalletModel* walletModel, QObject *parent = nullptr);
    ~PointOfSaleWebServer();

    bool start(quint16 port = 4200);
    void stop();
    bool isRunning() const { return m_running; }
    quint16 getPort() const { return m_port; }

private slots:
    void handleNewConnection();
    void handleClientReadyRead();
    void handleClientDisconnected();
    void checkForPayments(); // Monitor wallet for payments

private:
    void processRequest(QTcpSocket* client, const QByteArray& request);
    QString handleHomePage();
    QString handleCategoryPage(qint64 categoryId);
    QString handlePaymentPage(qint64 productId, int quantity);
    QString handleApiRequest(const QString& path, const QString& method, const QMap<QString, QString>& params);
    
    QString generatePaymentAddress(qint64 productId, int quantity);
    QString getQrCodeDataUrl(const QString& address, qint64 amount);
    QString getImageDataUrl(const QString& imagePath);
    QString getHtmlHeader(const QString& title);
    QString getCss();
    QString escapeHtml(const QString& text);
    QString createJsonResponse(const QString& json);
    QMap<QString, QString> parseQueryString(const QString& query);
    
    QString generateHtmlContent();

    QTcpServer* m_server;
    PointOfSaleDB* m_db;
    WalletModel* m_walletModel;
    TransactionTableModel* m_transactionModel;
    QList<QTcpSocket*> m_clients;
    QMutex m_clientsMutex;
    bool m_running;
    quint16 m_port;
    QTimer* m_paymentCheckTimer;
    
    // Track pending payments: address -> productId, quantity, totalPrice, paid
    QMap<QString, QPair<qint64, QPair<int, QPair<qint64, bool>>>> m_pendingPayments;
    QMutex m_pendingPaymentsMutex;
    
    void processPaymentForAddress(const QString& address, const CAmount& amount);
};

#endif // BITCOIN_QT_POINTOFSALEWEBSERVER_H

