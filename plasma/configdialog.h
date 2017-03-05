

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "ui_config.h"

class ConfigDialog : public QWidget
{
   Q_OBJECT

   public:

      ConfigDialog(QWidget* parent);
      ~ConfigDialog();
      
      void setSmoothScaling(bool smooth);
      bool smoothScaling() const;
      void setHost(QString host);
      QString host() const;
      void setPort(unsigned int port);
      unsigned int port() const;
      
      Ui::config ui;
      
};

#endif // CONFIGDIALOG_H
