// -*- c-basic-offset: 2 -*-
/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// written by Graham Roff <graham@licq.org>
// contributions by Dirk A. Mueller <dirk@licq.org>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <qaccel.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qsplitter.h>
#include <qapplication.h>
#include <qpopupmenu.h>
#include <qtextcodec.h>
#include <qwhatsthis.h>

#ifdef USE_KDE
#include <kfiledialog.h>
#include <kcolordialog.h>
#else
#include <qfiledialog.h>
#include <qcolordialog.h>
#endif

#include "licq_message.h"
#include "licq_icqd.h"
#include "licq_log.h"
#include "usercodec.h"

#include "authuserdlg.h"
#include "ewidgets.h"
#include "mainwin.h"
#include "messagebox.h"
#include "mmlistview.h"
#include "mmsenddlg.h"
#include "chatdlg.h"
#include "chatjoin.h"
#include "eventdesc.h"
#include "filedlg.h"
#include "forwarddlg.h"
#include "usereventdlg.h"
#include "refusedlg.h"
#include "sigman.h"
#include "showawaymsgdlg.h"
#include "keyrequestdlg.h"
#include "xpm/chatChangeFg.xpm"
#include "xpm/chatChangeBg.xpm"

// -----------------------------------------------------------------------------

UserEventCommon::UserEventCommon(CICQDaemon *s, CSignalManager *theSigMan,
                                 CMainWindow *m, unsigned long _nUin,
                                 QWidget* parent, const char* name)
  : QWidget(parent, name, WDestructiveClose)
{
  server = s;
  mainwin = m;
  sigman = theSigMan;
  icqEventTag = 0;
  m_nUin = _nUin;
  m_bOwner = (m_nUin == gUserManager.OwnerUin());
  m_bDeleteUser = false;

  top_hlay = new QHBoxLayout(this, 6);
  top_lay = new QVBoxLayout(top_hlay);

  // initalize codec
  codec = QTextCodec::codecForLocale();

  QBoxLayout *layt = new QHBoxLayout(top_lay, 8);
  layt->addWidget(new QLabel(tr("Status:"), this));
  nfoStatus = new CInfoField(this, true);
  nfoStatus->setMinimumWidth(nfoStatus->sizeHint().width()+30);
  layt->addWidget(nfoStatus);
  layt->addWidget(new QLabel(tr("Time:"), this));
  nfoTimezone = new CInfoField(this, true);
  nfoTimezone->setMinimumWidth(nfoTimezone->sizeHint().width()/2+10);
  layt->addWidget(nfoTimezone);

  popupEncoding = new QPopupMenu(this);
  btnSecure = new QPushButton(this);
  QToolTip::add(btnSecure, tr("Secure channel information"));
  layt->addWidget(btnSecure);
  connect(btnSecure, SIGNAL(clicked()), this, SLOT(slot_security()));
  btnHistory = new QPushButton(this);
  btnHistory->setPixmap(mainwin->pmHistory);
  QToolTip::add(btnHistory, tr("Show User History"));
  connect(btnHistory, SIGNAL(clicked()), this, SLOT(showHistory()));
  layt->addWidget(btnHistory);
  btnInfo = new QPushButton(this);
  btnInfo->setPixmap(mainwin->pmInfo);
  QToolTip::add(btnInfo, tr("Show User Info"));
  connect(btnInfo, SIGNAL(clicked()), this, SLOT(showUserInfo()));
  layt->addWidget(btnInfo);
  btnEncoding = new QPushButton(this);
  btnEncoding->setPixmap(mainwin->pmEncoding);
  QToolTip::add(btnEncoding, tr("Change user text encoding"));
  QWhatsThis::add(btnEncoding, tr("This button selects the text encoding used when communicating with this user. You might need to change the encoding to communicate in a different language."));
  btnEncoding->setPopup(popupEncoding);

  layt->addWidget(btnEncoding);

  tmrTime = NULL;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
  if (u != NULL)
  {
    nfoStatus->setData(u->StatusStr());
    if (u->NewMessages() == 0)
      setIcon(CMainWindow::iconForStatus(u->StatusFull()));
    else
      setIcon(CMainWindow::iconForEvent(ICQ_CMDxSUB_MSG));
    SetGeneralInfo(u);

    // restore prefered encoding
    codec = UserCodec::codecForICQUser(u);

    gUserManager.DropUser(u);
  }

  QString codec_name = QString( codec->name() ).lower();
  popupEncoding->setCheckable(true);
  QStringList enc = UserCodec::encodings();
  for (uint i=0; i < enc.count(); i++) {
    popupEncoding->insertItem(enc[i], this, SLOT(slot_setEncoding(int)), 0, i);
    if (UserCodec::encodingForName(enc[i]).lower() == codec_name)
      popupEncoding->setItemChecked(i, true);
  }

  connect (sigman, SIGNAL(signal_updatedUser(CICQSignal *)),
           this, SLOT(slot_userupdated(CICQSignal *)));

  mainWidget = new QWidget(this);
  top_lay->addWidget(mainWidget);
}

void UserEventCommon::slot_setEncoding(int encoding_index) {
  /* uncheck all encodings */
  for (unsigned int i=0; i<popupEncoding->count(); i++) {
    popupEncoding->setItemChecked(popupEncoding->idAt(i), false);
  }

  /* initialize a codec according to the encoding menu's value */
  if ((encoding_index >= 0) && ((uint)encoding_index < UserCodec::encodings().count())) {
    codec = QTextCodec::codecForName(UserCodec::encodingForIndex((uint) encoding_index).latin1());

    /* make the chosen encoding checked */
    popupEncoding->setItemChecked(encoding_index, true);

    /* save prefered character set */
    ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
    if (u != NULL) {
      u->SetEnableSave(false);
      u->SetUserEncoding( codec->name() );
      u->SetEnableSave(true);
      u->SaveLicqInfo();
      gUserManager.DropUser(u);
    }

    emit encodingChanged();
  }
}

//-----UserEventCommon::SetGeneralInfo---------------------------------------
void UserEventCommon::SetGeneralInfo(ICQUser *u)
{
  QTextCodec * codec = UserCodec::codecForICQUser(u);

  if (u->GetTimezone() == TIMEZONE_UNKNOWN)
    nfoTimezone->setText(tr("Unknown"));
  else
  {
    m_nRemoteTimeOffset = u->LocalTimeOffset();
    QDateTime t;
    t.setTime_t(u->LocalTime());
    nfoTimezone->setText(t.time().toString());
    /*nfoTimezone->setText(tr("%1 (GMT%1%1%1)")
                         .arg(t.time().toString())
                         .arg(u->GetTimezone() > 0 ? "-" : "+")
                         .arg(abs(u->GetTimezone() / 2))
                         .arg(u->GetTimezone() % 2 ? "30" : "00") );*/
    if (tmrTime == NULL)
    {
      tmrTime = new QTimer(this);
      connect(tmrTime, SIGNAL(timeout()), this, SLOT(slot_updatetime()));
      tmrTime->start(3000);
    }
  }

  if (u->Secure())
    btnSecure->setPixmap(mainwin->pmSecureOn);
  else
    btnSecure->setPixmap(mainwin->pmSecureOff);

  m_sBaseTitle = codec->toUnicode(u->GetAlias()) + " (" +
             codec->toUnicode(u->GetFirstName()) + " " +
             codec->toUnicode(u->GetLastName())+ ")";
  setCaption(m_sBaseTitle);
  setIconText(codec->toUnicode(u->GetAlias()));
}


void UserEventCommon::slot_updatetime()
{
  QDateTime t;
  t.setTime_t(time(NULL) + m_nRemoteTimeOffset);
  nfoTimezone->setText(t.time().toString());
  //nfoTimezone->setText(nfoTimezone->text().replace(0, t.time().toString().length(), t.time().toString()));
}


