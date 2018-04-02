/*
 * DesktopBrowserWindow.cpp
 *
 * Copyright (C) 2009-18 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include "DesktopBrowserWindow.hpp"

#include <QApplication>
#include <QToolBar>
#include <QWebChannel>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

#include "DesktopInfo.hpp"
#include "DesktopOptions.hpp"

namespace rstudio {
namespace desktop {

namespace {

void initializeWebchannel(BrowserWindow* pWindow)
{
   auto* page = pWindow->webView()->webPage();
   
   // create web channel
   auto* channel = new QWebChannel(pWindow);
   channel->registerObject(QStringLiteral("desktopInfo"), &desktopInfo());
   page->setWebChannel(channel);
   
   // load qwebchannel.js
   QFile webChannelJsFile(QStringLiteral(":/qtwebchannel/qwebchannel.js"));
   if (!webChannelJsFile.open(QFile::ReadOnly))
      qWarning() << "Failed to load qwebchannel.js";

   QString webChannelJs = QString::fromUtf8(webChannelJsFile.readAll());
   webChannelJsFile.close();

   // append our WebChannel initialization code
   const char* webChannelInit = 1 + R"EOF(
new QWebChannel(qt.webChannelTransport, function(channel) {

   // export channel objects to the main window
   for (var key in channel.objects) {
      window[key] = channel.objects[key];
   }

   // notify that we're finished initialization and load
   // GWT sources if necessary
   window.qt.webChannelReady = true;
   if (typeof window.rstudioDelayLoadApplication == "function") {
      window.rstudioDelayLoadApplication();
      window.rstudioDelayLoadApplication = null;
   }
});
   )EOF";

   webChannelJs.append(QString::fromUtf8(webChannelInit));

   QWebEngineScript script;
   script.setName(QStringLiteral("qwebchannel"));
   script.setInjectionPoint(QWebEngineScript::DocumentCreation);
   script.setWorldId(QWebEngineScript::MainWorld);
   script.setSourceCode(webChannelJs);
   page->scripts().insert(script);
}

} // end anonymous namespace

BrowserWindow::BrowserWindow(bool showToolbar,
                             bool adjustTitle,
                             QString name,
                             QUrl baseUrl,
                             QWidget* pParent,
                             WebPage* pOpener,
                             bool allowExternalNavigate) :
   QMainWindow(pParent),
   name_(name),
   pOpener_(pOpener)
{
   adjustTitle_ = adjustTitle;
   progress_ = 0;

   pView_ = new WebView(baseUrl, this, allowExternalNavigate);
   connect(pView_, SIGNAL(titleChanged(QString)), SLOT(adjustTitle()));
   connect(pView_, SIGNAL(loadProgress(int)), SLOT(setProgress(int)));
   connect(pView_, SIGNAL(loadFinished(bool)), SLOT(finishLoading(bool)));
   
   initializeWebchannel(this);
   
   // set zoom factor
   double zoomLevel = options().zoomLevel();
   pView_->setZoomFactor(zoomLevel);

   // Kind of a hack to new up a toolbar and not attach it to anything.
   // Once it is clear what secondary browser windows look like we can
   // decide whether to keep this.
   pToolbar_ = showToolbar ? addToolBar(tr("Navigation")) : new QToolBar();
   pToolbar_->setMovable(false);

   setCentralWidget(pView_);
   setUnifiedTitleAndToolBarOnMac(true);

   desktop::enableFullscreenMode(this, false);
}

void BrowserWindow::closeEvent(QCloseEvent *event)
{
   if (pOpener_ == nullptr)
   {
      // if we don't know where we were opened from, check window.opener
      // (note that this could also be empty)
      QString cmd = QString::fromUtf8("if (window.opener && "
         "window.opener.unregisterDesktopChildWindow)"
         "   window.opener.unregisterDesktopChildWindow('");
      cmd.append(name_);
      cmd.append(QString::fromUtf8("');"));
      webPage()->runJavaScript(cmd);
   }
   else if (pOpener_)
   {
      // if we do know where we were opened from and it has the appropriate
      // handlers, let it know we're closing
      QString cmd = QString::fromUtf8(
                  "if (window.unregisterDesktopChildWindow) "
                  "   window.unregisterDesktopChildWindow('");
      cmd.append(name_);
      cmd.append(QString::fromUtf8("');"));
      webPage()->runJavaScript(cmd);
   }

   // forward the close event to the page
   webPage()->event(event);
}

void BrowserWindow::adjustTitle()
{
   if (adjustTitle_)
      setWindowTitle(pView_->title());
}

void BrowserWindow::setProgress(int p)
{
   progress_ = p;
   adjustTitle();
}

void BrowserWindow::finishLoading(bool succeeded)
{
   progress_ = 100;
   adjustTitle();

   if (succeeded)
   {
      QString cmd = QString::fromUtf8("if (window.opener && "
         "window.opener.registerDesktopChildWindow)"
         "   window.opener.registerDesktopChildWindow('");
      cmd.append(name_);
      cmd.append(QString::fromUtf8("', window);"));
      webPage()->runJavaScript(cmd);
   }
}

WebView* BrowserWindow::webView()
{
   return pView_;
}

void BrowserWindow::avoidMoveCursorIfNecessary()
{
#ifdef Q_OS_MAC
   webView()->page()->runJavaScript(
         QString::fromUtf8("document.body.className = document.body.className + ' avoid-move-cursor'"));
#endif
}

QWidget* BrowserWindow::asWidget()
{
   return this;
}

WebPage* BrowserWindow::webPage()
{
   return webView()->webPage();
}

void BrowserWindow::postWebViewEvent(QEvent *keyEvent)
{
   for (auto* child : webView()->children())
      QApplication::sendEvent(child, keyEvent);
}

void BrowserWindow::triggerPageAction(QWebEnginePage::WebAction action)
{
   webView()->triggerPageAction(action);
}

QString BrowserWindow::getName()
{
   return name_;
}

} // namespace desktop
} // namespace rstudio
