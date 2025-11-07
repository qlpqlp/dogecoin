// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pointofsaledb.h"

#include "fs.h"
#include "util.h"

#include <QDateTime>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>

#include <fstream>

PointOfSaleDB::PointOfSaleDB(QObject *parent)
    : QObject(parent), initialized(false)
{
}

PointOfSaleDB::~PointOfSaleDB()
{
    close();
}

bool PointOfSaleDB::initialize()
{
    if (initialized)
        return true;

    // Get wallet data directory
    fs::path dataDir = GetDataDir();
    fs::path dbPath = dataDir / "pointofsale.db";

    // Ensure directory exists
    TryCreateDirectory(dataDir);

    db = QSqlDatabase::addDatabase("QSQLITE", "PointOfSaleDB");
    db.setDatabaseName(QString::fromStdString(dbPath.string()));

    if (!db.open()) {
        LogPrintf("PointOfSaleDB: Failed to open database: %s\n", db.lastError().text().toStdString());
        return false;
    }

    if (!createTables()) {
        LogPrintf("PointOfSaleDB: Failed to create tables\n");
        close();
        return false;
    }

    initialized = true;
    return true;
}

void PointOfSaleDB::close()
{
    if (db.isOpen()) {
        db.close();
    }
    initialized = false;
}

bool PointOfSaleDB::createTables()
{
    QSqlQuery query(db);

    // Categories table
    if (!query.exec("CREATE TABLE IF NOT EXISTS pos_categories ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT NOT NULL, "
                   "timestamp INTEGER NOT NULL)")) {
        LogPrintf("PointOfSaleDB: Failed to create categories table: %s\n", query.lastError().text().toStdString());
        return false;
    }

    // Products table
    if (!query.exec("CREATE TABLE IF NOT EXISTS pos_products ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "categoryId INTEGER NOT NULL, "
                   "name TEXT NOT NULL, "
                   "description TEXT, "
                   "imagePath TEXT, "
                   "quantity INTEGER NOT NULL DEFAULT 0, "
                   "priceDoge INTEGER NOT NULL, "
                   "timestamp INTEGER NOT NULL, "
                   "updatedTimestamp INTEGER NOT NULL, "
                   "paymentAddress TEXT, "
                   "requestedQuantity INTEGER DEFAULT 0, "
                   "FOREIGN KEY(categoryId) REFERENCES pos_categories(id) ON DELETE CASCADE)")) {
        LogPrintf("PointOfSaleDB: Failed to create products table: %s\n", query.lastError().text().toStdString());
        return false;
    }

    // Create index
    query.exec("CREATE INDEX IF NOT EXISTS idx_products_categoryId ON pos_products(categoryId)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_products_paymentAddress ON pos_products(paymentAddress)");

    return true;
}

qint64 PointOfSaleDB::addCategory(const QString& name)
{
    if (!initialized) return 0;

    QSqlQuery query(db);
    query.prepare("INSERT INTO pos_categories (name, timestamp) VALUES (?, ?)");
    query.addBindValue(name);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to add category: %s\n", query.lastError().text().toStdString());
        return 0;
    }

    return query.lastInsertId().toLongLong();
}

bool PointOfSaleDB::updateCategory(qint64 id, const QString& name)
{
    if (!initialized) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE pos_categories SET name = ? WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(id);

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to update category: %s\n", query.lastError().text().toStdString());
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool PointOfSaleDB::deleteCategory(qint64 id)
{
    if (!initialized) return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM pos_categories WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to delete category: %s\n", query.lastError().text().toStdString());
        return false;
    }

    return query.numRowsAffected() > 0;
}

QList<Category> PointOfSaleDB::getAllCategories()
{
    QList<Category> categories;

    if (!initialized) return categories;

    QSqlQuery query(db);
    query.prepare("SELECT id, name, timestamp FROM pos_categories ORDER BY name ASC");

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to get categories: %s\n", query.lastError().text().toStdString());
        return categories;
    }

    while (query.next()) {
        Category cat;
        cat.id = query.value(0).toLongLong();
        cat.name = query.value(1).toString();
        cat.timestamp = query.value(2).toLongLong();
        categories.append(cat);
    }

    return categories;
}

Category PointOfSaleDB::getCategoryById(qint64 id)
{
    Category cat;

    if (!initialized) return cat;

    QSqlQuery query(db);
    query.prepare("SELECT id, name, timestamp FROM pos_categories WHERE id = ?");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        cat.id = query.value(0).toLongLong();
        cat.name = query.value(1).toString();
        cat.timestamp = query.value(2).toLongLong();
    }

    return cat;
}