UserEventCommon::~UserEventCommon()
{
  emit finished(m_nUin);

  if (m_bDeleteUser && !m_bOwner)
    mainwin->RemoveUserFromList(m_nUin, this);
}


//-----UserEventCommon::slot_userupdated-------------------------------------
void UserEventCommon::slot_userupdated(CICQSignal *sig)
{
  if (m_nUin != sig->Uin()) return;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u == NULL) return;

  switch (sig->SubSignal())
  {
    case USER_STATUS:
    {
      nfoStatus->setData(u->StatusStr());
      if (u->NewMessages() == 0)
        setIcon(CMainWindow::iconForStatus(u->StatusFull()));
      break;
    }
    case USER_GENERAL:
    case USER_SECURITY:
    case USER_BASIC:
    {
      SetGeneralInfo(u);
      break;
    }
    case USER_EVENTS:
    {
      if (u->NewMessages() == 0)
        setIcon(CMainWindow::iconForStatus(u->StatusFull()));
      else
        setIcon(CMainWindow::iconForEvent(ICQ_CMDxSUB_MSG));
      break;
    }
  }
  // Call the event specific function now
  UserUpdated(sig, u);

  gUserManager.DropUser(u);
}


void UserEventCommon::showHistory()
{
  mainwin->callInfoTab(mnuUserHistory, m_nUin, true);
}


void UserEventCommon::showUserInfo()
{
  mainwin->callInfoTab(mnuUserGeneral, m_nUin, true);
}

void UserEventCommon::slot_security()
{
  (void) new KeyRequestDlg(sigman, m_nUin);
}


//=====UserViewEvent=========================================================
UserViewEvent::UserViewEvent(CICQDaemon *s, CSignalManager *theSigMan,
                             CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserEventCommon(s, theSigMan, m, _nUin, parent, "UserViewEvent")
{
  QBoxLayout* lay = new QVBoxLayout(mainWidget);
  splRead = new QSplitter(Vertical, mainWidget);
  lay->addWidget(splRead);
  splRead->setOpaqueResize();

  QAccel *a = new QAccel( this );
  a->connectItem(a->insertItem(Key_Escape), this, SLOT(close()));

  msgView = new MsgView(splRead);
  mleRead = new MLEditWrap(true, splRead, true);
  mleRead->setReadOnly(true);
  splRead->setResizeMode(msgView, QSplitter::FollowSizeHint);
  splRead->setResizeMode(mleRead, QSplitter::Stretch);

  connect (msgView, SIGNAL(currentChanged(QListViewItem *)), this, SLOT(slot_printMessage(QListViewItem *)));
  connect (mainwin, SIGNAL(signal_sentevent(ICQEvent *)), this, SLOT(slot_sentevent(ICQEvent *)));

  QHGroupBox *h_action = new QHGroupBox(mainWidget);
  lay->addSpacing(10);
  lay->addWidget(h_action);
  btnRead1 = new CEButton(h_action);
  btnRead2 = new QPushButton(h_action);
  btnRead3 = new QPushButton(h_action);
  btnRead4 = new QPushButton(h_action);

  btnRead1->setEnabled(false);
  btnRead2->setEnabled(false);
  btnRead3->setEnabled(false);
  btnRead4->setEnabled(false);

  connect(btnRead1, SIGNAL(clicked()), this, SLOT(slot_btnRead1()));
  connect(btnRead2, SIGNAL(clicked()), this, SLOT(slot_btnRead2()));
  connect(btnRead3, SIGNAL(clicked()), this, SLOT(slot_btnRead3()));
  connect(btnRead4, SIGNAL(clicked()), this, SLOT(slot_btnRead4()));

  QBoxLayout *h_lay = new QHBoxLayout(top_lay, 4);
  if (!m_bOwner)
  {
    QPushButton *btnMenu = new QPushButton(tr("&Menu"), this);
    h_lay->addWidget(btnMenu);
    connect(btnMenu, SIGNAL(pressed()), this, SLOT(slot_usermenu()));
    btnMenu->setPopup(mainwin->UserMenu());
    chkAutoClose = new QCheckBox(tr("Aut&o Close"), this);
    chkAutoClose->setChecked(mainwin->m_bAutoClose);
    h_lay->addWidget(chkAutoClose);
  }
  h_lay->addStretch(1);
  int bw = 75;
  btnReadNext = new QPushButton(tr("Nex&t"), this);
  setTabOrder(btnRead4, btnReadNext);
  btnClose = new CEButton(tr("&Close"), this);
  QToolTip::add(btnClose, tr("Normal Click - Close Window\n<CTRL>+Click - also delete User"));
  setTabOrder(btnReadNext, btnClose);
  bw = QMAX(bw, btnReadNext->sizeHint().width());
  bw = QMAX(bw, btnClose->sizeHint().width());
  btnReadNext->setFixedWidth(bw);
  btnClose->setFixedWidth(bw);
  h_lay->addWidget(btnReadNext);
  btnReadNext->setEnabled(false);
  connect(btnReadNext, SIGNAL(clicked()), this, SLOT(slot_btnReadNext()));
  connect(btnClose, SIGNAL(clicked()), SLOT(slot_close()));
  h_lay->addWidget(btnClose);

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u != NULL && u->NewMessages() > 0)
  {
    MsgViewItem *e = new MsgViewItem(u->EventPeek(0), msgView);
    for (unsigned short i = 1; i < u->NewMessages(); i++)
    {
      (void) new MsgViewItem(u->EventPeek(i), msgView);
    }
    gUserManager.DropUser(u);
    slot_printMessage(e);
    msgView->setSelected(e, true);
    msgView->ensureItemVisible(e);
  }
  else
    gUserManager.DropUser(u);

  connect(this, SIGNAL(encodingChanged()), this, SLOT(slot_setEncoding()));
}


void UserViewEvent::slot_setEncoding() {
  /* if we have an open view, refresh it */
  if (this->msgView) {
    slot_printMessage(this->msgView->selectedItem());
  }
}

UserViewEvent::~UserViewEvent()
{
}


void UserViewEvent::slot_close()
{
  m_bDeleteUser = btnClose->stateWhenPressed() & ControlButton;
  close();
}


//-----UserViewEvent::slot_autoClose-----------------------------------------
void UserViewEvent::slot_autoClose()
{
  if(!chkAutoClose->isChecked()) return;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  bool doclose = (u->NewMessages() == 0);
  gUserManager.DropUser(u);

  if(doclose)
    close();
}

//-----UserViewEvent::updateNextButton---------------------------------------
void UserViewEvent::updateNextButton()
{
  int num = 0;

  MsgViewItem *it = static_cast<MsgViewItem*>(msgView->firstChild());
  MsgViewItem *e = NULL;

  while (it)
  {
    if (it->m_nEventId != -1 && it->msg->Direction() == D_RECEIVER)
    {
      e = it;
      num++;
    }
    it = static_cast<MsgViewItem*>(it->nextSibling());
  }

  btnReadNext->setEnabled(num > 0);

  if (num > 1)
    btnReadNext->setText(tr("Nex&t (%1)").arg(num));
  else if (num == 1)
    btnReadNext->setText(tr("Nex&t"));

  if(e && e->msg)
    btnReadNext->setIconSet(CMainWindow::iconForEvent(e->msg->SubCommand()));
}


