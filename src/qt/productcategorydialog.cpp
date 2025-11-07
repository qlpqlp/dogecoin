// Copyright (c) 2024 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "productcategorydialog.h"
#include "ui_productcategorydialog.h"

#include "pointofsaledb.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

ProductCategoryDialog::ProductCategoryDialog(PointOfSaleDB* db, qint64 categoryId, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::ProductCategoryDialog),
      m_db(db),
      m_categoryId(categoryId)
{
    ui->setupUi(this);
    
    if (m_categoryId > 0) {
        // Editing existing category
        Category category = m_db->getCategoryById(m_categoryId);
        if (category.id > 0) {
            ui->nameLineEdit->setText(category.name);
            setWindowTitle(tr("Edit Category"));
        }
    } else {
        setWindowTitle(tr("Add Category"));
    }
    
    ui->nameLineEdit->setFocus();
}

ProductCategoryDialog::~ProductCategoryDialog()
{
    delete ui;
}

void ProductCategoryDialog::accept()
{
    QString name = ui->nameLineEdit->text().trimmed();
    
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Category"), tr("Category name cannot be empty."));
        return;
    }
    
    bool success = false;
    if (m_categoryId > 0) {
        success = m_db->updateCategory(m_categoryId, name);
        if (success) {
            QMessageBox::information(this, tr("Category"), tr("Category updated successfully."));
        } else {
            QMessageBox::critical(this, tr("Category"), tr("Failed to update category."));
            return;
        }
    } else {
        qint64 id = m_db->addCategory(name);
        success = (id > 0);
        if (success) {
            QMessageBox::information(this, tr("Category"), tr("Category added successfully."));
        } else {
            QMessageBox::critical(this, tr("Category"), tr("Failed to add category."));
            return;
        }
    }
    
    QDialog::accept();
}

