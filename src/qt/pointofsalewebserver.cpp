// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pointofsalewebserver.h"

#include "addresstablemodel.h"
#include "amount.h"
#include "base58.h"
#include "bitcoinunits.h"
#include "config/bitcoin-config.h"
#include "guiutil.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "util.h"

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

#include <QTimer>
#include "wallet/wallet.h"

#include <QByteArray>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QPainter>
#include <QRegExp>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

#include "wallet/wallet.h"

static const int MAX_URI_LENGTH = 255;
static const int QR_IMAGE_SIZE = 300;

PointOfSaleWebServer::PointOfSaleWebServer(PointOfSaleDB* db, WalletModel* walletModel, QObject *parent)
    : QObject(parent),
      m_server(new QTcpServer(this)),
      m_db(db),
      m_walletModel(walletModel),
      m_transactionModel(nullptr),
      m_running(false),
      m_port(4200),
      m_paymentCheckTimer(new QTimer(this))
{
    connect(m_server, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));
    
    if (m_walletModel) {
        m_transactionModel = m_walletModel->getTransactionTableModel();
        // Check for payments every 3 seconds
        connect(m_paymentCheckTimer, SIGNAL(timeout()), this, SLOT(checkForPayments()));
        m_paymentCheckTimer->start(3000);
    }
}

PointOfSaleWebServer::~PointOfSaleWebServer()
{
    stop();
}

bool PointOfSaleWebServer::start(quint16 port)
{
    if (m_running) {
        return false;
    }

    m_port = port;
    if (!m_server->listen(QHostAddress::Any, port)) {
        LogPrintf("PointOfSaleWebServer: Failed to start server on port %d: %s\n", port, m_server->errorString().toStdString());
        return false;
    }

    m_running = true;
    LogPrintf("PointOfSaleWebServer: Started on port %d\n", port);
    return true;
}

void PointOfSaleWebServer::stop()
{
    if (!m_running) {
        return;
    }

    m_server->close();
    m_paymentCheckTimer->stop();
    
    QMutexLocker locker(&m_clientsMutex);
    foreach (QTcpSocket* client, m_clients) {
        client->close();
        client->deleteLater();
    }
    m_clients.clear();
    
    m_running = false;
    LogPrintf("PointOfSaleWebServer: Stopped\n");
}

void PointOfSaleWebServer::handleNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* client = m_server->nextPendingConnection();
        QMutexLocker locker(&m_clientsMutex);
        m_clients.append(client);
        
        connect(client, SIGNAL(readyRead()), this, SLOT(handleClientReadyRead()));
        connect(client, SIGNAL(disconnected()), this, SLOT(handleClientDisconnected()));
    }
}

void PointOfSaleWebServer::handleClientReadyRead()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray requestData = client->readAll();
    processRequest(client, requestData);
}

void PointOfSaleWebServer::handleClientDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        QMutexLocker locker(&m_clientsMutex);
        m_clients.removeAll(client);
        client->deleteLater();
    }
}