//-----UserViewEvent::slot_printMessage--------------------------------------
void UserViewEvent::slot_printMessage(QListViewItem *eq)
{
  if (eq == NULL) return;

  MsgViewItem *e = (MsgViewItem *)eq;

  btnRead1->setText("");
  btnRead2->setText("");
  btnRead3->setText("");
  btnRead4->setText("");
  btnRead1->setEnabled(false);
  btnRead2->setEnabled(false);
  btnRead3->setEnabled(false);
  btnRead4->setEnabled(false);

  CUserEvent *m = e->msg;
  m_xCurrentReadEvent = m;
  // Set the color for the message
  mleRead->setBackground(QColor(m->Color()->BackRed(), m->Color()->BackGreen(), m->Color()->BackBlue()));
  mleRead->setForeground(QColor(m->Color()->ForeRed(), m->Color()->ForeGreen(), m->Color()->ForeBlue()));
  // Set the text
  mleRead->setText(codec->toUnicode(m->Text()));
  mleRead->setCursorPosition(0, 0);

  if (m->Direction() == D_RECEIVER && (m->Command() == ICQ_CMDxTCP_START || m->Command() == ICQ_CMDxRCV_SYSxMSGxONLINE))
  {
    switch (m->SubCommand())
    {
      case ICQ_CMDxSUB_CHAT:  // accept or refuse a chat request
      case ICQ_CMDxSUB_FILE:  // accept or refuse a file transfer
        btnRead1->setText(tr("&Reply"));
        if (m->IsCancelled())
        {
          mleRead->append(tr("\n--------------------\nRequest was cancelled."));
        }
        else
        {
          if (m->Pending())
          {
            btnRead2->setText(tr("A&ccept"));
            btnRead3->setText(tr("&Refuse"));
          }
          // If this is a chat, and we already have chats going, and this is
          // not a join request, then we can join
          if (m->SubCommand() == ICQ_CMDxSUB_CHAT &&
              ChatDlg::chatDlgs.size() > 0 &&
              ((CEventChat *)m)->Port() == 0)
            btnRead4->setText(tr("&Join"));
        }
        break;

      case ICQ_CMDxSUB_MSG:
        btnRead1->setText(tr("&Reply"));
        btnRead2->setText(tr("&Quote"));
        btnRead3->setText(tr("&Forward"));
        btnRead4->setText(tr("Start Chat"));
        break;

      case ICQ_CMDxSUB_URL:   // view a url
        btnRead1->setText(tr("&Reply"));
        btnRead2->setText(tr("&Quote"));
        btnRead3->setText(tr("&Forward"));
        if (server->getUrlViewer() != NULL)
          btnRead4->setText(tr("&View"));
        break;

      case ICQ_CMDxSUB_AUTHxREQUEST:
      {
        btnRead1->setText(tr("A&uthorize"));
        btnRead2->setText(tr("&Refuse"));
        ICQUser *u = gUserManager.FetchUser( ((CEventAuthRequest *)m)->Uin(), LOCK_R);
        if (u == NULL)
          btnRead3->setText(tr("A&dd User"));
        else
          gUserManager.DropUser(u);
        break;
      }
      case ICQ_CMDxSUB_AUTHxGRANTED:
      {
        ICQUser *u = gUserManager.FetchUser( ((CEventAuthGranted *)m)->Uin(), LOCK_R);
        if (u == NULL)
          btnRead1->setText(tr("A&dd User"));
        else
          gUserManager.DropUser(u);
        break;
      }
      case ICQ_CMDxSUB_ADDEDxTOxLIST:
      {
        ICQUser *u = gUserManager.FetchUser( ((CEventAdded *)m)->Uin(), LOCK_R);
        if (u == NULL)
          btnRead1->setText(tr("A&dd User"));
        else
          gUserManager.DropUser(u);
        break;
      }
      case ICQ_CMDxSUB_CONTACTxLIST:
      {
        int s = static_cast<CEventContactList*>(m)->Contacts().size();
        if(s > 1)
          btnRead1->setText(tr("A&dd %1 Users").arg(s));
        else if(s == 1)
          btnRead1->setText(tr("A&dd User"));
        break;
      }
    } // switch
  }  // if

  if (!btnRead1->text().isEmpty()) btnRead1->setEnabled(true);
  if (!btnRead2->text().isEmpty()) btnRead2->setEnabled(true);
  if (!btnRead3->text().isEmpty()) btnRead3->setEnabled(true);
  if (!btnRead4->text().isEmpty()) btnRead4->setEnabled(true);

  btnRead1->setFocus();

  if (e->m_nEventId != -1 && e->msg->Direction() == D_RECEIVER)
  {
    ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
    u->EventClearId(e->m_nEventId);
    gUserManager.DropUser(u);
    e->MarkRead();
  }
}


void UserViewEvent::generateReply()
{
  QString s;
  if (mleRead->hasMarkedText())
  {
    s = QString("> ") + mleRead->markedText();
    s.replace( QRegExp("\n"), "\n> ");
  }
  else
  {
    for (int i = 0; i < mleRead->numLines(); i++)
      if(!mleRead->textLine(i).stripWhiteSpace().isEmpty())
        s += QString("> ") + mleRead->textLine(i) + " \n";
      else
        s += "\n";
  }

  s = s.stripWhiteSpace()+ " \n";

  sendMsg(s);
}


void UserViewEvent::sendMsg(QString txt)
{
  UserSendMsgEvent *e = new UserSendMsgEvent(server, sigman, mainwin, m_nUin);
  e->setText(txt);

  // Find a good position for the new window
  if (mainwin->m_bAutoPosReplyWin)
  {
    int yp = btnRead1->parentWidget()->mapToGlobal(QPoint(0, 0)).y();
    if (yp + e->height() + 8 > QApplication::desktop()->height())
      yp = QApplication::desktop()->height() - e->height() - 8;
    e->move(x(), yp);
  }
  QTimer::singleShot( 10, e, SLOT( show() ) );

  connect(e, SIGNAL(autoCloseNotify()), this, SLOT(slot_autoClose()));
  connect(e, SIGNAL(signal_msgtypechanged(UserSendCommon *, UserSendCommon *)),
          this, SLOT(slot_msgtypechanged(UserSendCommon *, UserSendCommon *)));
}

void UserViewEvent::slot_msgtypechanged(UserSendCommon *old_t, UserSendCommon *new_t)
{
  disconnect(old_t, SIGNAL(autoCloseNotify()), this, SLOT(slot_autoClose()));
  disconnect(old_t, SIGNAL(signal_msgtypechanged(UserSendCommon *, UserSendCommon *)),
          this, SLOT(slot_msgtypechanged(UserSendCommon *, UserSendCommon *)));

  connect(new_t, SIGNAL(autoCloseNotify()), this, SLOT(slot_autoClose()));
  connect(new_t, SIGNAL(signal_msgtypechanged(UserSendCommon *, UserSendCommon *)),
          this, SLOT(slot_msgtypechanged(UserSendCommon *, UserSendCommon *)));
}


void UserViewEvent::slot_btnRead1()
{
  if (m_xCurrentReadEvent == NULL) return;

  switch (m_xCurrentReadEvent->SubCommand())
  {
    case ICQ_CMDxSUB_MSG:  // reply/quote
    case ICQ_CMDxSUB_URL:
    case ICQ_CMDxSUB_CHAT:
    case ICQ_CMDxSUB_FILE:
      sendMsg("");
      break;

    case ICQ_CMDxSUB_AUTHxREQUEST:
      (void) new AuthUserDlg(server, ((CEventAuthRequest *)m_xCurrentReadEvent)->Uin(), true);
      break;

    case ICQ_CMDxSUB_AUTHxGRANTED:
      server->AddUserToList( ((CEventAuthGranted *)m_xCurrentReadEvent)->Uin());
      break;

    case ICQ_CMDxSUB_ADDEDxTOxLIST:
      server->AddUserToList( ((CEventAdded *)m_xCurrentReadEvent)->Uin());
      break;
    case ICQ_CMDxSUB_CONTACTxLIST:
    {
      const ContactList& cl = static_cast<CEventContactList*>(m_xCurrentReadEvent)->Contacts();

      ContactList::const_iterator it;
      for(it = cl.begin(); it != cl.end(); ++it) {
        ICQUser* u = gUserManager.FetchUser((*it)->Uin(), LOCK_R);
        if(u == NULL)
          server->AddUserToList((*it)->Uin());
        gUserManager.DropUser(u);
      }
      btnRead1->setEnabled(false);
    }
  } // switch
}

