// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pointofsalepage.h"
#include "ui_pointofsalepage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "pointofsaledb.h"
#include "pointofsalewebserver.h"
#include "productcategorydialog.h"
#include "productdialog.h"
#include "walletmodel.h"

#include <QMessageBox>
#include <QUrl>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>

PointOfSalePage::PointOfSalePage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PointOfSalePage),
    clientModel(0),
    walletModel(0),
    platformStyle(platformStyle),
    m_db(new PointOfSaleDB(this)),
    m_webServer(nullptr)
{
    ui->setupUi(this);

    // Initialize database
    if (!m_db->initialize()) {
        QMessageBox::critical(this, tr("Point of Sale"), tr("Failed to initialize database."));
    }

    // Connect buttons
    connect(ui->addCategoryButton, SIGNAL(clicked()), this, SLOT(onAddCategory()));
    connect(ui->addProductButton, SIGNAL(clicked()), this, SLOT(onAddProduct()));
    connect(ui->startServerButton, SIGNAL(clicked()), this, SLOT(onStartWebServer()));
    connect(ui->stopServerButton, SIGNAL(clicked()), this, SLOT(onStopWebServer()));
}

PointOfSalePage::~PointOfSalePage()
{
    delete ui;
}

void PointOfSalePage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Update UI when model changes
    }
}

void PointOfSalePage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model)
    {
        updateDisplayUnit();
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        
        // Initialize web server if wallet model is available
        if (!m_webServer) {
            m_webServer = new PointOfSaleWebServer(m_db, walletModel, this);
        }
    }
}

void PointOfSalePage::showOutOfSyncWarning(bool fShow)
{
    // TODO: Show sync warning if needed
}

void PointOfSalePage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        // Update unit display if needed
    }
}

void PointOfSalePage::onAddCategory()
{
    if (!m_db) return;
    
    ProductCategoryDialog dialog(m_db, 0, this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshCategories();
    }
}

void PointOfSalePage::onAddProduct()
{
    if (!m_db) return;
    
    ProductDialog dialog(m_db, 0, 0, this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshProducts();
    }
}

void PointOfSalePage::onEditCategory()
{
    // TODO: Implement edit category from selection
}

void PointOfSalePage::onEditProduct()
{
    // TODO: Implement edit product from selection
}

void PointOfSalePage::onDeleteCategory()
{
    // TODO: Implement delete category with confirmation
}

void PointOfSalePage::onDeleteProduct()
{
    // TODO: Implement delete product with confirmation
}

void PointOfSalePage::refreshCategories()
{
    if (!m_db) return;
    // TODO: Refresh category list in UI
}

void PointOfSalePage::refreshProducts()
{
    if (!m_db) return;
    // TODO: Refresh product list in UI
}

void PointOfSalePage::onStartWebServer()
{
    if (!m_webServer) {
        if (!walletModel) {
            QMessageBox::warning(this, tr("Point of Sale"), tr("Wallet is not loaded."));
            return;
        }
        m_webServer = new PointOfSaleWebServer(m_db, walletModel, this);
    }

    if (m_webServer->start(4200)) {
        ui->startServerButton->setEnabled(false);
        ui->stopServerButton->setEnabled(true);
        ui->serverStatusLabel->setText(tr("Server Status: Running on port 4200"));
        QMessageBox::information(this, tr("Point of Sale"), 
            tr("Web server started. Customers can access your point of sale at http://localhost:4200"));
    } else {
        QMessageBox::critical(this, tr("Point of Sale"), 
            tr("Failed to start web server. Port 4200 may already be in use."));
    }
}

void PointOfSalePage::onStopWebServer()
{
    if (m_webServer) {
        m_webServer->stop();
        ui->startServerButton->setEnabled(true);
        ui->stopServerButton->setEnabled(false);
        ui->serverStatusLabel->setText(tr("Server Status: Stopped"));
    }
}

