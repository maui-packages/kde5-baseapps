/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_childview.h"
#include "konq_factory.h"
#include "konq_frame.h"
#include "konq_mainview.h"
#include "konq_propsview.h"
#include "konq_run.h"
#include "konq_viewmgr.h"
#include <kio/job.h>

#include <assert.h>

#include <kdebug.h>

#include <qapplication.h>

template class QList<HistoryEntry>;

KonqChildView::KonqChildView( KonqViewFactory &viewFactory,
			      KonqFrame* viewFrame,
			      KonqMainView *mainView,
			      const KService::Ptr &service,
			      const KTrader::OfferList &serviceOffers,
			      const QString &serviceType
                              )
{
  m_pKonqFrame = viewFrame;
  m_pKonqFrame->setChildView( this );

  m_sLocationBarURL = "";
  m_bLockHistory = false;
  m_pMainView = mainView;
  m_pRun = 0L;
  m_pView = 0L;

  switchView( viewFactory );

  m_service = service;
  m_serviceOffers = serviceOffers;
  m_serviceType = serviceType;

  m_bAllowHTML = KonqPropsView::defaultProps()->isHTMLAllowed();
  m_lstHistory.setAutoDelete( true );
  m_bReloadURL = false;
  m_iXOffset = 0;
  m_iYOffset = 0;
  m_bLoading = false;
  m_bViewStarted = false;
  m_iProgress = -1;
  m_bPassiveMode = false;
  m_bProgressSignals = true;
  show();
}

KonqChildView::~KonqChildView()
{
  kdDebug(1202) << "KonqChildView::~KonqChildView" << endl;
  // No! We don't take ownership! (David) delete m_pKonqFrame;
  delete m_pView;
  delete (KonqRun *)m_pRun;
}

void KonqChildView::repaint()
{
//  kdDebug(1202) << "KonqChildView::repaint()" << endl;
  if (m_pKonqFrame != 0L)
    m_pKonqFrame->repaint();
//  kdDebug(1202) << "KonqChildView::repaint() : done" << endl;
}

void KonqChildView::show()
{
  kdDebug(1202) << "KonqChildView::show()" << endl;
  if ( m_pKonqFrame )
    m_pKonqFrame->show();
}

void KonqChildView::openURL( const KURL &url, bool useMiscURLData  )
{
  if ( useMiscURLData && browserExtension() )
  {
    KParts::URLArgs args(false, m_iXOffset, m_iYOffset);
    browserExtension()->setURLArgs( args );
  }
  m_pView->openURL( url );

  m_pMainView->setLocationBarURL( this, url.decodedURL() );

  sendOpenURLEvent( url );

  if ( !m_bLockHistory )
  {
      // Update the history with this new URL
      // First, remove any forward history
      HistoryEntry * current = m_lstHistory.current();
      if (current)
      {
          kdDebug(1202) << "Truncating history" << endl;
          m_lstHistory.at( m_lstHistory.count() - 1 ); // go to last one
          for ( ; m_lstHistory.current() != current ; )
          {
              if ( !m_lstHistory.removeLast() ) // and remove from the end (faster and easier)
                  assert(0);
          }
          // Now current is the current again.
      }
      // Append a new entry
      kdDebug(1202) << "Append a new entry" << endl;
      m_lstHistory.append( new HistoryEntry ); // made current
  } else
      m_bLockHistory = false;
  updateHistoryEntry();
  kdDebug(1202) << "Current position : " << m_lstHistory.at() << endl;
}

void KonqChildView::switchView( KonqViewFactory &viewFactory )
{
  kdDebug(1202) << "KonqChildView::switchView" << endl;
  if ( m_pView )
    m_pView->widget()->hide();

  KParts::ReadOnlyPart *oldView = m_pView;
  m_pView = m_pKonqFrame->attach( viewFactory );

  if ( oldView )
  {
    emit sigViewChanged( oldView, m_pView );

    delete oldView;
  }

  connectView();
}

bool KonqChildView::changeViewMode( const QString &serviceType,
                                    const KURL &url, bool useMiscURLData,
				    const QString &serviceName )
{
  if ( m_bViewStarted )
    stop();

  if ( !m_service->serviceTypes().contains( serviceType ) ||
       ( !serviceName.isEmpty() && serviceName != m_service->name() ) )
  {

    KTrader::OfferList serviceOffers;
    KService::Ptr service = 0L;
    KonqViewFactory viewFactory = KonqFactory::createView( serviceType, serviceName, &service, &serviceOffers );

    if ( viewFactory.isNull() )
      return false;

    m_service = service;
    m_serviceOffers = serviceOffers;
    m_serviceType = serviceType;

    switchView( viewFactory );
  }

  openURL( url, useMiscURLData );

  show();
  // Give focus to the view
  m_pView->widget()->setFocus();

  // openURL is the one that changes the history
  if ( m_pMainView->currentChildView() == this )
  {
    kdDebug() << "updating toolbar actions" << endl;
    m_pMainView->updateToolBarActions();
  }
  return true;
}

