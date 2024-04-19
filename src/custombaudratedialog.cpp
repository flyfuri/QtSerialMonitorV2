#include "custombaudratedialog.h"
#include "ui_custombaudratedialog.h"

CustomBaudrateDialog::CustomBaudrateDialog(QWidget *parent, QString defaultValue, QComboBox *targetComboBox)
    : QDialog(parent),
    comboBox(targetComboBox),
    ui(new Ui::CustomBaudrateDialog)
{
    ui->setupUi(this);
    ui->lineEditCustomBaud->setText(defaultValue);
    ui->lineEditCustomBaud->setMaxLength(7);
    ui->lineEditCustomBaud->setInputMask("9999999");
    //ui->lineEditCustomBaud->setValidator( new QIntValidator(1, 3000000, this) );
}

CustomBaudrateDialog::~CustomBaudrateDialog()
{
    delete ui;
}


void CustomBaudrateDialog::on_buttonBox_accepted()
{
    if (comboBox == nullptr)
    {
        QMessageBox::warning(this, "programming error (missing pointer to comboBox)", QString(comboBox->itemText(comboBox->count() - 2) + " will be choosen instead"),
                             QMessageBox::Ok);
    }
    else if (ui->lineEditCustomBaud->text().toInt() > 3000000 || ui->lineEditCustomBaud->text().toInt() < 1)
    {
        QMessageBox::warning(this, "invalid value", QString(comboBox->itemText(comboBox->count() - 2) + " will be choosen instead"),
                             QMessageBox::Ok);
    }
    else
    {
        comboBox->insertItem((comboBox->count() -1) , ui->lineEditCustomBaud->text());
    }
    comboBox->setCurrentIndex((comboBox->count() -2));
}