QList<qint64> PointOfSaleDB::getCategoriesWithAvailableProducts()
{
    QList<qint64> categoryIds;

    if (!initialized) return categoryIds;

    QSqlQuery query(db);
    query.prepare("SELECT DISTINCT categoryId FROM pos_products WHERE quantity = -1 OR quantity > 0");

    if (query.exec()) {
        while (query.next()) {
            categoryIds.append(query.value(0).toLongLong());
        }
    }

    return categoryIds;
}

qint64 PointOfSaleDB::addProduct(qint64 categoryId, const QString& name, const QString& description,
                                 const QString& imagePath, int quantity, qint64 priceDoge)
{
    if (!initialized) return 0;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QSqlQuery query(db);
    query.prepare("INSERT INTO pos_products (categoryId, name, description, imagePath, quantity, priceDoge, timestamp, updatedTimestamp) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(categoryId);
    query.addBindValue(name);
    query.addBindValue(description);
    query.addBindValue(imagePath);
    query.addBindValue(quantity);
    query.addBindValue(priceDoge);
    query.addBindValue(now);
    query.addBindValue(now);

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to add product: %s\n", query.lastError().text().toStdString());
        return 0;
    }

    return query.lastInsertId().toLongLong();
}

bool PointOfSaleDB::updateProduct(qint64 id, qint64 categoryId, const QString& name, const QString& description,
                                  const QString& imagePath, int quantity, qint64 priceDoge)
{
    if (!initialized) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE pos_products SET categoryId = ?, name = ?, description = ?, imagePath = ?, "
                 "quantity = ?, priceDoge = ?, updatedTimestamp = ? WHERE id = ?");
    query.addBindValue(categoryId);
    query.addBindValue(name);
    query.addBindValue(description);
    query.addBindValue(imagePath);
    query.addBindValue(quantity);
    query.addBindValue(priceDoge);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(id);

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to update product: %s\n", query.lastError().text().toStdString());
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool PointOfSaleDB::updateProductQuantity(qint64 id, int quantity)
{
    if (!initialized) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE pos_products SET quantity = ?, updatedTimestamp = ? WHERE id = ?");
    query.addBindValue(quantity);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(id);

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to update product quantity: %s\n", query.lastError().text().toStdString());
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool PointOfSaleDB::updateProductPaymentAddress(qint64 id, const QString& address, int requestedQuantity)
{
    if (!initialized) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE pos_products SET paymentAddress = ?, requestedQuantity = ? WHERE id = ?");
    query.addBindValue(address);
    query.addBindValue(requestedQuantity);
    query.addBindValue(id);

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to update product payment address: %s\n", query.lastError().text().toStdString());
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool PointOfSaleDB::deleteProduct(qint64 id)
{
    if (!initialized) return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM pos_products WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to delete product: %s\n", query.lastError().text().toStdString());
        return false;
    }

    return query.numRowsAffected() > 0;
}

QList<Product> PointOfSaleDB::getAllProducts()
{
    QList<Product> products;

    if (!initialized) return products;

    QSqlQuery query(db);
    query.prepare("SELECT id, categoryId, name, description, imagePath, quantity, priceDoge, "
                 "timestamp, updatedTimestamp, paymentAddress, requestedQuantity "
                 "FROM pos_products ORDER BY timestamp DESC");

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to get products: %s\n", query.lastError().text().toStdString());
        return products;
    }

    while (query.next()) {
        Product prod;
        prod.id = query.value(0).toLongLong();
        prod.categoryId = query.value(1).toLongLong();
        prod.name = query.value(2).toString();
        prod.description = query.value(3).toString();
        prod.imagePath = query.value(4).toString();
        prod.quantity = query.value(5).toInt();
        prod.priceDoge = query.value(6).toLongLong();
        prod.timestamp = query.value(7).toLongLong();
        prod.updatedTimestamp = query.value(8).toLongLong();
        prod.paymentAddress = query.value(9).toString();
        prod.requestedQuantity = query.value(10).toInt();
        products.append(prod);
    }

    return products;
}