void KonqChildView::connectView(  )
{
  kdDebug(1202) << "KonqChildView::connectView" << endl;
  connect( m_pView, SIGNAL( started( KIO::Job * ) ),
           this, SLOT( slotStarted( KIO::Job * ) ) );
  connect( m_pView, SIGNAL( completed() ),
           this, SLOT( slotCompleted() ) );
  connect( m_pView, SIGNAL( canceled( const QString & ) ),
           this, SLOT( slotCanceled( const QString & ) ) );

  KParts::BrowserExtension *ext = browserExtension();

  if ( !ext )
    return;

  connect( ext, SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs &) ),
           m_pMainView, SLOT( openURL( const KURL &, const KParts::URLArgs & ) ) );

  connect( ext, SIGNAL( popupMenu( const QPoint &, const KonqFileItemList & ) ),
           m_pMainView, SLOT( slotPopupMenu( const QPoint &, const KonqFileItemList & ) ) );

  connect( ext, SIGNAL( popupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ),
	   m_pMainView, SLOT( slotPopupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ) );

  connect( ext, SIGNAL( setLocationBarURL( const QString & ) ),
           m_pMainView, SLOT( slotSetLocationBarURL( const QString & ) ) );

  connect( ext, SIGNAL( createNewWindow( const KURL &, const KParts::URLArgs & ) ),
           m_pMainView, SLOT( slotCreateNewWindow( const KURL &, const KParts::URLArgs & ) ) );

  connect( ext, SIGNAL( loadingProgress( int ) ),
           this, SLOT( slotLoadingProgress( int ) ) );

  connect( ext, SIGNAL( speedProgress( int ) ),
           this, SLOT( slotSpeedProgress( int ) ) );

}

void KonqChildView::slotStarted( KIO::Job * job )
{
  kdDebug(1202) << "KonqChildView::slotStarted"  << job << endl;
  m_bLoading = true;
  setViewStarted( true );

  if ( m_pMainView->currentChildView() == this )
  {
    m_pMainView->updateStatusBar();
    m_pMainView->updateToolBarActions();
  }

  if (job)
  {
      connect( job, SIGNAL( sigTotalSize( int, unsigned long ) ), this, SLOT( slotTotalSize( int, unsigned long ) ) );
      connect( job, SIGNAL( sigProcessedSize( int, unsigned long ) ), this, SLOT( slotProcessedSize( int, unsigned long ) ) );
      connect( job, SIGNAL( sigSpeed( int, unsigned long ) ), this, SLOT( slotSpeed( int, unsigned long ) ) );
  }
  m_ulTotalDocumentSize = 0;
}

void KonqChildView::slotTotalSize( int, unsigned long size )
{
  m_ulTotalDocumentSize = size;
}

void KonqChildView::slotProcessedSize( int, unsigned long size )
{
  if ( m_ulTotalDocumentSize > (unsigned long)0 )
    slotLoadingProgress( size * 100 / m_ulTotalDocumentSize );
}

void KonqChildView::slotSpeed( int, unsigned long bytesPerSecond )
{
  slotSpeedProgress( (long int)bytesPerSecond );
}

void KonqChildView::slotLoadingProgress( int percent )
{
  m_iProgress = percent;
  if ( m_pMainView->currentChildView() == this )
  {
    m_pMainView->updateStatusBar();
  }
}

void KonqChildView::slotSpeedProgress( int bytesPerSecond )
{
  if ( m_pMainView->currentChildView() == this )
  {
    m_pMainView->speedProgress( bytesPerSecond );
  }
}

void KonqChildView::slotCompleted()
{
  kdDebug() << "KonqChildView::slotCompleted" << endl;
  m_bLoading = false;
  setViewStarted( false );
  slotLoadingProgress( -1 );

  if ( m_pMainView->currentChildView() == this )
  {
    kdDebug() << "updating toolbar actions" << endl;
    m_pMainView->updateToolBarActions();
  }
}

void KonqChildView::slotCanceled( const QString & )
{
#ifdef __GNUC__
#warning TODO obey errMsg
#endif
  slotCompleted();
}

void KonqChildView::updateHistoryEntry()
{
  /*
  if ( pushEntry )
  {
    if ( !m_bHistoryLock )
    {
      if ( m_bBack )
      {
        m_bBack = false;
//        kdDebug(1202) << "pushing into forward history : " << m_pCurrentHistoryEntry->strURL << endl;
        m_lstForward.insert( 0, m_pCurrentHistoryEntry );
      }
      else if ( m_bForward )
      {
        m_bForward = false;
//        kdDebug(1202) << "pushing into backward history : " << m_pCurrentHistoryEntry->strURL << endl;
        m_lstBack.insert( 0, m_pCurrentHistoryEntry );
      }
      else
      {
        m_lstForward.clear();
//        kdDebug(1202) << "pushing into backward history : " << m_pCurrentHistoryEntry->strURL << endl;
        m_lstBack.insert( 0, m_pCurrentHistoryEntry );
      }
    }
    else
      m_bHistoryLock = false;
  }

  if ( pushEntry || !m_pCurrentHistoryEntry )
    m_pCurrentHistoryEntry = new HistoryEntry;
  */

  HistoryEntry * current = m_lstHistory.current();
  assert( current ); // let's see if this happens
  if ( current == 0L) // empty history
  {
    kdDebug(1202) << "Creating item because history is empty !" << endl;
    current = new HistoryEntry;
    m_lstHistory.append( current );
  }

  kdDebug() << "looking for extension" << endl;
  if ( browserExtension() )
  {
    kdDebug() << "creating stream" << endl;
    QDataStream stream( current->buffer, IO_WriteOnly );

    kdDebug() << "saving" << endl;
    browserExtension()->saveState( stream );
  }

  kdDebug() << "storing stuff" << endl;
  current->url = m_pView->url();
  current->strServiceType = m_serviceType;
  current->strServiceName = m_service->name();
}