void UserViewEvent::slot_btnRead2()
{
  if (m_xCurrentReadEvent == NULL) return;

  switch (m_xCurrentReadEvent->SubCommand())
  {
    case ICQ_CMDxSUB_MSG:  // quote
    case ICQ_CMDxSUB_URL:
      generateReply();
      break;

    case ICQ_CMDxSUB_CHAT:  // accept a chat request
    {
      m_xCurrentReadEvent->SetPending(false);
      btnRead2->setEnabled(false);
      btnRead3->setEnabled(false);
      CEventChat *c = (CEventChat *)m_xCurrentReadEvent;
      ChatDlg *chatDlg = new ChatDlg(m_nUin, server, mainwin);
      if (c->Port() != 0)  // Joining a multiparty chat (we connect to them)
      {
        if (chatDlg->StartAsClient(c->Port()))
          server->icqChatRequestAccept(m_nUin, chatDlg->LocalPort(), c->Sequence());
      }
      else  // single party (other side connects to us)
      {
        if (chatDlg->StartAsServer())
          server->icqChatRequestAccept(m_nUin, chatDlg->LocalPort(), c->Sequence());
      }
      break;
    }

    case ICQ_CMDxSUB_FILE:  // accept a file transfer
    {
      m_xCurrentReadEvent->SetPending(false);
      btnRead2->setEnabled(false);
      btnRead3->setEnabled(false);
      CEventFile *f = (CEventFile *)m_xCurrentReadEvent;
      CFileDlg *fileDlg = new CFileDlg(m_nUin, server);
      if (fileDlg->ReceiveFiles())
        server->icqFileTransferAccept(m_nUin, fileDlg->LocalPort(), f->Sequence());
      break;
    }

    case ICQ_CMDxSUB_AUTHxREQUEST:
    {
      (void) new AuthUserDlg(server, ((CEventAuthRequest *)m_xCurrentReadEvent)->Uin(), false);
      break;
    }
  } // switch

}


void UserViewEvent::slot_btnRead3()
{
  if (m_xCurrentReadEvent == NULL) return;

  switch (m_xCurrentReadEvent->SubCommand())
  {
    case ICQ_CMDxSUB_MSG:  // Forward
    case ICQ_CMDxSUB_URL:
    {
      CForwardDlg *f = new CForwardDlg(sigman, m_xCurrentReadEvent, this);
      f->show();
      break;
    }

    case ICQ_CMDxSUB_CHAT:  // refuse a chat request
    {
      CRefuseDlg *r = new CRefuseDlg(m_nUin, tr("Chat"), this);
      if (r->exec())
      {
        m_xCurrentReadEvent->SetPending(false);
        btnRead2->setEnabled(false);
        btnRead3->setEnabled(false);
        server->icqChatRequestRefuse(m_nUin, codec->fromUnicode(r->RefuseMessage()),
           m_xCurrentReadEvent->Sequence());
      }
      delete r;
      break;
    }

    case ICQ_CMDxSUB_FILE:  // refuse a file transfer
    {
      CRefuseDlg *r = new CRefuseDlg(m_nUin, tr("File Transfer"), this);
      if (r->exec())
      {
        m_xCurrentReadEvent->SetPending(false);
        btnRead2->setEnabled(false);
        btnRead3->setEnabled(false);
        server->icqFileTransferRefuse(m_nUin, codec->fromUnicode(r->RefuseMessage()),
           m_xCurrentReadEvent->Sequence());
      }
      delete r;
      break;
    }

    case ICQ_CMDxSUB_AUTHxREQUEST:
      server->AddUserToList( ((CEventAuthRequest *)m_xCurrentReadEvent)->Uin());
      break;

  }
}


void UserViewEvent::slot_btnRead4()
{
  if (m_xCurrentReadEvent == NULL) return;

  switch (m_xCurrentReadEvent->SubCommand())
  {
    case ICQ_CMDxSUB_MSG:
      mainwin->callFunction(mnuUserSendChat, Uin());
      break;
    case ICQ_CMDxSUB_CHAT:  // join to current chat
    {
      CEventChat *c = (CEventChat *)m_xCurrentReadEvent;
      if (c->Port() != 0)  // Joining a multiparty chat (we connect to them)
      {
        ChatDlg *chatDlg = new ChatDlg(m_nUin, server, mainwin);
        if (chatDlg->StartAsClient(c->Port()))
          server->icqChatRequestAccept(m_nUin, chatDlg->LocalPort(), c->Sequence());
      }
      else  // single party (other side connects to us)
      {
        ChatDlg *chatDlg = NULL;
        CJoinChatDlg *j = new CJoinChatDlg(this);
        if (j->exec() && (chatDlg = j->JoinedChat()) != NULL)
          server->icqChatRequestAccept(m_nUin, chatDlg->LocalPort(), c->Sequence());
        delete j;
      }
      break;
    }
    case ICQ_CMDxSUB_URL:   // view a url
      if (!server->ViewUrl(((CEventUrl *)m_xCurrentReadEvent)->Url()))
        WarnUser(this, tr("View URL failed"));
      break;
  }
}


void UserViewEvent::slot_btnReadNext()
{
  MsgViewItem *it = static_cast<MsgViewItem*>(msgView->firstChild());
  MsgViewItem *e = NULL;

  while (it)
  {
    if (it->m_nEventId != -1 && it->msg->Direction() == D_RECEIVER)
    {
      e = it;
    }
    it = static_cast<MsgViewItem*>(it->nextSibling());
  }

  updateNextButton();

  if (e != NULL)
  {
    msgView->setSelected(e, true);
    msgView->ensureItemVisible(e);
    slot_printMessage(e);
  }
}


void UserViewEvent::UserUpdated(CICQSignal *sig, ICQUser *u)
{
  if (sig->SubSignal() == USER_EVENTS)
  {
    CUserEvent* e = NULL;

    if (sig->Argument() > 0)
    {
      e = u->EventPeekId(sig->Argument());
      if (e != NULL)
      {
        MsgViewItem *m = new MsgViewItem(e, msgView);
        msgView->ensureItemVisible(m);
      }
    }

    if (sig->Argument() != 0) updateNextButton();
  }
}


void UserViewEvent::slot_sentevent(ICQEvent *e)
{
  if (e->Uin() != m_nUin) return;
  (void) new MsgViewItem(e->GrabUserEvent(), msgView);
}