void PointOfSaleWebServer::processRequest(QTcpSocket* client, const QByteArray& request)
{
    QString requestStr = QString::fromUtf8(request);
    QStringList lines = requestStr.split("\r\n");
    
    if (lines.isEmpty()) {
        client->close();
        return;
    }

    QString requestLine = lines[0];
    QStringList parts = requestLine.split(" ");
    if (parts.size() < 2) {
        client->close();
        return;
    }

    QString method = parts[0];
    QString path = parts[1];
    QString query;

    int queryIndex = path.indexOf("?");
    if (queryIndex != -1) {
        query = path.mid(queryIndex + 1);
        path = path.left(queryIndex);
    }

    LogPrintf("PointOfSaleWebServer: %s %s\n", method.toStdString(), path.toStdString());

    QString response;
    
    if (path == "/" || path == "/index.html") {
        response = handleHomePage();
    } else if (path.startsWith("/category/")) {
        QString categoryIdStr = path.mid(QString("/category/").length());
        bool ok;
        qint64 categoryId = categoryIdStr.toLongLong(&ok);
        if (ok) {
            response = handleCategoryPage(categoryId);
        } else {
            response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n400 Bad Request";
        }
    } else if (path.startsWith("/pay/")) {
        QString productIdStr = path.mid(QString("/pay/").length());
        bool ok;
        qint64 productId = productIdStr.toLongLong(&ok);
        if (ok) {
            QMap<QString, QString> params = parseQueryString(query);
            int quantity = params.value("quantity", "1").toInt();
            if (quantity < 1) quantity = 1;
            response = handlePaymentPage(productId, quantity);
        } else {
            response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n400 Bad Request";
        }
    } else if (path.startsWith("/api/")) {
        response = handleApiRequest(path, method, parseQueryString(query));
    } else {
        response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n404 Not Found";
    }

    client->write(response.toUtf8());
    client->flush();
    client->close();
}

QString PointOfSaleWebServer::handleHomePage()
{
    QList<Category> categories = m_db->getAllCategories();
    QList<qint64> categoriesWithProducts = m_db->getCategoriesWithAvailableProducts();

    QString html;
    html += "HTTP/1.1 200 OK\r\n";
    html += "Content-Type: text/html; charset=utf-8\r\n\r\n";
    html += getHtmlHeader(tr("Point of Sale - Categories"));
    html += "<body>";
    html += "<div class='container'>";
    html += "<h1>" + tr("Point of Sale") + "</h1>";
    html += "<div class='categories'>";

    foreach (const Category& category, categories) {
        if (categoriesWithProducts.contains(category.id)) {
            html += "<a href='/category/" + QString::number(category.id) + "' class='category-card'>";
            html += "<h2>" + escapeHtml(category.name) + "</h2>";
            html += "</a>";
        }
    }

    html += "</div>";
    html += "</div>";
    html += getCss();
    html += "</body></html>";

    return html;
}