void KonqChildView::go( int steps )
{
  kdDebug(1202) << "go : " << steps << endl;
  int newPos = m_lstHistory.at() + steps;
  assert( newPos >= 0 && (uint)newPos < m_lstHistory.count() );
  // Yay, we can move there without a loop !
  HistoryEntry *h = m_lstHistory.at( newPos ); // sets current item
  assert( h );
  assert( newPos == m_lstHistory.at() ); // check we moved (i.e. if I understood the docu)
  assert( h == m_lstHistory.current() );
  kdDebug(1202) << "New position " << m_lstHistory.at() << endl;

  if ( m_bViewStarted )
    stop();

  if ( !m_service->serviceTypes().contains( h->strServiceType ) ||
       h->strServiceName != m_service->name() )
  {
    KTrader::OfferList serviceOffers;
    KService::Ptr service;
    KonqViewFactory viewFactory = KonqFactory::createView( h->strServiceType, h->strServiceName, &service, &serviceOffers );

    if ( viewFactory.isNull() )
     return;

    m_service = service;
    m_serviceOffers = serviceOffers;
    m_serviceType = h->strServiceType;

    switchView( viewFactory );
  }

  if ( browserExtension() )
  {
    QDataStream stream( h->buffer, IO_ReadOnly );

    browserExtension()->restoreState( stream );
  }
  else
    m_pView->openURL( h->url );

  sendOpenURLEvent( h->url );

  m_pMainView->setLocationBarURL( this, h->url.decodedURL() );

  if ( m_pMainView->currentChildView() == this )
    m_pMainView->updateToolBarActions();

  //updateHistoryEntry(); // do we really need that here ?
  kdDebug(1202) << "New position (2) " << m_lstHistory.at() << endl;
}

KURL KonqChildView::url()
{
  assert( m_pView );
  return m_pView->url();
}

void KonqChildView::setRun( KonqRun * run )
{
  m_pRun = run;
}

void KonqChildView::stop()
{
  if ( m_bViewStarted )
  {
    m_pView->closeURL();
    m_bViewStarted = false;
  }
  else if ( m_pRun )
    delete (KonqRun *)m_pRun; // should set m_pRun to 0L

  m_bLoading = false;
  slotLoadingProgress( -1 );

    //  if ( m_pRun ) debug(" m_pRun is not NULL "); else debug(" m_pRun is NULL ");
  //if ( m_pRun ) delete (KonqRun *)m_pRun; // should set m_pRun to 0L
}

void KonqChildView::reload()
{
  //lockHistory();
  if ( browserExtension() )
  {
    KParts::URLArgs args(true, browserExtension()->xOffset(), browserExtension()->yOffset());
    browserExtension()->setURLArgs( args );
  }

  //  m_pView->openURL( m_pView->url(), true, m_pView->xOffset(), m_pView->yOffset() );
  m_pView->openURL( m_pView->url() );
}

void KonqChildView::setPassiveMode( bool mode )
{
  m_bPassiveMode = mode;

  if ( mode && m_pMainView->viewCount() > 1 && m_pMainView->currentChildView() == this )
  //    m_pMainView->setActiveView( m_pMainView->viewManager()->chooseNextView( this )->view() );
    m_pMainView->viewManager()->setActivePart( m_pMainView->viewManager()->chooseNextView( this )->view() );

  // Hide the mode button for the last active view
  //
  KonqChildView *current = m_pMainView->currentChildView();

  if (current != 0L) {
    if ( m_pMainView->viewManager()->chooseNextView( current ) == 0L ) {
      current->frame()->statusbar()->passiveModeCheckBox()->hide();
    } else {
      current->frame()->statusbar()->passiveModeCheckBox()->show();
    }
  }
}

void KonqChildView::sendOpenURLEvent( const KURL &url )
{
  QMap<KParts::ReadOnlyPart *, KonqChildView *> views = m_pMainView->viewMap();
  QMap<KParts::ReadOnlyPart *, KonqChildView *>::ConstIterator it = views.begin();
  QMap<KParts::ReadOnlyPart *, KonqChildView *>::ConstIterator end = views.end();
  for (; it != end; ++it )
  {
    KParts::OpenURLEvent ev( m_pView, url );
    QApplication::sendEvent( it.key(), &ev );
  }
}

#include "konq_childview.moc"
