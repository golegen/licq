/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2010 Licq Developers <licq-dev@googlegroups.com>
 *
 * Please refer to the COPYRIGHT file distributed with this source
 * distribution for the names of the individual contributors.
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

#include "client.h"
#include "handler.h"
#include "jabber.h"

#include <licq/contactlist/owner.h>
#include <licq/contactlist/user.h>
#include <licq/contactlist/usermanager.h>
#include <licq/daemon.h>
#include <licq/log.h>
#include <licq_events.h>
#include <licq/plugin.h>
#include <licq/protocolsignal.h>

#include <sys/select.h>

using Licq::gLog;
using std::string;

Jabber::Jabber() :
  myHandler(NULL),
  myDoRun(false),
  myClient(NULL)
{
  myHandler = new Handler();
}

Jabber::~Jabber()
{
  delete myClient;
  delete myHandler;
}

int Jabber::run(int pipe)
{
  fd_set readFds;

  myDoRun = (pipe != -1);
  while (myDoRun)
  {
    FD_ZERO(&readFds);
    FD_SET(pipe, &readFds);
    int nfds = pipe + 1;

    int sock = -1;
    if (myClient != NULL)
    {
      sock = myClient->getSocket();
      if (sock != -1)
      {
        FD_SET(sock, &readFds);
        if (sock > pipe)
          nfds = sock + 1;
      }
    }

    if (::select(nfds, &readFds, NULL, NULL, NULL) > 0)
    {
      if (sock != -1 && FD_ISSET(sock, &readFds))
        myClient->recv();
      if (FD_ISSET(pipe, &readFds))
        processPipe(pipe);
    }
  }

  return 0;
}

void Jabber::processPipe(int pipe)
{
  char ch;
  ::read(pipe, &ch, sizeof(ch));

  switch (ch)
  {
    case Licq::ProtocolPlugin::PipeSignal:
    {
      Licq::ProtocolSignal* signal = Licq::gDaemon.PopProtoSignal();
      processSignal(signal);
      delete signal;
      break;
    }
    case Licq::ProtocolPlugin::PipeShutdown:
      myDoRun = false;
      break;
    default:
      gLog.error("Unkown command %c", ch);
      break;
  }
}

void Jabber::processSignal(Licq::ProtocolSignal* signal)
{
  assert(signal != NULL);

  gLog.info("Got signal %u", signal->signal());
  switch (signal->signal())
  {
    case Licq::ProtocolSignal::SignalLogon:
      doLogon(static_cast<Licq::ProtoLogonSignal*>(signal));
      break;
    case Licq::ProtocolSignal::SignalChangeStatus:
      doChangeStatus(static_cast<Licq::ProtoChangeStatusSignal*>(signal));
      break;
    case Licq::ProtocolSignal::SignalLogoff:
      doLogoff();
      break;
    case Licq::ProtocolSignal::SignalSendMessage:
      doSendMessage(static_cast<Licq::ProtoSendMessageSignal*>(signal));
      break;
    case Licq::ProtocolSignal::SignalRequestInfo:
      doGetInfo(static_cast<Licq::ProtoRequestInfo*>(signal));
      break;
    case Licq::ProtocolSignal::SignalAddUser:
      doAddUser(static_cast<Licq::ProtoAddUserSignal*>(signal));
      break;
    case Licq::ProtocolSignal::SignalChangeUserGroups:
      doChangeUserGroups(static_cast<Licq::ProtoChangeUserGroupsSignal*>(signal));
      break;
    case Licq::ProtocolSignal::SignalRemoveUser:
      doRemoveUser(static_cast<Licq::ProtoRemoveUserSignal*>(signal));
      break;
    case Licq::ProtocolSignal::SignalRenameUser:
      doRenameUser(static_cast<Licq::ProtoRenameUserSignal*>(signal));
      break;
    case Licq::ProtocolSignal::SignalNotifyTyping:
    case Licq::ProtocolSignal::SignalGrantAuth:
    case Licq::ProtocolSignal::SignalRefuseAuth:
    case Licq::ProtocolSignal::SignalUpdateInfo:
    case Licq::ProtocolSignal::SignalRequestPicture:
    case Licq::ProtocolSignal::SignalBlockUser:
    case Licq::ProtocolSignal::SignalUnblockUser:
    case Licq::ProtocolSignal::SignalAcceptUser:
    case Licq::ProtocolSignal::SignalUnacceptUser:
    case Licq::ProtocolSignal::SignalIgnoreUser:
    case Licq::ProtocolSignal::SignalUnignoreUser:
    case Licq::ProtocolSignal::SignalSendFile:
    case Licq::ProtocolSignal::SignalSendChat:
    case Licq::ProtocolSignal::SignalCancelEvent:
    case Licq::ProtocolSignal::SignalSendReply:
    case Licq::ProtocolSignal::SignalOpenedWindow:
    case Licq::ProtocolSignal::SignalClosedWindow:
    case Licq::ProtocolSignal::SignalOpenSecure:
    case Licq::ProtocolSignal::SignalCloseSecure:
    default:
      gLog.info("Unkown signal %u", signal->signal());
      break;
  }
}

