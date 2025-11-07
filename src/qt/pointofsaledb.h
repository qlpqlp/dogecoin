// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_POINTOFSALEDB_H
#define BITCOIN_QT_POINTOFSALEDB_H

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class Category
{
public:
    qint64 id;
    QString name;
    qint64 timestamp;

    Category() : id(0), timestamp(0) {}
    Category(const QString& name) : id(0), name(name), timestamp(QDateTime::currentMSecsSinceEpoch()) {}
};

class Product
{
public:
    qint64 id;
    qint64 categoryId;
    QString name;
    QString description;
    QString imagePath;
    int quantity; // -1 for unlimited
    qint64 priceDoge; // Price in smallest unit (1 DOGE = 100000000)
    qint64 timestamp;
    qint64 updatedTimestamp;
    QString paymentAddress; // Generated payment address
    int requestedQuantity; // Quantity requested in current payment

    Product() : id(0), categoryId(0), quantity(0), priceDoge(0), timestamp(0), updatedTimestamp(0), requestedQuantity(0) {}
    
    bool isAvailable() const { return quantity == -1 || quantity > 0; }
};

class PointOfSaleDB : public QObject
{
    Q_OBJECT

public:
    explicit PointOfSaleDB(QObject *parent = nullptr);
    ~PointOfSaleDB();

    bool initialize();
    void close();

    // Category operations
    qint64 addCategory(const QString& name);
    bool updateCategory(qint64 id, const QString& name);
    bool deleteCategory(qint64 id);
    QList<Category> getAllCategories();
    Category getCategoryById(qint64 id);
    QList<qint64> getCategoriesWithAvailableProducts();

    // Product operations
    qint64 addProduct(qint64 categoryId, const QString& name, const QString& description,
                     const QString& imagePath, int quantity, qint64 priceDoge);
    bool updateProduct(qint64 id, qint64 categoryId, const QString& name, const QString& description,
                      const QString& imagePath, int quantity, qint64 priceDoge);
    bool updateProductQuantity(qint64 id, int quantity);
    bool updateProductPaymentAddress(qint64 id, const QString& address, int requestedQuantity);
    bool deleteProduct(qint64 id);
    QList<Product> getAllProducts();
    QList<Product> getAvailableProductsByCategory(qint64 categoryId);
    QList<Product> getAllProductsByCategory(qint64 categoryId);
    Product getProductById(qint64 id);
    Product getProductByPaymentAddress(const QString& address);

private:
    QSqlDatabase db;
    bool createTables();
    bool initialized;
};

#endif // BITCOIN_QT_POINTOFSALEDB_H

