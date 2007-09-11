/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2000-2006 Licq developers
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

#ifndef MMSENDDLG_H
#define MMSENDDLG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <qstring.h>

#include "licqdialog.h"
#include "licq_user.h"

class QPushButton;
class QVGroupBox;
class QProgressBar;

class CSignalManager;
class CMMUserView;
class CMMUserViewItem;
class CICQDaemon;
class ICQEvent;

class CMMSendDlg : public LicqDialog
{
  Q_OBJECT
public:
  CMMSendDlg(CICQDaemon *, CSignalManager *, CMMUserView *,
     QWidget *p = 0);
  ~CMMSendDlg();

  int go_message(QString);
  int go_url(QString, QString);
  int go_contact(UserStringList &_users);

protected:
  QString s1, s2;
  UserStringList *users;

  unsigned long m_nEventType;
  QVGroupBox *grpSending;
  QPushButton *btnCancel;
  QProgressBar *barSend;
  char *m_szId;
  unsigned long m_nPPID;
  CMMUserView *mmv;
  CMMUserViewItem *mmvi;
  CICQDaemon *server;
  unsigned long icqEventTag;

  void SendNext();
protected slots:
  void slot_done(ICQEvent *);
  void slot_cancel();
};

#endif