void Jabber::doLogon(Licq::ProtoLogonSignal* signal)
{
  unsigned status = signal->status();
  if (status == Licq::User::OfflineStatus)
    return;

  string username;
  string password;
  {
    Licq::OwnerReadGuard owner(JABBER_PPID);
    if (!owner.isLocked())
    {
      gLog.error("No owner set");
      return;
    }

    username = owner->accountId();
    password = owner->password();
  }

  myHandler->setStatus(status);

  if (myClient == NULL)
    myClient = new Client(*myHandler, username, password);
  else
    myClient->setPassword(password);

  if (!myClient->isConnected())
  {
    if (!myClient->connect(status))
    {
      delete myClient;
      myClient = NULL;
      return;
    }
  }
}

void Jabber::doChangeStatus(Licq::ProtoChangeStatusSignal* signal)
{
  assert(myClient != NULL);
  myClient->changeStatus(signal->status());
}

void Jabber::doLogoff()
{
  if (myClient == NULL)
    return;

  delete myClient;
  myClient = NULL;
}

void Jabber::doSendMessage(Licq::ProtoSendMessageSignal* signal)
{
  assert(myClient != NULL);
  myClient->sendMessage(signal->userId().accountId(), signal->message());

  CEventMsg* message = new CEventMsg(signal->message().c_str(), 0, CUserEvent::TimeNow, 0);
  message->setIsReceiver(false);

  LicqEvent* event = new LicqEvent(signal->eventId(), 0, NULL, CONNECT_SERVER,
                                   signal->userId(), message);
  event->thread_plugin = signal->callerThread();
  event->m_eResult = EVENT_ACKED;

  if (event->m_pUserEvent)
  {
    Licq::UserWriteGuard user(signal->userId());
    event->m_pUserEvent->AddToHistory(*user, false);
  }

  Licq::gDaemon.PushPluginEvent(event);
}

void Jabber::doGetInfo(Licq::ProtoRequestInfo* signal)
{
  assert(myClient != NULL);
  myClient->getVCard(signal->userId().accountId());
}

void Jabber::doAddUser(Licq::ProtoAddUserSignal* signal)
{
  assert(myClient != NULL);
  myClient->addUser(signal->userId().accountId());
}

void Jabber::doChangeUserGroups(Licq::ProtoChangeUserGroupsSignal* signal)
{
  assert(myClient != NULL);
  const Licq::UserId userId = signal->userId();

  // Get names of all group user is member of
  gloox::StringList groupNames;
  {
    Licq::UserReadGuard u(userId);
    if (!u.isLocked())
      return;
    const Licq::UserGroupList groups = u->GetGroups();
    for (Licq::UserGroupList::const_iterator i = groups.begin(); i != groups.end(); ++i)
    {
      string groupName = Licq::gUserManager. GetGroupNameFromGroup(*i);
      if (!groupName.empty())
        groupNames.push_back(groupName);
    }
  }
  myClient->changeUserGroups(userId.accountId(), groupNames);
}

void Jabber::doRemoveUser(Licq::ProtoRemoveUserSignal* signal)
{
  assert(myClient != NULL);
  myClient->removeUser(signal->userId().accountId());
}

void Jabber::doRenameUser(Licq::ProtoRenameUserSignal* signal)
{
  assert(myClient != NULL);
  string newName;
  {
    Licq::UserReadGuard u(signal->userId());
    if (!u.isLocked())
      return;
    newName = u->getAlias();
  }

  myClient->renameUser(signal->userId().accountId(), newName);
}
