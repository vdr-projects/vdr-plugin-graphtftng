
#include <KConfigDialog>
#include <KSharedConfig>
#include <KServiceTypeTrader>
#include <kglobalsettings.h>

#include <gtft.h>
#include <configdialog.h>

GtftApplet::GtftApplet(QObject* parent, const QVariantList& args)
   : Plasma::Applet(parent, args)
{
   configDialog = 0;
   vdrWidth = 720;
   vdrHeight = 576;

   thread = new ComThread();

   setHasConfigurationInterface(true);
   setCacheMode(QGraphicsItem::NoCache);
   setBackgroundHints(Plasma::Applet::NoBackground);
   setAspectRatioMode(Plasma::KeepAspectRatio);  // Plasma::IgnoreAspectRatio

   resize(400, 300);
}

GtftApplet::~GtftApplet()
{
   if (thread)
   {
      tell(eloAlways, "Stopping thread");

      thread->stop();

      if (!thread->wait(1000))
         tell(eloAlways, "Thread would not end!");
      else
         tell(eloAlways, "Thread ended regularly");

      delete thread;
   }
}

void GtftApplet::init()
{
   // picture.load("/home/wendel/test.jpg");

   // read config
   
   KConfigGroup cg = config();
   
   smoothScaling = cg.readEntry("smoothScaling", true);
   host = cg.readEntry("vdr host", "localhost");
   port = cg.readEntry("vdr port", 2039);

   connect(thread, SIGNAL(updateImage(unsigned char*, int)),
           this, SLOT(updateImage(unsigned char*, int)));

   tell(eloAlways, "Starting thread");

   thread->setHost(host.toAscii());
   thread->setPort(port);

   thread->start();
}

//***************************************************************************
// Configuration
//***************************************************************************

void GtftApplet::createConfigurationInterface(KConfigDialog* parent)
{
   configDialog = new ConfigDialog(parent);

   parent->addPage(configDialog, i18n("General"), "configure");
   parent->setDefaultButton(KDialog::Ok);
   parent->showButtonSeparator(true);

   connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
   connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));

   configDialog->setSmoothScaling(smoothScaling);
   configDialog->setHost(host);
   configDialog->setPort(port);
}

void GtftApplet::configAccepted()
{
    KConfigGroup cg = config();

    // connection parameter changed ?

    if (host != configDialog->host() ||
        port != configDialog->port())
    {      
       // reconnect ..

       thread->stop();
       host = configDialog->host();
       port = configDialog->port();
       thread->setHost(host.toAscii());
       thread->setPort(port);
       thread->start();
    }

    smoothScaling = configDialog->smoothScaling();

    cg.writeEntry("smoothScaling", smoothScaling);
    cg.writeEntry("vdr host", host);
    cg.writeEntry("vdr port", port);

    emit configNeedsSaving();
}

void GtftApplet::updateImage(unsigned char* buffer, int size)
{
   thread->getBufferLock()->lockForRead();
   picture.loadFromData(buffer, size);
   thread->getBufferLock()->unlock();

   update();
}

void GtftApplet::paintInterface(QPainter* p, const QStyleOptionGraphicsItem* option,
                                const QRect& rect)
{
   Q_UNUSED(option);
   QPixmap* pixmap;

   Qt::TransformationMode tm = smoothScaling ? Qt::SmoothTransformation : Qt::FastTransformation;

   if (!picture.isNull())
      pixmap = &picture;
   else
      return ;

   QPixmap scaledImage = pixmap->scaled(rect.size(), Qt::KeepAspectRatio, tm);
   p->drawPixmap(rect, scaledImage);

   // notice some data for 'scaling' the mouse cooridinates ...

   paintRect = rect;
   vdrWidth = pixmap->width();
   vdrHeight = pixmap->height();
}

//***************************************************************************
// User Action
//***************************************************************************

void GtftApplet::keyPressEvent(QKeyEvent* keyEvent)
{
   thread->mouseEvent(0, 0, keyEvent->nativeScanCode(), ComThread::efKeyboard);
}

void GtftApplet::mouseMoveEvent(QGraphicsSceneMouseEvent* /*event*/)
{
   // don't drag the widget ...
}

void GtftApplet::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
   QPoint p = clickCoordinate(event->screenPos());

   thread->mouseEvent(p.x(), p.y(), cGraphTftComService::mbLeft, 
                      cGraphTftComService::efDoubleClick);
}

void GtftApplet::mouseReleaseEvent(QGraphicsSceneMouseEvent* /*event*/)
{
   // #TODO
   // Mouse gestures
}

void GtftApplet::wheelEvent(QGraphicsSceneWheelEvent* wheelEvent)
{
   QPoint p = clickCoordinate(wheelEvent->screenPos());

   if (wheelEvent->delta() > 0)
      thread->mouseEvent(p.x(), p.y(), cGraphTftComService::mbWheelUp, 0);
   else
      thread->mouseEvent(p.x(), p.y(), cGraphTftComService::mbWheelDown, 0);
}

void GtftApplet::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
   QPoint p = clickCoordinate(event->screenPos());

   tell(4, "mousePressEvent %d/%d", p.x(), p.y());

   if (event->buttons() == Qt::LeftButton) 
      thread->mouseEvent(p.x(), p.y(), cGraphTftComService::mbLeft, 0);
   else if (event->buttons() == Qt::MidButton)
      thread->mouseEvent(p.x(), p.y(), cGraphTftComService::mbRight, 0);
}

QPoint GtftApplet::clickCoordinate(QPoint screenPos)
{
   QPoint p;

   // calc position relative to the draw area (the image)
   
   p.setX(screenPos.x() - (geometry().x() + paintRect.x()));
   p.setY(screenPos.y() - (geometry().y() + paintRect.y()));

   // scale

   p.setX((int)(((double)p.x() / (double)paintRect.width()) * (double)vdrWidth));
   p.setY((int)(((double)p.y() / (double)paintRect.height()) * (double)vdrHeight));

   return p;
}

//***************************************************************************

#include "gtft.moc"