QString PointOfSaleWebServer::handleCategoryPage(qint64 categoryId)
{
    Category category = m_db->getCategoryById(categoryId);
    if (category.id == 0) {
        return "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n404 Category Not Found";
    }

    QList<Product> products = m_db->getAvailableProductsByCategory(categoryId);

    QString html;
    html += "HTTP/1.1 200 OK\r\n";
    html += "Content-Type: text/html; charset=utf-8\r\n\r\n";
    html += getHtmlHeader(tr("Category: %1").arg(category.name));
    html += "<body>";
    html += "<div class='container'>";
    html += "<a href='/' class='back-link'>← " + tr("Back to Categories") + "</a>";
    html += "<h1>" + escapeHtml(category.name) + "</h1>";
    html += "<div class='products'>";

    foreach (const Product& product, products) {
        html += "<div class='product-card'>";
        
        if (!product.imagePath.isEmpty()) {
            QString imageDataUrl = getImageDataUrl(product.imagePath);
            if (!imageDataUrl.isEmpty()) {
                html += "<img src='" + imageDataUrl + "' alt='" + escapeHtml(product.name) + "' />";
            }
        }
        
        html += "<div class='product-info'>";
        html += "<h3>" + escapeHtml(product.name) + "</h3>";
        if (!product.description.isEmpty()) {
            html += "<p>" + escapeHtml(product.description) + "</p>";
        }
        
        html += "<div class='product-price-qty'>";
        html += "<div class='price'>" + BitcoinUnits::format(BitcoinUnits::BTC, product.priceDoge, false, BitcoinUnits::separatorNever) + " DOGE</div>";
        if (product.quantity == -1) {
            html += "<div class='quantity'>" + tr("Stock: Unlimited") + "</div>";
        } else {
            html += "<div class='quantity'>" + tr("Stock: %1").arg(product.quantity) + "</div>";
        }
        html += "</div>";
        
        html += "<div class='product-quantity-pay'>";
        html += "<div class='product-quantity-selector'>";
        html += "<div class='quantity-controls'>";
        html += "<button type='button' onclick='decreaseQty(" + QString::number(product.id) + ")' class='qty-btn qty-minus'>-</button>";
        int maxQty = product.quantity == -1 ? 999999 : product.quantity;
        html += "<input type='number' id='qty_" + QString::number(product.id) + "' min='1' max='" + QString::number(maxQty) + "' value='1' class='quantity-input' />";
        html += "<button type='button' onclick='increaseQty(" + QString::number(product.id) + ")' class='qty-btn qty-plus'>+</button>";
        html += "</div>";
        html += "</div>";
        html += "<button onclick='payProduct(" + QString::number(product.id) + ")' class='btn-pay-doge'>" + tr("Pay In Doge") + "</button>";
        html += "</div>";
        
        html += "</div>"; // product-info
        html += "</div>"; // product-card
    }

    html += "<script>";
    html += "function increaseQty(productId) {";
    html += "  var qtyInput = document.getElementById('qty_' + productId);";
    html += "  if (qtyInput) {";
    html += "    var current = parseInt(qtyInput.value) || 1;";
    html += "    var max = parseInt(qtyInput.max) || 999999;";
    html += "    if (current < max) qtyInput.value = current + 1;";
    html += "  }";
    html += "}";
    html += "function decreaseQty(productId) {";
    html += "  var qtyInput = document.getElementById('qty_' + productId);";
    html += "  if (qtyInput) {";
    html += "    var current = parseInt(qtyInput.value) || 1;";
    html += "    var min = parseInt(qtyInput.min) || 1;";
    html += "    if (current > min) qtyInput.value = current - 1;";
    html += "  }";
    html += "}";
    html += "function payProduct(productId) {";
    html += "  var qtyInput = document.getElementById('qty_' + productId);";
    html += "  var qty = qtyInput ? qtyInput.value : 1;";
    html += "  if (qty < 1) qty = 1;";
    html += "  window.location.href = '/pay/' + productId + '?quantity=' + qty;";
    html += "}";
    html += "</script>";
    
    html += "</div>";
    html += "</div>";
    html += getCss();
    html += "</body></html>";

    return html;
}

