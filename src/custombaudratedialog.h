#ifndef CUSTOMBAUDRATEDIALOG_H
#define CUSTOMBAUDRATEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QIntValidator>
#include <QMessageBox>

namespace Ui {
class CustomBaudrateDialog;
}

class CustomBaudrateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomBaudrateDialog(QWidget *parent  = nullptr, QString defaultValue = "460800" , QComboBox *targetComboBox = nullptr);
    ~CustomBaudrateDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::CustomBaudrateDialog *ui;
    QComboBox *comboBox;
};

#endif // CUSTOMBAUDRATEDIALOG_H
