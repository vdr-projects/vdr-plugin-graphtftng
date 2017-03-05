
#include "configdialog.h"

ConfigDialog::ConfigDialog(QWidget* parent)
   : QWidget(parent)
{
   ui.setupUi(this);
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::setSmoothScaling(bool smooth)
{
   ui.smoothScaling->setChecked(smooth);
}

bool ConfigDialog::smoothScaling() const
{
   return ui.smoothScaling->isChecked();
}

void ConfigDialog::setHost(QString host)
{
   ui.lineEditHost->setText(host);
}

QString ConfigDialog::host() const
{
   return ui.lineEditHost->text();
}

void ConfigDialog::setPort(unsigned int port)
{
   ui.lineEditPort->setText(QString::number(port));
}

unsigned int ConfigDialog::port() const
{
   return ui.lineEditPort->text().toInt();
}