//=====UserSendCommon========================================================
UserSendCommon::UserSendCommon(CICQDaemon *s, CSignalManager *theSigMan,
                               CMainWindow *m, unsigned long _nUin, QWidget* parent, const char* name)
  : UserEventCommon(s, theSigMan, m, _nUin, parent, name)
{
  grpMR = NULL;

  QAccel *a = new QAccel( this );
  a->connectItem(a->insertItem(Key_Escape), this, SLOT(cancelSend()));

  QGroupBox *box = new QGroupBox(this);
  top_lay->addWidget(box);
  QBoxLayout *vlay = new QVBoxLayout(box, 10, 5);
  QBoxLayout *hlay = new QHBoxLayout(vlay);
  chkSendServer = new QCheckBox(tr("Se&nd through server"), box);
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  chkSendServer->setChecked(u->SendServer() || (u->StatusOffline() && u->SocketDesc() == -1));
  if( (u->GetInGroup(GROUPS_SYSTEM, GROUP_INVISIBLE_LIST)) ||
      (u->Ip() == 0 && u->SocketDesc() == -1))
  {
    chkSendServer->setChecked(true);
    chkSendServer->setEnabled(false);
  }
  gUserManager.DropUser(u);
  hlay->addWidget(chkSendServer);
  chkUrgent = new QCheckBox(tr("U&rgent"), box);
  hlay->addWidget(chkUrgent);
  chkMass = new QCheckBox(tr("M&ultiple recipients"), box);
  hlay->addWidget(chkMass);
  connect(chkMass, SIGNAL(toggled(bool)), this, SLOT(massMessageToggled(bool)));
  btnForeColor = new QPushButton(box);
  btnForeColor->setPixmap(chatChangeFg_xpm);
  connect(btnForeColor, SIGNAL(clicked()), this, SLOT(slot_SetForegroundICQColor()));
  hlay->addWidget(btnForeColor);
  btnBackColor = new QPushButton(box);
  btnBackColor->setPixmap(chatChangeBg_xpm);
  connect(btnBackColor, SIGNAL(clicked()), this, SLOT(slot_SetBackgroundICQColor()));
  hlay->addWidget(btnBackColor);

  QBoxLayout *h_lay = new QHBoxLayout(top_lay);
  if (!m_bOwner)
  {
    QPushButton *btnMenu = new QPushButton(tr("&Menu"), this);
    h_lay->addWidget(btnMenu);
    connect(btnMenu, SIGNAL(pressed()), this, SLOT(slot_usermenu()));
    btnMenu->setPopup(mainwin->UserMenu());
  }
  cmbSendType = new QComboBox(this);
  cmbSendType->insertItem(tr("Message"));
  cmbSendType->insertItem(tr("URL"));
  cmbSendType->insertItem(tr("Chat Request"));
  cmbSendType->insertItem(tr("File Transfer"));
  cmbSendType->insertItem(tr("Contact List"));
  connect(cmbSendType, SIGNAL(activated(int)), this, SLOT(changeEventType(int)));
  h_lay->addWidget(cmbSendType);
  h_lay->addStretch(1);
  btnSend = new QPushButton(tr("&Send"), this);
  int w = QMAX(btnSend->sizeHint().width(), 75);
  // add a wrapper around the send button that
  // tries to establish a secure connection first.
  connect( btnSend, SIGNAL( clicked() ), this, SLOT( trySecure() ) );

  btnCancel = new QPushButton(tr("&Close"), this);
  w = QMAX(btnCancel->sizeHint().width(), w);
  btnSend->setFixedWidth(w);
  btnCancel->setFixedWidth(w);
  h_lay->addWidget(btnSend);
  h_lay->addWidget(btnCancel);
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(cancelSend()));
  splView = new QSplitter(Vertical, mainWidget);
  //splView->setOpaqueResize();
  mleHistory=0;
  if (mainwin->m_bMsgChatView) {
    mleHistory = new CMessageViewWidget(_nUin,splView);
    connect (mainwin, SIGNAL(signal_sentevent(ICQEvent *)), mleHistory, SLOT(addMsg(ICQEvent *)));
    //splView->setResizeMode(mleHistory, QSplitter::FollowSizeHint);
  }
  mleSend = new MLEditWrap(true, splView, true);
  //splView->setResizeMode(mleSend, QSplitter::Stretch);
  setTabOrder(mleSend, btnSend);
  setTabOrder(btnSend, btnCancel);
  icqColor.SetToDefault();
  mleSend->setBackground(QColor(icqColor.BackRed(), icqColor.BackGreen(), icqColor.BackBlue()));
  mleSend->setForeground(QColor(icqColor.ForeRed(), icqColor.ForeGreen(), icqColor.ForeBlue()));
  connect (mleSend, SIGNAL(signal_CtrlEnterPressed()), btnSend, SIGNAL(clicked()));
  connect(this, SIGNAL(updateUser(CICQSignal*)), mainwin, SLOT(slot_updatedUser(CICQSignal*)));
}


UserSendCommon::~UserSendCommon()
{
}

//-----UserSendCommon::slot_SetForegroundColor-------------------------------
void UserSendCommon::slot_SetForegroundICQColor()
{
#ifdef USE_KDE
  QColor c = mleSend->foregroundColor();
  if (KColorDialog::getColor(c, this) != KColorDialog::Accepted) return;
#else
  QColor c = QColorDialog::getColor(mleSend->foregroundColor(), this);
  if (!c.isValid()) return;
#endif

  icqColor.SetForeground(c.red(), c.green(), c.blue());
  mleSend->setForeground(c);
}

void UserSendCommon::trySecure()
{
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  bool autoSecure = ( u->AutoSecure() && gLicqDaemon->CryptoEnabled() &&
                      (u->SecureChannelSupport() == SECURE_CHANNEL_SUPPORTED ) &&
                      !chkSendServer->isChecked() && !u->Secure());
  gUserManager.DropUser( u );
  disconnect( btnSend, SIGNAL( clicked() ), this, SLOT( trySecure() ) );
  connect(btnSend, SIGNAL(clicked()), this, SLOT(sendButton()));
  if ( autoSecure ) {
    QWidget* w = new KeyRequestDlg(sigman, m_nUin);
    connect(w, SIGNAL( destroyed() ), this, SLOT( sendButton() ) );
  }
  else
    sendButton();
}

//-----UserSendCommon::slot_SetBackgroundColor-------------------------------
void UserSendCommon::slot_SetBackgroundICQColor()
{
#ifdef USE_KDE
  QColor c = mleSend->backgroundColor();
  if (KColorDialog::getColor(c, this) != KColorDialog::Accepted) return;
#else
  QColor c = QColorDialog::getColor(mleSend->backgroundColor(), this);
  if (!c.isValid()) return;
#endif

  icqColor.SetBackground(c.red(), c.green(), c.blue());
  mleSend->setBackground(c);
}


//-----UserSendCommon::changeEventType---------------------------------------
void UserSendCommon::changeEventType(int id)
{
  UserSendCommon* e = NULL;
  switch(id)
  {
  case 0:
    e = new UserSendMsgEvent(server, sigman, mainwin, m_nUin);
    break;
  case 1:
    e = new UserSendUrlEvent(server, sigman, mainwin, m_nUin);
    break;
  case 2:
    e = new UserSendChatEvent(server, sigman, mainwin, m_nUin);
    break;
  case 3:
    e = new UserSendFileEvent(server, sigman, mainwin, m_nUin);
    break;
  case 4:
    e = new UserSendContactEvent(server, sigman, mainwin, m_nUin);
    break;
  }

  if (e != NULL)
  {
    QPoint p = topLevelWidget()->pos();
    if (e->mleSend && mleSend)
      e->mleSend->setText(mleSend->text());
    if (e->mleHistory && mleHistory){
      e->mleHistory->setText(mleHistory->text());
      e->mleHistory->GotoEnd();
    }
    e->move(p);

    emit signal_msgtypechanged(this, e);

    QTimer::singleShot( 10, e, SLOT( show() ) );

    QTimer::singleShot( 100, this, SLOT( close() ) );
  }
}


// -----------------------------------------------------------------------------
QCString UserSendCommon::generatePart(const QString& text)
{
#define PARTLEN (MAX_MESSAGE_SIZE - 14)

  QCString msgTextCurrent = codec->fromUnicode(text);

  if (chkSendServer->isChecked() && msgTextCurrent.length() > PARTLEN)
  {
    QString after_cut = codec->toUnicode(msgTextCurrent.left(PARTLEN));

    // we try to find the optimal place to cut
    // (according to our narrow-minded Latin1 idea of optimal :)
    int found_index = after_cut.findRev(QRegExp("[\\s\\.]"));
    if(found_index > 0)
      msgTextCurrent = codec->fromUnicode(after_cut.left(found_index));
  }

  return msgTextCurrent;
}

//-----UserSendCommon::massMessageToggled------------------------------------
void UserSendCommon::massMessageToggled(bool b)
{
  if (grpMR == NULL)
  {
    grpMR = new QVGroupBox(this);
    top_hlay->addWidget(grpMR);

    (void) new QLabel(tr("Drag Users Here\nRight Click for Options"), grpMR);
    lstMultipleRecipients = new CMMUserView(mainwin->colInfo, mainwin->m_bShowHeader,
                                            m_nUin, mainwin, grpMR);
    lstMultipleRecipients->setFixedWidth(mainwin->UserView()->width());
  }

  if (b)
  {
    grpMR->show();
  }
  else
  {
    // doesn't work right TODO investigate why
    int w = grpMR->width();
    grpMR->hide();
    //resize(width()-w, height());
    top_hlay->setGeometry(QRect(0, 0, width()-w, height()));
  }
}


