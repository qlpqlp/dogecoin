// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_POINTOFSALEPAGE_H
#define BITCOIN_QT_POINTOFSALEPAGE_H

#include "amount.h"

#include <QWidget>
#include <QList>
#include <memory>

class ClientModel;
class PlatformStyle;
class PointOfSaleDB;
class PointOfSaleWebServer;
class WalletModel;

namespace Ui {
    class PointOfSalePage;
}

/** Point of Sale page widget */
class PointOfSalePage : public QWidget
{
    Q_OBJECT

public:
    explicit PointOfSalePage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~PointOfSalePage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);

private:
    void refreshCategories();
    void refreshProducts();

private:
    Ui::PointOfSalePage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    const PlatformStyle *platformStyle;
    PointOfSaleDB* m_db;
    PointOfSaleWebServer* m_webServer;

private Q_SLOTS:
    void updateDisplayUnit();
    void onAddCategory();
    void onAddProduct();
    void onEditCategory();
    void onEditProduct();
    void onDeleteCategory();
    void onDeleteProduct();
    void onStartWebServer();
    void onStopWebServer();
};

#endif // BITCOIN_QT_POINTOFSALEPAGE_H

