// -*- c-basic-offset: 2 -*-
/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2000-2011 Licq developers
 *
 * Licq is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Licq is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Licq; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "usersendurlevent.h"

#include <QAction>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QPushButton>
#include <QSplitter>
#include <QTextCodec>
#include <QTimer>
#include <QVBoxLayout>

#include <licq/contactlist/user.h>
#include <licq/event.h>
#include <licq/icqdefines.h>
#include <licq/protocolmanager.h>
#include <licq/protocolsignal.h>

#include "config/chat.h"

#include "core/gui-defines.h"
#include "core/messagebox.h"

#include "dialogs/mmsenddlg.h"
#include "dialogs/showawaymsgdlg.h"

#include "widgets/infofield.h"
#include "widgets/mledit.h"

using Licq::gProtocolManager;
using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::UserSendUrlEvent */

UserSendUrlEvent::UserSendUrlEvent(const Licq::UserId& userId, QWidget* parent)
  : UserSendCommon(UrlEvent, userId, parent, "UserSendUrlEvent")
{
  myMainWidget->addWidget(myViewSplitter);
  myMessageEdit->setFocus();

  QHBoxLayout* h_lay = new QHBoxLayout();
  myMainWidget->addLayout(h_lay);
  myUrlLabel = new QLabel(tr("URL : "));
  h_lay->addWidget(myUrlLabel);
  myUrlEdit = new InfoField(false);
  h_lay->addWidget(myUrlEdit);
  myUrlEdit->installEventFilter(this);

  myBaseTitle += tr(" - URL");

  setWindowTitle(myBaseTitle);
  myEventTypeGroup->actions().at(UrlEvent)->setChecked(true);
}

UserSendUrlEvent::~UserSendUrlEvent()
{
  // Empty
}

bool UserSendUrlEvent::eventFilter(QObject* watched, QEvent* e)
{
  if (watched == myUrlEdit)
  {
    if (e->type() == QEvent::KeyPress)
    {
      QKeyEvent* key = dynamic_cast<QKeyEvent*>(e);
      const bool isEnter = (key->key() == Qt::Key_Enter || key->key() == Qt::Key_Return);
      if (isEnter && (Config::Chat::instance()->singleLineChatMode() || key->modifiers() & Qt::ControlModifier))
      {
        mySendButton->animateClick();
        return true; // filter the event out
      }
    }
    return false;
  }
  else
    return UserSendCommon::eventFilter(watched, e);
}

void UserSendUrlEvent::setUrl(const QString& url, const QString& description)
{
  myUrlEdit->setText(url);
  setText(description);
}

bool UserSendUrlEvent::sendDone(const Licq::Event* e)
{
  if (e->Command() != ICQ_CMDxTCP_START)
    return true;

  bool showAwayDlg = false;
  {
    Licq::UserReadGuard u(myUsers.front());
    if (u.isLocked())
      showAwayDlg = u->isAway() && u->ShowAwayMsg();
  }

  if (showAwayDlg && Config::Chat::instance()->popupAutoResponse())
    new ShowAwayMsgDlg(myUsers.front());

  return true;
}

void UserSendUrlEvent::resetSettings()
{
  myMessageEdit->clear();
  myUrlEdit->clear();
  myMessageEdit->setFocus();
  massMessageToggled(false);
}

void UserSendUrlEvent::send()
{
  // Take care of typing notification now
  mySendTypingTimer->stop();
  connect(myMessageEdit, SIGNAL(textChanged()), SLOT(messageTextChanged()));
  gProtocolManager.sendTypingNotification(myUsers.front(), false, myConvoId);

  if (myUrlEdit->text().trimmed().isEmpty())
  {
    InformUser(this, tr("No URL specified"));
    return;
  }

  if (!checkSecure())
    return;

  if (myMassMessageCheck->isChecked())
  {
    MMSendDlg* m = new MMSendDlg(myMassMessageList, this);
    connect(m, SIGNAL(eventSent(const Licq::Event*)), SIGNAL(eventSent(const Licq::Event*)));
    int r = m->go_url(myUrlEdit->text(), myMessageEdit->toPlainText());
    delete m;
    if (r != QDialog::Accepted)
      return;
  }

  unsigned flags = 0;
  if (!mySendServerCheck->isChecked())
    flags |= Licq::ProtocolSignal::SendDirect;
  if (myUrgentCheck->isChecked())
    flags |= Licq::ProtocolSignal::SendUrgent;
  if (myMassMessageCheck->isChecked())
    flags |= Licq::ProtocolSignal::SendToMultiple;

  unsigned long icqEventTag;
  icqEventTag = gProtocolManager.sendUrl(
      myUsers.front(),
      myUrlEdit->text().toLatin1().data(),
      myCodec->fromUnicode(myMessageEdit->toPlainText()).data(),
      flags,
      &myIcqColor);

  myEventTag.push_back(icqEventTag);

  UserSendCommon::send();
}
