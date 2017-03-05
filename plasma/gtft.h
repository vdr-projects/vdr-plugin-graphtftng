

#ifndef _GTFT_H
#define _GTFT_H

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <Plasma/Applet>

#include <comthread.hpp>

class ConfigDialog;

// using namespace Plasma;

class GtftApplet : public Plasma::Applet
{
   Q_OBJECT

   public:

      GtftApplet(QObject* parent, const QVariantList& args);
      ~GtftApplet();

      void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent);
      void mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent);
      void wheelEvent(QGraphicsSceneWheelEvent* wheelEvent);
      void mousePressEvent(QGraphicsSceneMouseEvent *event);
      void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
      void keyPressEvent(QKeyEvent* keyEvent);

      void paintInterface(QPainter* painter, const QStyleOptionGraphicsItem* option,
                          const QRect& contentsRect);
      void init();

   public slots:

      void updateImage(unsigned char* buffer, int size);
      void createConfigurationInterface(KConfigDialog *parent);

   protected Q_SLOTS:

      void configAccepted();

   protected:

      QPoint clickCoordinate(QPoint screenPos);

   private:

      ConfigDialog* configDialog;
      bool smoothScaling;
      QString host;
      unsigned int port;
      QRect paintRect;

      QPixmap picture;
      ComThread* thread;

      int vdrWidth;
      int vdrHeight;
};

K_EXPORT_PLASMA_APPLET(gtft, GtftApplet)

#endif // _GTFT_H