QString PointOfSaleWebServer::handlePaymentPage(qint64 productId, int quantity)
{
    Product product = m_db->getProductById(productId);
    if (product.id == 0 || !product.isAvailable()) {
        return "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n404 Product Not Found or Out of Stock";
    }

    if (quantity < 1) quantity = 1;
    if (product.quantity != -1 && quantity > product.quantity) {
        quantity = product.quantity;
    }

    qint64 totalPrice = product.priceDoge * quantity;
    QString paymentAddress = generatePaymentAddress(productId, quantity);

    QString html;
    html += "HTTP/1.1 200 OK\r\n";
    html += "Content-Type: text/html; charset=utf-8\r\n\r\n";
    html += getHtmlHeader(tr("Payment: %1").arg(product.name));
    html += "<body>";
    html += "<div class='container payment-page'>";
    
    html += "<div class='payment-content'>";
    html += "<div class='payment-header'>";
    html += "<a href='/category/" + QString::number(product.categoryId) + "' class='back-button-payment'>← " + tr("Back") + "</a>";
    html += "<h1>" + escapeHtml(product.name) + "</h1>";
    html += "</div>";
    
    html += "<div id='payment-section' class='payment-section-simple'>";
    html += "<div id='qr-code-container' class='qr-container-large'>";
    QString qrDataUrl = getQrCodeDataUrl(paymentAddress, totalPrice);
    if (!qrDataUrl.isEmpty()) {
        html += "<img src='" + qrDataUrl + "' alt='QR Code' id='qr-code' />";
    } else {
        html += "<div class='qr-error'>" + tr("QR Code generation failed. Please try refreshing the page.") + "</div>";
    }
    html += "</div>";
    
    html += "<div class='payment-details-below-qr'>";
    html += "<div class='payment-detail-item-inline'>";
    html += "<span class='detail-label'>" + tr("Quantity:") + "</span>";
    html += "<span class='detail-value'>" + QString::number(quantity) + "</span>";
    html += "</div>";
    html += "<div class='payment-detail-item-inline'>";
    html += "<span class='detail-label'>" + tr("Amount to Pay:") + "</span>";
    html += "<span class='detail-value total-amount'>" + BitcoinUnits::format(BitcoinUnits::BTC, totalPrice, false, BitcoinUnits::separatorNever) + " DOGE</span>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='payment-address-simple'>";
    html += "<code id='payment-address-code'>" + paymentAddress + "</code>";
    html += "<button onclick='copyAddress()' class='btn-copy'>" + tr("Copy Address") + "</button>";
    html += "</div>";
    html += "</div>";
    
    html += "<div id='payment-success' class='payment-success' style='display:none;'>";
    html += "<div class='success-icon'>✓</div>";
    html += "<h2 class='success-title'>" + tr("Payment Successful!") + "</h2>";
    html += "<p class='success-message'>" + tr("Thank you for your purchase. Your payment has been received and confirmed.") + "</p>";
    html += "</div>";
    
    html += "</div>"; // payment-content
    html += "</div>"; // payment-page
    
    html += "<script>";
    html += "var paymentAddress = '" + paymentAddress + "';";
    html += "function copyAddress() {";
    html += "  navigator.clipboard.writeText(paymentAddress).then(function() {";
    html += "    alert('" + tr("Address copied to clipboard!") + "');";
    html += "  }).catch(function() {";
    html += "    alert('" + tr("Failed to copy address") + "');";
    html += "  });";
    html += "}";
    html += "var paymentCheckInterval;";
    html += "function checkPayment() {";
    html += "  fetch('/api/payment-status?address=' + encodeURIComponent(paymentAddress))";
    html += "    .then(function(response) { return response.json(); })";
    html += "    .then(function(data) {";
    html += "      if (data.paid) {";
    html += "        clearInterval(paymentCheckInterval);";
    html += "        document.getElementById('payment-section').style.display = 'none';";
    html += "        document.getElementById('payment-success').style.display = 'block';";
    html += "        setTimeout(function() { window.location.href = '/'; }, 5000);";
    html += "      }";
    html += "    })";
    html += "    .catch(function(error) {";
    html += "      console.error('Error checking payment:', error);";
    html += "    });";
    html += "}";
    html += "paymentCheckInterval = setInterval(checkPayment, 3000);";
    html += "checkPayment();";
    html += "</script>";
    
    html += getCss();
    html += "</body></html>";

    return html;
}

QString PointOfSaleWebServer::handleApiRequest(const QString& path, const QString& method, const QMap<QString, QString>& params)
{
    if (path.startsWith("/api/payment-status")) {
        QString address = params.value("address");
        QMutexLocker locker(&m_pendingPaymentsMutex);
        bool paid = false;
        if (m_pendingPayments.contains(address)) {
            paid = m_pendingPayments[address].second.second.second; // Get the paid flag
        }
        
        QJsonObject json;
        json["paid"] = paid;
        QJsonDocument doc(json);
        
        return createJsonResponse(QString::fromUtf8(doc.toJson()));
    }
    
    return "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Not Found\"}";
}

