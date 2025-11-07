// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PRODUCTDIALOG_H
#define BITCOIN_QT_PRODUCTDIALOG_H

#include <QDialog>

class PointOfSaleDB;

namespace Ui {
    class ProductDialog;
}

class ProductDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProductDialog(PointOfSaleDB* db, qint64 productId = 0, qint64 categoryId = 0, QWidget *parent = nullptr);
    ~ProductDialog();

private slots:
    void accept() override;
    void onBrowseImage();

private:
    Ui::ProductDialog *ui;
    PointOfSaleDB* m_db;
    qint64 m_productId;
    qint64 m_categoryId;
    void loadCategories();
};

#endif // BITCOIN_QT_PRODUCTDIALOG_H

