/**
 * kcookiespolicies.cpp - Cookies configuration
 *
 * Original Authors
 * Copyright (c) Waldo Bastian <bastian@kde.org>
 * Copyright (c) 1999 David Faure <faure@kde.org>
 *
 * Re-written by:
 * Copyright (c) 2000- Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <qvbox.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>

#include <kidna.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>
#include <dcopclient.h>

#include "ksaveioconfig.h"
#include "kcookiespolicies.h"

KCookiesPolicies::KCookiesPolicies(QWidget *parent)
                 :KCModule(parent, "kcmkio")
{
    QVBoxLayout *mainLayout = new QVBoxLayout( this, KDialog::marginHint(),
                                               KDialog::spacingHint() );

    mainLayout->setAutoAdd (true);

    m_cbEnableCookies = new QCheckBox( i18n("Enable coo&kies"), this );
    m_cbEnableCookies->setSizePolicy (m_cbEnableCookies->sizePolicy().verData(),
                                      QSizePolicy::Minimum);

    QWhatsThis::add( m_cbEnableCookies, i18n("Enable cookie support. Normally "
                                            "you will want to have cookie support "
                                            "enabled and customize it to suit your "
                                            "privacy needs.<p>Please note that "
                                            "disabling cookie support makes many "
                                            "of web today's web sites unbrowsable.") );


    m_bgPreferences = new QVButtonGroup( this );
    m_cbRejectCrossDomainCookies = new QCheckBox ( i18n("Only accept cookies from "
                                                        "originating server"),
                                                   m_bgPreferences );
    m_cbRejectCrossDomainCookies->setSizePolicy (m_cbRejectCrossDomainCookies->sizePolicy().verData(),
                                                 QSizePolicy::Minimum);
    QWhatsThis::add( m_cbRejectCrossDomainCookies,
                     i18n("Reject the so called third-party cookies. These "
                          "are cookies that originate from a site other than "
                          "the one you are currently browsing. For example, if "
                          "you visit www.foobar.com while this option is on, "
                          "only cookies that originate from www.foobar.com "
                          "will be processed per your settings. Cookies from "
                          "any other site will be rejected. This reduces the "
                          "chances of site operators compiling a profile about "
                          "your daily browsing habits.") );

    m_bgPreferences->insert( m_cbRejectCrossDomainCookies );

    m_cbAutoAcceptSessionCookies = new QCheckBox ( i18n("Automatically accept "
                                                        "session cookies"),
                                                   m_bgPreferences );
    m_cbAutoAcceptSessionCookies->setSizePolicy (m_cbAutoAcceptSessionCookies->sizePolicy().verData(),
                                                 QSizePolicy::Minimum);
    QWhatsThis::add( m_cbAutoAcceptSessionCookies,
                     i18n("Check this option to automatically accept temporary "
                          "cookies meant to expire at the end of the current "
                          "session. Such cookies will not be stored in your "
                          "computer's hard drive or storage device. Instead, "
                          "they are deleted when you close all applications "
                          "(e.g. your browser) that use them."
                          "<P>NOTE: Checking this option along with the next "
                          "one will override your default as well as site "
                          "specific cookie policies. However, doing so also "
                          "increases your privacy since all cookies will be "
                          "removed when the current session ends.") );

    m_bgPreferences->insert( m_cbAutoAcceptSessionCookies );

    m_cbIgnoreCookieExpirationDate = new QCheckBox ( i18n("Treat all cookies as "
                                                     "session cookies"),
                                                     m_bgPreferences );
    m_cbIgnoreCookieExpirationDate->setSizePolicy (m_cbIgnoreCookieExpirationDate->sizePolicy().verData(),
                                                   QSizePolicy::Minimum);
    QWhatsThis::add( m_cbIgnoreCookieExpirationDate,
                     i18n("Treat all cookies as session cookies. Session "
                          "cookies are small pieces of data that are temporarily "
                          "stored in your computer's memory until you quit or "
                          "close all applications (e.g. your browser) that "
                          "use use them. Unlike regular cookies, session "
                          "cookies are never stored on your hard drive or"
                          "other storage medium."
                          "<P>NOTE: checking this option along with the "
                          "previous one will override your default as well "
                          "as site specific cookie policies. However, doing "
                          "so also increases your privacy since all cookies "
                          "will be removed when the current session ends.") );

    m_bgPreferences->insert( m_cbAutoAcceptSessionCookies );


    m_bgDefault = new QVButtonGroup( i18n("Default Policy"), this );
    QWhatsThis::add( m_bgDefault,
                     i18n("The default policy determines how cookies received "
                          "from a remote machine, which is not associated with "
                          "a specific policy (see below), will be handled: "
                          "<ul><li><b>Ask</b> will cause KDE to ask for your "
                          "confirmation whenever a server wants to set a cookie."
                          "</li><li><b>Accept</b> will cause cookies to be "
                          "accepted without prompting you.</li><li><b>Reject</b> "
                          "will cause the cookiejar to refuse all cookies it "
                          "receives.</li></ul>") );
    m_bgDefault->setExclusive( true );

    m_rbPolicyAsk = new QRadioButton( i18n("A&sk for confirmation before "
                                            "accepting cookies"), m_bgDefault );
    m_rbPolicyAsk->setSizePolicy (m_rbPolicyAsk->sizePolicy().verData(),
                                  QSizePolicy::Minimum);
    m_bgDefault->insert (m_rbPolicyAsk, KCookieAdvice::Ask);

    m_rbPolicyAccept = new QRadioButton( i18n("Accep&t all cookies by "
                                               "default"), m_bgDefault );
    m_rbPolicyAccept->setSizePolicy (m_rbPolicyAccept->sizePolicy().verData(),
                                     QSizePolicy::Minimum);
    m_bgDefault->insert (m_rbPolicyAccept, KCookieAdvice::Accept);

    m_rbPolicyReject = new QRadioButton( i18n("Re&ject all cookies by "
                                               "default"), m_bgDefault );
    m_rbPolicyReject->setSizePolicy (m_rbPolicyReject->sizePolicy().verData(),
                                     QSizePolicy::Minimum);
    m_bgDefault->insert (m_rbPolicyReject, KCookieAdvice::Reject);

    // Create Group Box for specific settings
    m_gbDomainSpecific = new QGroupBox( i18n("Domain Specific Policy"), this);
    m_gbDomainSpecific->setColumnLayout( 0, Qt::Horizontal );
    QGridLayout *s_grid = new QGridLayout( m_gbDomainSpecific->layout(), 3, 2,
                                           KDialog::spacingHint() );
    s_grid->setColStretch( 0, 2 ); // only resize the listbox horizontally, not the buttons
    s_grid->setRowStretch( 2, 2 );

    // CREATE SPLIT LIST BOX
    m_lvDomainPolicy = new KListView( m_gbDomainSpecific );
    m_lvDomainPolicy->setShowSortIndicator (true);
    m_lvDomainPolicy->setSelectionMode (QListView::Extended);

    m_lvDomainPolicy->addColumn(i18n("Domain"));
    m_lvDomainPolicy->addColumn(i18n("Policy"), 100);

    m_lvDomainPolicy->setTreeStepSize(0);
    m_lvDomainPolicy->setSorting(0);
    s_grid->addMultiCellWidget( m_lvDomainPolicy, 1, 2, 0, 0 );

    QString wtstr = i18n("This box contains the domains for which you have set a "
                         "specific cookie policy. This policy will be used instead "
                         "of the default policy for any cookie sent by these "
                         "domains. <p>Select a policy and use the controls on "
                         "the right to modify it.");

    QWhatsThis::add( m_lvDomainPolicy, wtstr );
    QWhatsThis::add( m_gbDomainSpecific, wtstr );

    QVBox* vbox = new QVBox( m_gbDomainSpecific );
    vbox->setSpacing( KDialog::spacingHint() );
    m_pbAdd = new QPushButton( i18n("&New..."), vbox );
    QWhatsThis::add( m_pbAdd, i18n("Add a domain specific cookie policy.") );
    connect( m_pbAdd, SIGNAL(clicked()), SLOT( addPressed() ) );

    m_pbChange = new QPushButton( i18n("Chan&ge..."), vbox );
    m_pbChange->setEnabled( false );
    QWhatsThis::add( m_pbChange, i18n("Change the policy for the selected item "
                                              "in the list box.") );
    connect( m_pbChange, SIGNAL( clicked() ), SLOT( changePressed() ) );


    m_pbDelete = new QPushButton( i18n("De&lete"), vbox );
    m_pbDelete->setEnabled( false );
    QWhatsThis::add( m_pbDelete, i18n("Remove the selected policy.") );
    connect( m_pbDelete, SIGNAL( clicked() ), SLOT( deletePressed() ) );

    m_pbDeleteAll = new QPushButton( i18n("D&elete All"), vbox );
    m_pbDeleteAll->setEnabled( false );
    QWhatsThis::add( m_pbDeleteAll, i18n("Remove all domain policies.") );
    connect( m_pbDeleteAll, SIGNAL( clicked() ), SLOT( deleteAllPressed() ) );

#if 0
    pb_domPolicyImport = new QPushButton( i18n("Import..."), vbox );
    QWhatsThis::add( pb_domPolicyImport, i18n("Click this button to choose the file that contains "
                                              "the cookie policies. These policies will be merged "
                                              "with the existing ones. Duplicate entries are ignored.") );
    connect( pb_domPolicyImport, SIGNAL( clicked() ), SLOT( importPressed() ) );
    pb_domPolicyImport->hide();

    pb_domPolicyExport = new QPushButton( i18n("Export..."), vbox );
    QWhatsThis::add( pb_domPolicyExport, i18n("Click this button to save the cookie policy to a zipped "
                                              "file. The file, named <b>cookie_policy.tgz</b>, will be "
                                              "saved to a location of your choice." ) );

    connect( pb_domPolicyExport, SIGNAL( clicked() ), SLOT( exportPressed() ) );
    pb_domPolicyExport->hide();
#endif

    QWhatsThis::add( m_gbDomainSpecific, i18n("Here you can set specific cookie policies for any particular "
                                             "domain. To add a new policy, simply click on the <i>Add...</i> "
                                             "button and supply the necessary information requested by the "
                                             "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                             "button and choose the new policy from the policy dialog box. Clicking "
                                             "on the <i>Delete</i> button will remove the selected policy causing the "
                                             "default policy setting to be used for that domain. ") );
#if 0
                                             "The <i>Import</i> and <i>Export</i> "
                                             "button allows you to easily share your policies with other people by allowing "
                                             "you to save and retrive them from a zipped file."
#endif

    s_grid->addWidget( vbox, 1, 1 );

    load();
}

KCookiesPolicies::~KCookiesPolicies()
{
}

void KCookiesPolicies::emitChanged ()
{
    emit changed ( true );
}

void KCookiesPolicies::cookiesEnabled( bool enable )
{
    m_bgDefault->setEnabled( enable );
    m_bgPreferences->setEnabled ( enable );
    m_gbDomainSpecific->setEnabled( enable );

    if (enable)
    {
      ignoreCookieExpirationDate ( enable );
      autoAcceptSessionCookies ( enable );
    }
}

void KCookiesPolicies::ignoreCookieExpirationDate ( bool enable )
{
    bool isAutoAcceptChecked = m_cbAutoAcceptSessionCookies->isChecked();
    enable = (enable && isAutoAcceptChecked);

    m_bgDefault->setEnabled( !enable );
    m_gbDomainSpecific->setEnabled( !enable );
}

void KCookiesPolicies::autoAcceptSessionCookies ( bool enable )
{
    bool isIgnoreExpirationChecked = m_cbIgnoreCookieExpirationDate->isChecked();
    enable = (enable && isIgnoreExpirationChecked);

    m_bgDefault->setEnabled( !enable );
    m_gbDomainSpecific->setEnabled( !enable );
}

void KCookiesPolicies::addPressed()
{
  int globalPolicy;
  KCookiePolicyDlg* dlg;

  globalPolicy = m_bgDefault->id (m_bgDefault->selected());
  dlg = new KCookiePolicyDlg (i18n("New Cookie Policy"), this);

  if( dlg->exec() && !dlg->domain().isEmpty())
  {
    QString domain = KIDNA::toUnicode(dlg->domain());
    int advice = dlg->advice();

    if ( !handleDuplicate(domain, advice) )
    {
      const char* strAdvice = KCookieAdvice::adviceToStr(advice);
      QListViewItem* index = new QListViewItem (m_lvDomainPolicy, domain,
                                                i18n(strAdvice));
      m_pDomainPolicy.insert (index, strAdvice);
      m_lvDomainPolicy->setCurrentItem (index);
      changed( true );
    }
  }

  delete dlg;
}

void KCookiesPolicies::changePressed()
{
  int globalPolicy;
  QString oldDomain;
  QString newDomain;
  QListViewItem *index;
  KCookiePolicyDlg* dlg;
  KCookieAdvice::Value advice;

  index = m_lvDomainPolicy->currentItem();

  if (!index)
    return;

  globalPolicy = m_bgDefault->id (m_bgDefault->selected());
  advice = KCookieAdvice::strToAdvice(m_pDomainPolicy[index]);
  dlg = new KCookiePolicyDlg (i18n("Change Cookie Policy"), this);

  oldDomain = index->text(0);
  dlg->setPolicy (advice);
  dlg->setEnableHostEdit (true, oldDomain);

  if( dlg->exec() && !dlg->domain().isEmpty())
  {
    newDomain = KIDNA::toUnicode(dlg->domain());
    int advice = dlg->advice();
    if (newDomain == oldDomain || !handleDuplicate(newDomain, advice))
    {
      m_pDomainPolicy[index] = KCookieAdvice::adviceToStr(advice);
      index->setText(0, newDomain);
      index->setText(1, i18n(m_pDomainPolicy[index]) );
      changed( true );
    }
  }

  delete dlg;
}

bool KCookiesPolicies::handleDuplicate( const QString& domain, int advice )
{
  QListViewItem* item = m_lvDomainPolicy->firstChild();
  while ( item != 0 )
  {
    if ( item->text(0) == domain )
    {
      QString msg = i18n("<qt>A policy already exists for"
                         "<center><b>%1</b></center>"
                         "Do you want to replace it?</qt>").arg(domain);
      int res = KMessageBox::warningYesNo(this, msg,
                                          i18n("Duplicate Policy"),
                                          QString::null);
      if ( res == KMessageBox::Yes )
      {
        m_pDomainPolicy[item]= KCookieAdvice::adviceToStr(advice);
        item->setText(0, domain);
        item->setText(1, i18n(m_pDomainPolicy[item]));
        changed( true );
        return true;
      }
      else
        return true;  // User Cancelled!!
    }
    item = item->nextSibling();
  }
  return false;
}

void KCookiesPolicies::deletePressed()
{
  QListViewItem* nextItem = 0L;
  QListViewItem* item = m_lvDomainPolicy->firstChild ();

  while (item != 0L)
  {
    if (m_lvDomainPolicy->isSelected (item))
    {
      nextItem = item->itemBelow();
      if ( !nextItem )
        nextItem = item->itemAbove();

      delete item;
      item = nextItem;
    }
    else
    {
      item = item->itemBelow();
    }
  }

  if (nextItem)
    m_lvDomainPolicy->setSelected (nextItem, true);

  updateButtons();
  changed( true );
}

void KCookiesPolicies::deleteAllPressed()
{
    m_pDomainPolicy.clear();
    m_lvDomainPolicy->clear();
    updateButtons();
    changed( true );
}

#if 0
void KCookiesPolicies::importPressed()
{
}

void KCookiesPolicies::exportPressed()
{
}
#endif

void KCookiesPolicies::updateButtons()
{
  bool hasItems = m_lvDomainPolicy->childCount() > 0;

  m_pbChange->setEnabled ((hasItems && d_itemsSelected == 1));
  m_pbDelete->setEnabled ((hasItems && d_itemsSelected > 0));
  m_pbDeleteAll->setEnabled ( hasItems );
}

void KCookiesPolicies::updateDomainList(const QStringList &domainConfig)
{
    m_lvDomainPolicy->clear();

    QStringList::ConstIterator it = domainConfig.begin();
    for (; it != domainConfig.end(); ++it)
    {
      QString domain;
      KCookieAdvice::Value advice;
      QListViewItem *index;

      splitDomainAdvice(*it, domain, advice);
      index = new QListViewItem( m_lvDomainPolicy, KIDNA::toUnicode(domain),
                                 i18n(KCookieAdvice::adviceToStr(advice)) );
      m_pDomainPolicy[index] = KCookieAdvice::adviceToStr(advice);
    }
}

void KCookiesPolicies::selectionChanged ()
{
  QListViewItem* item = m_lvDomainPolicy->firstChild ();

  d_itemsSelected = 0;

  while (item != 0L)
  {
    if (m_lvDomainPolicy->isSelected (item))
      d_itemsSelected++;
    item = item->nextSibling ();
  }

  updateButtons ();
}

void KCookiesPolicies::load()
{
  d_itemsSelected = 0;

  KConfig *cfg = new KConfig ("kcookiejarrc");
  cfg->setGroup ("Cookie Policy");

  bool enableCookies = cfg->readBoolEntry("Cookies", true);
  m_cbEnableCookies->setChecked (enableCookies);
  cookiesEnabled( enableCookies );

  KCookieAdvice::Value advice = KCookieAdvice::strToAdvice (cfg->readEntry(
                                               "CookieGlobalAdvice", "Ask"));
  switch (advice)
  {
    case KCookieAdvice::Accept:
      m_rbPolicyAccept->setChecked (true);
      break;
    case KCookieAdvice::Reject:
      m_rbPolicyReject->setChecked (true);
      break;
    case KCookieAdvice::Ask:
    case KCookieAdvice::Dunno:
    default:
      m_rbPolicyAsk->setChecked (true);
  }

  bool enable = cfg->readBoolEntry("RejectCrossDomainCookies", true);
  m_cbRejectCrossDomainCookies->setChecked (enable);

  bool sessionCookies = cfg->readBoolEntry("AcceptSessionCookies", true);
  m_cbAutoAcceptSessionCookies->setChecked (sessionCookies);
  bool cookieExpiration = cfg->readBoolEntry("IgnoreExpirationDate", false);
  m_cbIgnoreCookieExpirationDate->setChecked (cookieExpiration);

  if (enableCookies)
  {
    ignoreCookieExpirationDate( cookieExpiration );
    autoAcceptSessionCookies( sessionCookies );
    updateDomainList(cfg->readListEntry("CookieDomainAdvice"));
    updateButtons();
  }

  delete cfg;

  // Connect the main swicth :) Enable/disable cookie support
  connect( m_cbEnableCookies, SIGNAL( toggled(bool) ),
           SLOT( cookiesEnabled(bool) ) );
  connect( m_cbEnableCookies, SIGNAL( clicked() ), SLOT( emitChanged() ) );

  // Connect the preference check boxes...
  connect( m_cbRejectCrossDomainCookies, SIGNAL( clicked() ),
           SLOT( emitChanged() ) );
  connect( m_cbAutoAcceptSessionCookies, SIGNAL( clicked() ),
           SLOT( emitChanged() ) );
  connect( m_cbIgnoreCookieExpirationDate, SIGNAL( clicked() ),
           SLOT( emitChanged() ) );
  connect ( m_cbAutoAcceptSessionCookies, SIGNAL(toggled(bool)),
            SLOT(autoAcceptSessionCookies(bool)));
  connect ( m_cbIgnoreCookieExpirationDate, SIGNAL(toggled(bool)),
            SLOT(ignoreCookieExpirationDate(bool)));

  // Connect the default cookie policy radio buttons...
  connect(m_bgDefault, SIGNAL(clicked(int)), SLOT(emitChanged()));

  // Connect signals from the domain specific policy listview.
  connect( m_lvDomainPolicy, SIGNAL(selectionChanged()),
           SLOT(selectionChanged()) );
  connect( m_lvDomainPolicy, SIGNAL(doubleClicked (QListViewItem *)),
           SLOT(changePressed() ) );
  connect( m_lvDomainPolicy, SIGNAL(returnPressed ( QListViewItem * )),
           SLOT(changePressed() ) );
}

void KCookiesPolicies::save()
{
  QString advice;
  QStringList domainConfig;

  KConfig *cfg = new KConfig( "kcookiejarrc" );
  cfg->setGroup( "Cookie Policy" );

  bool state = m_cbEnableCookies->isChecked();
  cfg->writeEntry( "Cookies", state );
  state = m_cbRejectCrossDomainCookies->isChecked();
  cfg->writeEntry( "RejectCrossDomainCookies", state );
  state = m_cbAutoAcceptSessionCookies->isChecked();
  cfg->writeEntry( "AcceptSessionCookies", state );
  state = m_cbIgnoreCookieExpirationDate->isChecked();
  cfg->writeEntry( "IgnoreExpirationDate", state );


  if (m_rbPolicyAccept->isChecked())
      advice = KCookieAdvice::adviceToStr(KCookieAdvice::Accept);
  else if (m_rbPolicyReject->isChecked())
      advice = KCookieAdvice::adviceToStr(KCookieAdvice::Reject);
  else
      advice = KCookieAdvice::adviceToStr(KCookieAdvice::Ask);

  cfg->writeEntry("CookieGlobalAdvice", advice);


  QListViewItem *at = m_lvDomainPolicy->firstChild();
  while( at )
  {
qWarning("Saving %s [%s]", KIDNA::toAscii(at->text(0)).latin1(), at->text(0).latin1());
    domainConfig.append(QString::fromLatin1("%1:%2").arg(KIDNA::toAscii(at->text(0))).arg(m_pDomainPolicy[at]));
    at = at->nextSibling();
  }
  cfg->writeEntry("CookieDomainAdvice", domainConfig);
  cfg->sync();
  delete cfg;

  // Update the cookiejar...
  DCOPClient *m_dcopClient = new DCOPClient();
  if( !m_dcopClient->attach() )
    kdDebug(7103) << "Can't connect with the DCOP server." << endl;
  else
  {
    if( m_cbEnableCookies->isChecked() )
     {
        if ( !m_dcopClient->send ("kded", "kcookiejar", "reloadPolicy()",
                                  QString::null) )
           kdDebug(7104) << "Can't communicate with the cookiejar!" << endl;
     }
     else
     {
        if ( !m_dcopClient->send ("kded", "kcookiejar", "shutdown()",
                                  QString::null) )
           kdDebug(7104) << "Can't communicate with the cookiejar!" << endl;
     }
  }
  delete m_dcopClient;

  KSaveIOConfig::updateRunningIOSlaves (this);

  emit changed( false );
}


void KCookiesPolicies::defaults()
{
  m_cbEnableCookies->setChecked( true );
  m_rbPolicyAsk->setChecked( true );
  m_rbPolicyAccept->setChecked( false );
  m_rbPolicyReject->setChecked( false );
  m_cbRejectCrossDomainCookies->setChecked( true );
  m_cbAutoAcceptSessionCookies->setChecked( true );
  m_cbIgnoreCookieExpirationDate->setChecked( false );
  m_lvDomainPolicy->clear();

  cookiesEnabled( m_cbEnableCookies->isChecked() );
  updateButtons();
}

void KCookiesPolicies::splitDomainAdvice (const QString& cfg, QString &domain,
                                          KCookieAdvice::Value &advice)
{
  int pos;

  pos = cfg.find(':');

  if ( pos == -1)
  {
    domain = cfg;
    advice = KCookieAdvice::Dunno;
  }
  else
  {
    domain = cfg.left(pos);
    advice = KCookieAdvice::strToAdvice (cfg.mid (pos+1, cfg.length()));
  }
}

QString KCookiesPolicies::quickHelp() const
{
  return i18n("<h1>Cookies</h1> Cookies contain information that Konqueror"
              " (or any other KDE application using the HTTP protocol) stores"
              " on your computer from a remote Internet server. This means"
              " that a web server can store information about you and your"
              " browsing activities on your machine for later use. You might"
              " consider this an invasion of privacy.<p>However, cookies are"
              " useful in certain situations. For example, they are often used"
              " by Internet shops, so you can 'put things into a shopping"
              " basket'. Some sites require you have a browser that supports"
              " cookies.<p>Because most people want a compromise between privacy"
              " and the benefits cookies offer, KDE offers you the ability to"
              " customize the way it handles cookies. You might, for example"
              " want to set KDE's default policy to ask you whenever a server"
              " wants to set a cookie or simply reject or accept everything."
              " For example, you might choose to accept all cookies from your"
              " favorite shopping web site. For this all you have to do is"
              " either browse to that particular site and when you are presented"
              " with the cookie dialog box, click on <i> This domain </i> under"
              " the 'apply to' tab and choose accept or simply specify the name"
              " of the site in the <i> Domain Specific Policy </i> tab and set"
              " it to accept. This enables you to receive cookies from trusted"
              " web sites without being asked every time KDE receives a cookie."
             );
}

#include "kcookiespolicies.moc"