QString PointOfSaleWebServer::generatePaymentAddress(qint64 productId, int quantity)
{
    if (!m_walletModel) {
        return QString();
    }

    // Check if we already have a payment address for this product/quantity
    Product product = m_db->getProductById(productId);
    if (product.id != 0 && !product.paymentAddress.isEmpty() && product.requestedQuantity == quantity) {
        QMutexLocker locker(&m_pendingPaymentsMutex);
        m_pendingPayments[product.paymentAddress] = qMakePair(productId, qMakePair(quantity, product.priceDoge * quantity));
        return product.paymentAddress;
    }

    // Generate new address using AddressTableModel
    AddressTableModel* addressTableModel = m_walletModel->getAddressTableModel();
    if (!addressTableModel) {
        LogPrintf("PointOfSaleWebServer: Failed to get address table model\n");
        return QString();
    }

    QString address = addressTableModel->addRow(AddressTableModel::Receive, tr("Point of Sale"), "");
    if (address.isEmpty()) {
        LogPrintf("PointOfSaleWebServer: Failed to generate address\n");
        return QString();
    }

    // Save to database
    m_db->updateProductPaymentAddress(productId, address, quantity);

    // Track payment (productId, quantity, totalPrice, paid=false)
    QMutexLocker locker(&m_pendingPaymentsMutex);
    m_pendingPayments[address] = qMakePair(productId, qMakePair(quantity, qMakePair(product.priceDoge * quantity, false)));

    return address;
}

QString PointOfSaleWebServer::getQrCodeDataUrl(const QString& address, qint64 amount)
{
#ifdef USE_QRCODE
    QString uri = "dogecoin:" + address + "?amount=" + BitcoinUnits::format(BitcoinUnits::BTC, amount, false, BitcoinUnits::separatorNever);
    
    if (uri.length() > MAX_URI_LENGTH) {
        LogPrintf("PointOfSaleWebServer: URI too long for QR code\n");
        return QString();
    }

    QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!code) {
        LogPrintf("PointOfSaleWebServer: Failed to encode QR code\n");
        return QString();
    }

    QImage qrImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
    qrImage.fill(0xffffff);
    unsigned char *p = code->data;
    for (int y = 0; y < code->width; y++) {
        for (int x = 0; x < code->width; x++) {
            qrImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
            p++;
        }
    }
    QRcode_free(code);

    // Convert to base64 data URL
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    qrImage.save(&buffer, "PNG");
    QString base64 = ba.toBase64();
    
    return "data:image/png;base64," + base64;
#else
    return QString();
#endif
}

QString PointOfSaleWebServer::getImageDataUrl(const QString& imagePath)
{
    if (imagePath.isEmpty()) {
        return QString();
    }

    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QByteArray imageData = file.readAll();
    file.close();

    QFileInfo fileInfo(imagePath);
    QString mimeType = "image/jpeg";
    if (fileInfo.suffix().toLower() == "png") {
        mimeType = "image/png";
    }

    QString base64 = QString::fromLatin1(imageData.toBase64());
    return "data:" + mimeType + ";base64," + base64;
}

QString PointOfSaleWebServer::getHtmlHeader(const QString& title)
{
    return "<!DOCTYPE html><html><head>" +
           "<meta charset='utf-8'>" +
           "<meta name='viewport' content='width=device-width, initial-scale=1'>" +
           "<title>" + escapeHtml(title) + "</title>" +
           "<link href='https://fonts.googleapis.com/css2?family=Comic+Neue:wght@300;400;500;600;700&display=swap' rel='stylesheet'>" +
           "<style>" + getCss() + "</style>" +
           "</head>";
}

