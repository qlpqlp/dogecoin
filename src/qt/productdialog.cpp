// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "productdialog.h"
#include "ui_productdialog.h"

#include "bitcoinunits.h"
#include "guiutil.h"
#include "pointofsaledb.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSpinBox>

ProductDialog::ProductDialog(PointOfSaleDB* db, qint64 productId, qint64 categoryId, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::ProductDialog),
      m_db(db),
      m_productId(productId),
      m_categoryId(categoryId)
{
    ui->setupUi(this);
    
    loadCategories();
    
    if (m_productId > 0) {
        // Editing existing product
        Product product = m_db->getProductById(m_productId);
        if (product.id > 0) {
            ui->nameLineEdit->setText(product.name);
            ui->descriptionPlainTextEdit->setPlainText(product.description);
            ui->imagePathLineEdit->setText(product.imagePath);
            ui->quantitySpinBox->setValue(product.quantity == -1 ? 999999 : product.quantity);
            ui->unlimitedCheckBox->setChecked(product.quantity == -1);
            ui->priceSpinBox->setValue(BitcoinUnits::fromAmount(product.priceDoge, BitcoinUnits::BTC, false));
            
            // Find and select the category
            for (int i = 0; i < ui->categoryComboBox->count(); i++) {
                qint64 catId = ui->categoryComboBox->itemData(i).toLongLong();
                if (catId == product.categoryId) {
                    ui->categoryComboBox->setCurrentIndex(i);
                    break;
                }
            }
            
            setWindowTitle(tr("Edit Product"));
        }
    } else {
        setWindowTitle(tr("Add Product"));
        if (m_categoryId > 0) {
            // Pre-select the category
            for (int i = 0; i < ui->categoryComboBox->count(); i++) {
                qint64 catId = ui->categoryComboBox->itemData(i).toLongLong();
                if (catId == m_categoryId) {
                    ui->categoryComboBox->setCurrentIndex(i);
                    break;
                }
            }
        }
    }
    
    ui->nameLineEdit->setFocus();
}

ProductDialog::~ProductDialog()
{
    delete ui;
}

void ProductDialog::loadCategories()
{
    ui->categoryComboBox->clear();
    QList<Category> categories = m_db->getAllCategories();
    foreach (const Category& category, categories) {
        ui->categoryComboBox->addItem(category.name, category.id);
    }
    
    if (ui->categoryComboBox->count() == 0) {
        ui->categoryComboBox->addItem(tr("(No categories - please add one first)"), 0);
        ui->categoryComboBox->setEnabled(false);
    }
}

void ProductDialog::onBrowseImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Product Image"),
        QString(), tr("Image Files (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!fileName.isEmpty()) {
        ui->imagePathLineEdit->setText(fileName);
    }
}

void ProductDialog::accept()
{
    QString name = ui->nameLineEdit->text().trimmed();
    
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Product"), tr("Product name cannot be empty."));
        return;
    }
    
    if (ui->categoryComboBox->currentData().toLongLong() == 0) {
        QMessageBox::warning(this, tr("Product"), tr("Please select a category."));
        return;
    }
    
    qint64 categoryId = ui->categoryComboBox->currentData().toLongLong();
    QString description = ui->descriptionPlainTextEdit->toPlainText();
    QString imagePath = ui->imagePathLineEdit->text();
    int quantity = ui->unlimitedCheckBox->isChecked() ? -1 : ui->quantitySpinBox->value();
    qint64 priceDoge = BitcoinUnits::toAmount(ui->priceSpinBox->value(), BitcoinUnits::BTC, false);
    
    if (priceDoge <= 0) {
        QMessageBox::warning(this, tr("Product"), tr("Price must be greater than 0."));
        return;
    }
    
    bool success = false;
    if (m_productId > 0) {
        success = m_db->updateProduct(m_productId, categoryId, name, description, imagePath, quantity, priceDoge);
        if (success) {
            QMessageBox::information(this, tr("Product"), tr("Product updated successfully."));
        } else {
            QMessageBox::critical(this, tr("Product"), tr("Failed to update product."));
            return;
        }
    } else {
        qint64 id = m_db->addProduct(categoryId, name, description, imagePath, quantity, priceDoge);
        success = (id > 0);
        if (success) {
            QMessageBox::information(this, tr("Product"), tr("Product added successfully."));
        } else {
            QMessageBox::critical(this, tr("Product"), tr("Failed to add product."));
            return;
        }
    }
    
    QDialog::accept();
}