QList<Product> PointOfSaleDB::getAvailableProductsByCategory(qint64 categoryId)
{
    QList<Product> products;

    if (!initialized) return products;

    QSqlQuery query(db);
    query.prepare("SELECT id, categoryId, name, description, imagePath, quantity, priceDoge, "
                 "timestamp, updatedTimestamp, paymentAddress, requestedQuantity "
                 "FROM pos_products WHERE categoryId = ? AND (quantity = -1 OR quantity > 0) ORDER BY name ASC");
    query.addBindValue(categoryId);

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to get available products: %s\n", query.lastError().text().toStdString());
        return products;
    }

    while (query.next()) {
        Product prod;
        prod.id = query.value(0).toLongLong();
        prod.categoryId = query.value(1).toLongLong();
        prod.name = query.value(2).toString();
        prod.description = query.value(3).toString();
        prod.imagePath = query.value(4).toString();
        prod.quantity = query.value(5).toInt();
        prod.priceDoge = query.value(6).toLongLong();
        prod.timestamp = query.value(7).toLongLong();
        prod.updatedTimestamp = query.value(8).toLongLong();
        prod.paymentAddress = query.value(9).toString();
        prod.requestedQuantity = query.value(10).toInt();
        products.append(prod);
    }

    return products;
}

QList<Product> PointOfSaleDB::getAllProductsByCategory(qint64 categoryId)
{
    QList<Product> products;

    if (!initialized) return products;

    QSqlQuery query(db);
    query.prepare("SELECT id, categoryId, name, description, imagePath, quantity, priceDoge, "
                 "timestamp, updatedTimestamp, paymentAddress, requestedQuantity "
                 "FROM pos_products WHERE categoryId = ? ORDER BY name ASC");
    query.addBindValue(categoryId);

    if (!query.exec()) {
        LogPrintf("PointOfSaleDB: Failed to get products: %s\n", query.lastError().text().toStdString());
        return products;
    }

    while (query.next()) {
        Product prod;
        prod.id = query.value(0).toLongLong();
        prod.categoryId = query.value(1).toLongLong();
        prod.name = query.value(2).toString();
        prod.description = query.value(3).toString();
        prod.imagePath = query.value(4).toString();
        prod.quantity = query.value(5).toInt();
        prod.priceDoge = query.value(6).toLongLong();
        prod.timestamp = query.value(7).toLongLong();
        prod.updatedTimestamp = query.value(8).toLongLong();
        prod.paymentAddress = query.value(9).toString();
        prod.requestedQuantity = query.value(10).toInt();
        products.append(prod);
    }

    return products;
}

Product PointOfSaleDB::getProductById(qint64 id)
{
    Product prod;

    if (!initialized) return prod;

    QSqlQuery query(db);
    query.prepare("SELECT id, categoryId, name, description, imagePath, quantity, priceDoge, "
                 "timestamp, updatedTimestamp, paymentAddress, requestedQuantity "
                 "FROM pos_products WHERE id = ?");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        prod.id = query.value(0).toLongLong();
        prod.categoryId = query.value(1).toLongLong();
        prod.name = query.value(2).toString();
        prod.description = query.value(3).toString();
        prod.imagePath = query.value(4).toString();
        prod.quantity = query.value(5).toInt();
        prod.priceDoge = query.value(6).toLongLong();
        prod.timestamp = query.value(7).toLongLong();
        prod.updatedTimestamp = query.value(8).toLongLong();
        prod.paymentAddress = query.value(9).toString();
        prod.requestedQuantity = query.value(10).toInt();
    }

    return prod;
}

Product PointOfSaleDB::getProductByPaymentAddress(const QString& address)
{
    Product prod;

    if (!initialized || address.isEmpty()) return prod;

    QSqlQuery query(db);
    query.prepare("SELECT id, categoryId, name, description, imagePath, quantity, priceDoge, "
                 "timestamp, updatedTimestamp, paymentAddress, requestedQuantity "
                 "FROM pos_products WHERE paymentAddress = ?");
    query.addBindValue(address);

    if (query.exec() && query.next()) {
        prod.id = query.value(0).toLongLong();
        prod.categoryId = query.value(1).toLongLong();
        prod.name = query.value(2).toString();
        prod.description = query.value(3).toString();
        prod.imagePath = query.value(4).toString();
        prod.quantity = query.value(5).toInt();
        prod.priceDoge = query.value(6).toLongLong();
        prod.timestamp = query.value(7).toLongLong();
        prod.updatedTimestamp = query.value(8).toLongLong();
        prod.paymentAddress = query.value(9).toString();
        prod.requestedQuantity = query.value(10).toInt();
    }

    return prod;
}

