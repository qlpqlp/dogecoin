// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PRODUCTCATEGORYDIALOG_H
#define BITCOIN_QT_PRODUCTCATEGORYDIALOG_H

#include <QDialog>

class PointOfSaleDB;

namespace Ui {
    class ProductCategoryDialog;
}

class ProductCategoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProductCategoryDialog(PointOfSaleDB* db, qint64 categoryId = 0, QWidget *parent = nullptr);
    ~ProductCategoryDialog();

private slots:
    void accept() override;

private:
    Ui::ProductCategoryDialog *ui;
    PointOfSaleDB* m_db;
    qint64 m_categoryId;
};

#endif // BITCOIN_QT_PRODUCTCATEGORYDIALOG_H