//-----UserSendCommon::sendButton--------------------------------------------
void UserSendCommon::sendButton()
{
  if(!mainwin->m_bManualNewUser) {
    ICQUser* u = gUserManager.FetchUser(m_nUin, LOCK_W);

    if(u->NewUser())
    {
      u->SetNewUser(false);
      gUserManager.DropUser(u);

      CICQSignal s(SIGNAL_UPDATExUSER, USER_BASIC, m_nUin);
      emit updateUser(&s);
    }
    else
      gUserManager.DropUser(u);
  }

  if (icqEventTag != 0)
  {
    m_sProgressMsg = tr("Sending ");
    bool via_server = chkSendServer->isChecked();
    m_sProgressMsg += via_server ? tr("via server") : tr("direct");
    m_sProgressMsg += "...";
    QString title = m_sBaseTitle + " [" + m_sProgressMsg + "]";
    setCaption(title);
    setCursor(waitCursor);
    btnSend->setEnabled(false);
    btnCancel->setText(tr("&Cancel"));
    connect (sigman, SIGNAL(signal_doneUserFcn(ICQEvent *)), this, SLOT(sendDone_common(ICQEvent *)));
  }
}


//-----UserSendCommon::setText-----------------------------------------------
void UserSendCommon::setText(const QString& txt)
{
  if(!mleSend) return;
  mleSend->setText(txt);
  mleSend->GotoEnd();
  mleSend->setEdited(false);
}


//-----UserSendCommon::sendDone_common---------------------------------------
void UserSendCommon::sendDone_common(ICQEvent *e)
{
  if ( !e->Equals(icqEventTag) )
    return;

  QString title, result;
  if (e == NULL)
  {
    result = tr("error");
  }
  else
  {
    switch (e->Result())
    {
    case EVENT_ACKED:
    case EVENT_SUCCESS:
      result = tr("done");
      QTimer::singleShot(5000, this, SLOT(slot_resettitle()));
      break;
    case EVENT_FAILED:
      result = tr("failed");
      break;
    case EVENT_TIMEDOUT:
      result = tr("timed out");
      break;
    case EVENT_ERROR:
      result = tr("error");
      break;
    default:
      break;
    }
  }
  title = m_sBaseTitle + " [" + m_sProgressMsg + result + "]";
  setCaption(title);

  setCursor(arrowCursor);
  btnSend->setEnabled(true);
  btnCancel->setText(tr("&Close"));
  icqEventTag = 0;
  disconnect (sigman, SIGNAL(signal_doneUserFcn(ICQEvent *)), this, SLOT(sendDone_common(ICQEvent *)));

  if (e == NULL || e->Result() != EVENT_ACKED)
  {
    if (e->Command() == ICQ_CMDxTCP_START &&
        (e->SubCommand() != ICQ_CMDxSUB_CHAT &&
         e->SubCommand() != ICQ_CMDxSUB_FILE) &&
	(mainwin->m_bAutoSendThroughServer ||
         QueryUser(this, tr("Direct send failed,\nsend through server?"), tr("Yes"), tr("No"))) )
    {
      RetrySend(e, false, ICQ_TCPxMSG_NORMAL);
    }
    return;
  }

  ICQUser *u = NULL;
  CUserEvent *ue = e->UserEvent();
  QString msg;
  if (e->SubResult() == ICQ_TCPxACK_RETURN)
  {
    u = gUserManager.FetchUser(m_nUin, LOCK_W);
    msg = tr("%1 is in %2 mode:\n%3\nSend...")
             .arg(codec->toUnicode(u->GetAlias())).arg(u->StatusStr())
             .arg(codec->toUnicode(u->AutoResponse()));

    u->SetShowAwayMsg( false );
    gUserManager.DropUser(u);
    switch (QueryUser(this, msg, tr("Urgent"), tr(" to Contact List"), tr("Cancel")))
    {
      case 0:
        RetrySend(e, true, ICQ_TCPxMSG_URGENT);
        break;
      case 1:
        RetrySend(e, true, ICQ_TCPxMSG_LIST);
        break;
      case 2:
        break;
    }
    return;
  }
  else if (e->SubResult() == ICQ_TCPxACK_REFUSE)
  {
    u = gUserManager.FetchUser(m_nUin, LOCK_R);
    msg = tr("%1 refused %2, send through server.")
          .arg(codec->toUnicode(u->GetAlias())).arg(EventDescription(ue));
    InformUser(this, msg);
    gUserManager.DropUser(u);
    return;
  }
  else
  {
    emit autoCloseNotify();
    if (sendDone(e))
    {
      emit mainwin->signal_sentevent(e);

      if (mainwin->m_bMsgChatView) {
        mleHistory->GotoEnd();
        resetSettings();
      } else
        close();
    }
  }
}

//-----UserSendCommon::RetrySend---------------------------------------------
void UserSendCommon::RetrySend(ICQEvent *e, bool bOnline, unsigned short nLevel)
{
  chkSendServer->setChecked(!bOnline);
  chkUrgent->setChecked(nLevel == ICQ_TCPxMSG_URGENT);

  switch(e->SubCommand())
  {
    case ICQ_CMDxSUB_MSG:
    {
      CEventMsg *ue = (CEventMsg *)e->UserEvent();

      icqEventTag = server->icqSendMessage(m_nUin, ue->Message(), bOnline,
         nLevel, false, &icqColor);
      break;
    }
    case ICQ_CMDxSUB_URL:
    {
      CEventUrl *ue = (CEventUrl *)e->UserEvent();
      icqEventTag = server->icqSendUrl(m_nUin, ue->Url(), ue->Description(),
         bOnline, nLevel, false, &icqColor);
      break;
    }
    case ICQ_CMDxSUB_CONTACTxLIST:
    {
      CEventContactList* ue = (CEventContactList *) e->UserEvent();
      const ContactList& clist = ue->Contacts();
      UinList uins;

      for(ContactList::const_iterator i = clist.begin(); i != clist.end(); i++)
        uins.push_back((*i)->Uin());

      if(uins.size() == 0)
        break;

      icqEventTag = server->icqSendContactList(m_nUin, uins,
       bOnline, nLevel, false, &icqColor);
      break;
    }
    case ICQ_CMDxSUB_CHAT:
    {
      CEventChat *ue = (CEventChat *)e->UserEvent();
      icqEventTag = server->icqChatRequest(m_nUin, ue->Reason(), nLevel);
      break;
    }
    case ICQ_CMDxSUB_FILE:
    {
      CEventFile *ue = (CEventFile *)e->UserEvent();
      icqEventTag = server->icqFileTransfer(m_nUin, ue->Filename(),
         ue->FileDescription(), nLevel);
      break;
    }
    default:
    {
      gLog.Warn("%sInternal error: UserSendCommon::RetrySend()\n"
         "%sUnknown sub-command %d.\n", L_WARNxSTR, L_BLANKxSTR, e->SubCommand());
      break;
    }
  }

  UserSendCommon::sendButton();
}


//-----UserSendCommon::cancelSend--------------------------------------------
void UserSendCommon::cancelSend()
{
  if (!icqEventTag)
  {
    close();
    return;
  }

  setCaption(m_sBaseTitle);
  server->CancelEvent(icqEventTag);
  icqEventTag = 0;
  btnSend->setEnabled(true);
  btnCancel->setText(tr("&Close"));
  setCursor(arrowCursor);
}


//-----UserSendCommon::UserUpdated-------------------------------------------
void UserSendCommon::UserUpdated(CICQSignal *sig, ICQUser *u)
{
  switch (sig->SubSignal())
  {
    case USER_STATUS:
    {
      if (u->Ip() == 0)
      {
        chkSendServer->setChecked(true);
        chkSendServer->setEnabled(false);
      }
      else
      {
        chkSendServer->setEnabled(true);
      }
      if (u->StatusOffline())
        chkSendServer->setChecked(true);
      break;
    }
    case USER_EVENTS:
    {
      CUserEvent *e = 0;
      if (sig->Argument() > 0 && mleHistory)
      {
        e = u->EventPeekId(sig->Argument());
        if (e != NULL)
        {
          mleHistory->addMsg(e);
        }
      }
    }
  }
}