QString PointOfSaleWebServer::getCss()
{
    // Return the CSS from the Android implementation, simplified for C++ string
    return "* { margin: 0; padding: 0; box-sizing: border-box; } " +
           "body { font-family: 'Comic Neue', 'Arial', sans-serif; margin: 0; padding: 0; background: #0a0a0a; color: #ffffff; line-height: 1.6; min-height: 100vh; } " +
           ".container { max-width: 1400px; margin: 0 auto; padding: 15px; width: 100%; } " +
           "h1 { color: #ffc107; font-size: clamp(1.5em, 3vw, 2.2em); margin-bottom: 15px; text-align: center; text-shadow: 0 0 20px rgba(255, 193, 7, 0.3); } " +
           ".categories, .products { display: grid; grid-template-columns: repeat(auto-fill, minmax(280px, 1fr)); gap: 24px; margin-top: 30px; } " +
           ".category-card, .product-card { display: block; padding: 30px; background: linear-gradient(145deg, #1e1e1e, #252525); border: 1px solid #333333; border-radius: 16px; text-decoration: none; color: inherit; transition: all 0.3s; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.4); } " +
           ".category-card:hover, .product-card:hover { transform: translateY(-5px); box-shadow: 0 12px 40px rgba(255, 193, 7, 0.2); border-color: #ffc107; } " +
           ".category-card h2 { color: #ffc107; margin: 0; font-size: 1.5em; } " +
           ".product-card img { width: 100%; height: 220px; object-fit: cover; border-radius: 12px; margin-bottom: 15px; } " +
           ".product-info h3 { color: #ffffff; margin: 0 0 10px 0; font-size: 1.3em; } " +
           ".product-price-qty { display: flex; flex-direction: column; gap: 8px; margin-top: 15px; padding: 12px; background: rgba(0, 0, 0, 0.3); border-radius: 8px; } " +
           ".price { font-size: 1.6em; font-weight: bold; color: #ffc107; } " +
           ".quantity { color: #b0b0b0; } " +
           ".back-link { display: inline-block; margin-bottom: 10px; color: #ffc107; text-decoration: none; padding: 8px 16px; border-radius: 8px; background: rgba(255, 193, 7, 0.1); } " +
           ".payment-page { display: flex; flex-direction: column; padding: 20px; justify-content: center; align-items: center; } " +
           ".payment-section-simple { background: linear-gradient(145deg, #1e1e1e, #252525); padding: 25px; border-radius: 16px; border: 2px solid #ffc107; } " +
           ".qr-container-large { display: flex; align-items: center; justify-content: center; padding: 20px; } " +
           ".qr-container-large img { min-width: 220px; min-height: 220px; padding: 20px; background: #ffffff; border-radius: 12px; } " +
           ".payment-details-below-qr { display: flex; gap: 12px; justify-content: center; padding: 10px; } " +
           ".payment-detail-item-inline { display: flex; flex-direction: column; gap: 5px; text-align: center; } " +
           ".detail-label { color: #b0b0b0; } " +
           ".detail-value { color: #ffffff; font-weight: bold; } " +
           ".total-amount { color: #ffc107; font-size: 1.4em; } " +
           ".payment-address-simple { text-align: center; } " +
           ".payment-address-simple code { display: block; color: #ffffff; background: rgba(0, 0, 0, 0.5); padding: 15px; border-radius: 8px; word-break: break-all; font-family: monospace; margin-bottom: 15px; } " +
           ".btn-copy { padding: 12px 30px; background: linear-gradient(135deg, #ffc107, #ff8f00); color: #000000; border: none; border-radius: 8px; cursor: pointer; font-weight: bold; } " +
           ".payment-success { text-align: center; padding: 60px 40px; background: linear-gradient(135deg, #4CAF50, #45a049); border-radius: 16px; } " +
           ".success-icon { font-size: 4em; color: white; margin-bottom: 20px; } " +
           ".success-title { color: white; font-size: 2em; margin-bottom: 20px; } " +
           ".btn-pay-doge { padding: 12px 24px; background: linear-gradient(135deg, #ffc107, #ff8f00); color: #000000; border: none; border-radius: 8px; cursor: pointer; font-weight: bold; width: 100%; } " +
           ".quantity-controls { display: flex; align-items: center; gap: 8px; justify-content: center; } " +
           ".qty-btn { width: 44px; height: 44px; background: linear-gradient(135deg, #ffc107, #ff8f00); color: #000000; border: none; border-radius: 8px; cursor: pointer; font-weight: bold; font-size: 20px; } " +
           ".quantity-input { width: 80px; padding: 10px; border: 2px solid #333333; border-radius: 8px; background: rgba(0, 0, 0, 0.5); color: #ffffff; text-align: center; } ";
}

