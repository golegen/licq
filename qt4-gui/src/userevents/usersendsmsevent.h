// -*- c-basic-offset: 2 -*-
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

#ifndef USERSENDSMSEVENT_H
#define USERSENDSMSEVENT_H

#include "usersendcommon.h"

class QLabel;

namespace LicqQtGui
{

class UserSendSmsEvent : public UserSendCommon
{
  Q_OBJECT

public:
  UserSendSmsEvent(QString id, unsigned long ppid, QWidget* parent = 0);
  virtual ~UserSendSmsEvent();

private:
  QLabel* lblNumber;
  QLabel* lblCount;
  InfoField* nfoNumber;
  InfoField* nfoCount;

  virtual bool sendDone(ICQEvent* e);
  virtual void resetSettings();

private slots:
  virtual void send();
  void count();
};

} //namespace LicqQtGui

#endif