//-----UserSendCommon::checkSecure-------------------------------------------
bool UserSendCommon::checkSecure()
{
  ICQUser* u = gUserManager.FetchUser(m_nUin, LOCK_R);
  bool send_ok = true;
  if (chkSendServer->isChecked() && ( u->Secure() || u->AutoSecure()) )
  {
    if (!QueryUser(this, tr("Warning: Message can't be sent securely\n"
                            "through the server!"),
                   tr("Send anyway"), tr("Cancel")))
    {
      send_ok = false;
    }
  }
  gUserManager.DropUser(u);
  return send_ok;
}

//=====UserSendMsgEvent======================================================
UserSendMsgEvent::UserSendMsgEvent(CICQDaemon *s, CSignalManager *theSigMan,
  CMainWindow *m, unsigned long nUin, QWidget *parent)
  : UserSendCommon(s, theSigMan, m, nUin, parent, "UserSendMsgEvent")
{
  QBoxLayout* lay = new QVBoxLayout(mainWidget);
  lay->addWidget(splView);
  if (!m->m_bMsgChatView) mleSend->setMinimumHeight(150);
  mleSend->setFocus ();

  m_sBaseTitle += tr(" - Message");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(0);
}


UserSendMsgEvent::~UserSendMsgEvent()
{
}


//-----UserSendMsgEvent::sendButton------------------------------------------
void UserSendMsgEvent::sendButton()
{
  // do nothing if a command is already being processed
  if (icqEventTag != 0) return;

  if(!mleSend->edited() &&
     !QueryUser(this, tr("You didn't edit the message.\n"
                         "Do you really want to send it?"), tr("&Yes"), tr("&No")))
    return;

  // don't let the user send empty messages
  if (mleSend->text().stripWhiteSpace().isEmpty()) return;

  if (!UserSendCommon::checkSecure()) return;

  // the part is generated according to the raw data's length (number of bytes,
  // not number of glyphs), so we use a QCString
  QCString msgTextCurrent = generatePart(mleSend->text());

  if (chkMass->isChecked())
  {
    CMMSendDlg *m = new CMMSendDlg(server, sigman, lstMultipleRecipients, this);
    int r = m->go_message(codec->toUnicode(msgTextCurrent));
    delete m;
    if (r != QDialog::Accepted) return;
  }

  icqEventTag = server->icqSendMessage(m_nUin, msgTextCurrent.data(),
     chkSendServer->isChecked() ? false : true,
     chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL,
     chkMass->isChecked(), &icqColor);

  UserSendCommon::sendButton();
}


//-----UserSendMsgEvent::sendDone--------------------------------------------
bool UserSendMsgEvent::sendDone(ICQEvent *e)
{
  if (e->Command() != ICQ_CMDxTCP_START) {
    CEventMsg *ue = (CEventMsg *)e->UserEvent();
    mleSend->setText(mleSend->text().mid( codec->toUnicode(ue->Message()).length()+1 ));
    return mleSend->text().isEmpty();
  }

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u->Away() && u->ShowAwayMsg())
  {
    gUserManager.DropUser(u);
    (void) new ShowAwayMsgDlg(NULL, NULL, m_nUin);
  }
  else
    gUserManager.DropUser(u);

  return true;

}

void UserSendMsgEvent::resetSettings()
{
  mleSend->clear();
  mleSend->setFocus();
  massMessageToggled( false );
}

//=====UserSendUrlEvent======================================================
UserSendUrlEvent::UserSendUrlEvent(CICQDaemon *s, CSignalManager *theSigMan,
                                   CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserSendCommon(s, theSigMan, m, _nUin, parent, "UserSendUrlEvent")
{
  QBoxLayout* lay = new QVBoxLayout(mainWidget, 4);
  lay->addWidget(splView);
  mleSend->setFocus ();

  QBoxLayout* h_lay = new QHBoxLayout(lay);
  lblItem = new QLabel(tr("URL : "), mainWidget);
  h_lay->addWidget(lblItem);
  edtItem = new CInfoField(mainWidget, false);
  h_lay->addWidget(edtItem);

  m_sBaseTitle += tr(" - URL");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(1);
}


UserSendUrlEvent::~UserSendUrlEvent()
{
}

void UserSendUrlEvent::setUrl(const QString& url, const QString& description)
{
  edtItem->setText(url);
  setText(description);
}

void UserSendUrlEvent::resetSettings()
{
  mleSend->clear();
  edtItem->clear();
  mleSend->setFocus();
  massMessageToggled( false );
}


//-----UserSendUrlEvent::sendButton------------------------------------------
void UserSendUrlEvent::sendButton()
{
  if (edtItem->text().stripWhiteSpace().isEmpty())
    return;

  if (!UserSendCommon::checkSecure()) return;

  if (chkMass->isChecked())
  {
    CMMSendDlg *m = new CMMSendDlg(server, sigman, lstMultipleRecipients, this);
    int r = m->go_url(edtItem->text(), mleSend->text());
    delete m;
    if (r != QDialog::Accepted) return;
  }

  icqEventTag = server->icqSendUrl(m_nUin, edtItem->text().latin1(), codec->fromUnicode(mleSend->text()),
     chkSendServer->isChecked() ? false : true,
     chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL,
     chkMass->isChecked(), &icqColor);

  UserSendCommon::sendButton();
}


//-----UserSendUrlEvent::sendDone--------------------------------------------
bool UserSendUrlEvent::sendDone(ICQEvent *e)
{
  if (e->Command() != ICQ_CMDxTCP_START) return true;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u->Away() && u->ShowAwayMsg())
  {
    gUserManager.DropUser(u);
    (void) new ShowAwayMsgDlg(NULL, NULL, m_nUin);
  }
  else
    gUserManager.DropUser(u);

  return true;
}