QString PointOfSaleWebServer::escapeHtml(const QString& text)
{
    QString escaped = text;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    escaped.replace("'", "&#39;");
    return escaped;
}

QString PointOfSaleWebServer::createJsonResponse(const QString& json)
{
    return "HTTP/1.1 200 OK\r\n" +
           "Content-Type: application/json; charset=utf-8\r\n" +
           "Access-Control-Allow-Origin: *\r\n\r\n" +
           json;
}

QMap<QString, QString> PointOfSaleWebServer::parseQueryString(const QString& query)
{
    QMap<QString, QString> params;
    if (query.isEmpty()) {
        return params;
    }

    QStringList pairs = query.split("&");
    foreach (const QString& pair, pairs) {
        QStringList keyValue = pair.split("=");
        if (keyValue.size() == 2) {
            QString key = QUrl::fromPercentEncoding(keyValue[0].toUtf8());
            QString value = QUrl::fromPercentEncoding(keyValue[1].toUtf8());
            params[key] = value;
        }
    }

    return params;
}

void PointOfSaleWebServer::checkForPayments()
{
    if (!m_transactionModel || !m_walletModel || !m_db) {
        return;
    }

    // Get list of addresses to monitor
    QMutexLocker locker(&m_pendingPaymentsMutex);
    QList<QString> addressesToCheck;
    QMap<QString, QPair<qint64, QPair<int, QPair<qint64, bool>>>>::const_iterator i;
    for (i = m_pendingPayments.constBegin(); i != m_pendingPayments.constEnd(); ++i) {
        if (!i.value().second.second.second) { // Only check unpaid addresses
            addressesToCheck.append(i.key());
        }
    }
    locker.unlock();
    
    if (addressesToCheck.isEmpty()) {
        return;
    }

    // Get recent transactions
    int rowCount = m_transactionModel->rowCount();
    
    for (int i = 0; i < rowCount && i < 100; ++i) { // Check last 100 transactions
        QModelIndex idx = m_transactionModel->index(i, 0);
        
        // Get transaction details
        qint64 amount = m_transactionModel->data(idx, TransactionTableModel::AmountRole).toLongLong();
        if (amount <= 0) continue; // Only check incoming transactions
        
        QString address = m_transactionModel->data(idx, TransactionTableModel::AddressRole).toString();
        if (address.isEmpty()) continue;
        
        // Check if this address is in our pending payments
        if (addressesToCheck.contains(address)) {
            processPaymentForAddress(address, amount);
        }
    }
}

void PointOfSaleWebServer::processPaymentForAddress(const QString& address, const CAmount& amount)
{
    QMutexLocker locker(&m_pendingPaymentsMutex);
    
    if (!m_pendingPayments.contains(address)) {
        return;
    }
    
    auto paymentInfo = m_pendingPayments[address];
    qint64 productId = paymentInfo.first;
    int quantity = paymentInfo.second.first;
    qint64 expectedAmount = paymentInfo.second.second.first;
    bool alreadyPaid = paymentInfo.second.second.second;
    
    if (alreadyPaid) {
        return; // Already processed
    }
    
    // Check if payment amount matches (with small tolerance)
    if (amount >= expectedAmount) {
        // Mark as paid
        m_pendingPayments[address].second.second.second = true;
        
        // Update product quantity in database
        Product product = m_db->getProductById(productId);
        if (product.id > 0) {
            if (product.quantity != -1) {
                int newQuantity = product.quantity - quantity;
                if (newQuantity < 0) newQuantity = 0;
                m_db->updateProductQuantity(productId, newQuantity);
            }
            
            // Clear payment address
            m_db->updateProductPaymentAddress(productId, QString(), 0);
            
            LogPrintf("PointOfSaleWebServer: Payment received for product %lld: %s DOGE\n", 
                     productId, BitcoinUnits::format(BitcoinUnits::BTC, amount, false, BitcoinUnits::separatorNever).toStdString());
        }
    }
}