//=====UserSendFileEvent=====================================================
UserSendFileEvent::UserSendFileEvent(CICQDaemon *s, CSignalManager *theSigMan,
                                     CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserSendCommon(s, theSigMan, m, _nUin, parent, "UserSendFileEvent")
{
  chkSendServer->setChecked(false);
  chkSendServer->setEnabled(false);
  chkMass->setChecked(false);
  chkMass->setEnabled(false);
  btnForeColor->setEnabled(false);
  btnBackColor->setEnabled(false);

  QBoxLayout* lay = new QVBoxLayout(mainWidget, 4);
  lay->addWidget(splView);

  QBoxLayout* h_lay = new QHBoxLayout(lay);
  lblItem = new QLabel(tr("File(s): "), mainWidget);
  h_lay->addWidget(lblItem);

  edtItem = new CInfoField(mainWidget, false);
  h_lay->addWidget(edtItem);

  btnBrowse = new QPushButton(tr("Browse"), mainWidget);
  connect(btnBrowse, SIGNAL(clicked()), this, SLOT(browseFile()));
  h_lay->addWidget(btnBrowse);

  m_sBaseTitle += tr(" - File Transfer");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(3);
}


void UserSendFileEvent::browseFile()
{
#ifdef USE_KDE
    QStringList fl = KFileDialog::getOpenFileNames(NULL, NULL, this);
#else
    QStringList fl = QFileDialog::getOpenFileNames(QString::null, QString::null, this,
                                                   "SendFileBrowser", tr("Select files to send"));
#endif
    if (fl.isEmpty()) return;
    QStringList::ConstIterator it;
    QString f;
    for( it = fl.begin(); it != fl.end(); it++ )
    {
      if (it != fl.begin())
        f += ", ";
      f += (*it);
    }
    edtItem->setText(f);
}


UserSendFileEvent::~UserSendFileEvent()
{
}


void UserSendFileEvent::sendButton()
{
  if (edtItem->text().stripWhiteSpace().isEmpty())
  {
    WarnUser(this, tr("You must specify a file to transfer!"));
    return;
  }

  icqEventTag = server->icqFileTransfer(m_nUin, codec->fromUnicode(edtItem->text()),
     codec->fromUnicode(mleSend->text()),
     chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL);

  UserSendCommon::sendButton();
}

void UserSendFileEvent::setFile(const QString& file, const QString& description)
{
  edtItem->setText(file);
  setText(description);
}

void UserSendFileEvent::resetSettings()
{
  mleSend->clear();
  edtItem->clear();
  mleSend->setFocus();
  massMessageToggled( false );
}

//-----UserSendFileEvent::sendDone-------------------------------------------
bool UserSendFileEvent::sendDone(ICQEvent *e)
{
  if (!e->ExtendedAck()->Accepted())
  {
    ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
    QString result = tr("File transfer with %2 refused:\n%3").arg(u->GetAlias()).arg(e->ExtendedAck()->Response());
    gUserManager.DropUser(u);
    InformUser(this, result);
  }
  else
  {
    CEventFile *f = (CEventFile *)e->UserEvent();
    CFileDlg *fileDlg = new CFileDlg(m_nUin, server);
    fileDlg->SendFiles(f->Filename(), e->ExtendedAck()->Port());
  }

  return true;
}


//=====UserSendChatEvent=====================================================
UserSendChatEvent::UserSendChatEvent(CICQDaemon *s, CSignalManager *theSigMan,
                                     CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserSendCommon(s, theSigMan, m, _nUin, parent, "UserSendChatEvent")
{
  m_nMPChatPort = 0;
  chkSendServer->setChecked(false);
  chkSendServer->setEnabled(false);
  chkMass->setChecked(false);
  chkMass->setEnabled(false);
  btnForeColor->setEnabled(false);
  btnBackColor->setEnabled(false);

  QBoxLayout *lay = new QVBoxLayout(mainWidget, 9);
  lay->addWidget(splView);

  if (!m->m_bMsgChatView) mleSend->setMinimumHeight(150);

  QBoxLayout* h_lay = new QHBoxLayout(lay);
  lblItem = new QLabel(tr("Multiparty: "), mainWidget);
  h_lay->addWidget(lblItem);

  edtItem = new CInfoField(mainWidget, false);
  h_lay->addWidget(edtItem);

  btnBrowse = new QPushButton(tr("Invite"), mainWidget);
  connect(btnBrowse, SIGNAL(clicked()), this, SLOT(InviteUser()));
  h_lay->addWidget(btnBrowse);

  m_sBaseTitle += tr(" - Chat Request");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(2);
}


UserSendChatEvent::~UserSendChatEvent()
{
}

void UserSendChatEvent::InviteUser()
{
  if (m_nMPChatPort == 0)
  {
    if (ChatDlg::chatDlgs.size() > 0)
    {
      ChatDlg *chatDlg = NULL;
      CJoinChatDlg *j = new CJoinChatDlg(true, this);
      if (j->exec() && (chatDlg = j->JoinedChat()) != NULL)
      {
        edtItem->setText(j->ChatClients());
        m_nMPChatPort = chatDlg->LocalPort();
        m_szMPChatClients = chatDlg->ChatClients();
      }
      delete j;
      btnBrowse->setText(tr("Clear"));
    }
  }
  else
  {
    m_nMPChatPort = 0;
    m_szMPChatClients = "";
    edtItem->setText("");
    btnBrowse->setText(tr("Invite"));
  }
}

void UserSendChatEvent::resetSettings()
{
  mleSend->clear();
  edtItem->clear();
  mleSend->setFocus();
  massMessageToggled( false );
}

//-----UserSendChatEvent::sendButton-----------------------------------------
void UserSendChatEvent::sendButton()
{
  if (m_nMPChatPort == 0)
    icqEventTag = server->icqChatRequest(m_nUin,
                                         codec->fromUnicode(mleSend->text()),
                                         chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL);
  else
    icqEventTag = server->icqMultiPartyChatRequest(m_nUin,
                                                   codec->fromUnicode(mleSend->text()), codec->fromUnicode(m_szMPChatClients),
                                                   m_nMPChatPort,
                                                   chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL);
  UserSendCommon::sendButton();
}


//-----UserSendChatEvent::sendDone-------------------------------------------
bool UserSendChatEvent::sendDone(ICQEvent *e)
{
  if (!e->ExtendedAck()->Accepted())
  {
    ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
    QString result = tr("Chat with %2 refused:\n%3").arg(u->GetAlias())
                     .arg(e->ExtendedAck()->Response());
    gUserManager.DropUser(u);
    InformUser(this, result);

  }
  else
  {
    CEventChat *c = (CEventChat *)e->UserEvent();
    if (c->Port() == 0)  // If we requested a join, no need to do anything
    {
      ChatDlg *chatDlg = new ChatDlg(m_nUin, server, mainwin);
      chatDlg->StartAsClient(e->ExtendedAck()->Port());
    }
  }

  return true;
}


//=====UserSendContactEvent==================================================
UserSendContactEvent::UserSendContactEvent(CICQDaemon *s, CSignalManager *theSigMan,
                                           CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserSendCommon(s, theSigMan, m, _nUin, parent, "UserSendContactEvent")
{
  delete mleSend; mleSend = NULL;

  QBoxLayout* lay = new QVBoxLayout(mainWidget);
  lay->addWidget(splView);
  QLabel* lblContact =  new QLabel(tr("Drag Users Here - Right Click for Options"), mainWidget);
  lay->addWidget(lblContact);

  lstContacts = new CMMUserView(mainwin->colInfo, mainwin->m_bShowHeader,
                                m_nUin, mainwin, mainWidget);
  lay->addWidget(lstContacts);

  m_sBaseTitle += tr(" - Contact List");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(4);
}


UserSendContactEvent::~UserSendContactEvent()
{
}


void UserSendContactEvent::sendButton()
{
  CMMUserViewItem *i = static_cast<CMMUserViewItem*>(lstContacts->firstChild());
  UinList uins;

  while (i)
  {
    uins.push_back(i->Uin());
    i = static_cast<CMMUserViewItem *>(i->nextSibling());
  }

  if (uins.size() == 0)
    return;

  if (!UserSendCommon::checkSecure()) return;

  if (chkMass->isChecked())
  {
    CMMSendDlg *m = new CMMSendDlg(server, sigman, lstMultipleRecipients, this);
    int r = m->go_contact(uins);
    delete m;
    if (r != QDialog::Accepted) return;
  }

  icqEventTag = server->icqSendContactList(m_nUin, uins,
    chkSendServer->isChecked() ? false : true,
    chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL,
    chkMass->isChecked(), &icqColor);

  UserSendCommon::sendButton();
}

void UserSendContactEvent::resetSettings()
{
  lstContacts->clear();
  massMessageToggled( false );
}

//-----UserSendMsgEvent::sendDone--------------------------------------------
bool UserSendContactEvent::sendDone(ICQEvent *e)
{
  if (e->Command() != ICQ_CMDxTCP_START) return true;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u->Away() && u->ShowAwayMsg())
  {
    gUserManager.DropUser(u);
    (void) new ShowAwayMsgDlg(NULL, NULL, m_nUin);
  }
  else
    gUserManager.DropUser(u);

  return true;
}


//-----UserSendContactEvent::setContact--------------------------------------
void UserSendContactEvent::setContact(unsigned long Uin, const QString&)
{
  ICQUser* u = gUserManager.FetchUser(Uin, LOCK_R);

  if(u != NULL)
  {
    (void) new CMMUserViewItem(u, lstContacts);

    gUserManager.DropUser(u);
  }
}


// -----------------------------------------------------------------------------

#include "usereventdlg.moc"
