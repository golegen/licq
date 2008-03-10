/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2007 Licq developers
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

#ifndef USERMENU_H
#define USERMENU_H

#include "config.h"

#include <QMap>
#include <QMenu>

#include "gui-defines.h"

class QActionGroup;

namespace LicqQtGui
{
/**
 * User menu for contact list and user dialogs.
 *
 * The same menu is used for all contacts. Before displaying, the list is updated
 * with options and settings for the selected contact.
 */
class UserMenu : public QMenu
{
  Q_OBJECT

public:
  /**
   * Constructor
   *
   * @param parent Parent widget
   */
  UserMenu(QWidget* parent = 0);

  /**
   * Update the list of groups from the daemon. This must be called when the
   * list of groups is changed.
   */
  void updateGroups();

  /**
   * Change which contact the menu will be displayed for.
   *
   * @param id Contact id
   * @param ppid Contact protocol id
   */
  void setUser(QString id, unsigned long ppid);

  /**
   * Convenience function t set user and popup the menu on a given location.
   *
   * @param pos Posititon to show menu in global coordinates
   * @param id Contact id
   * @param ppid Contact protocol id
   */
  void popup(QPoint pos, QString id, unsigned ppid);

private slots:
  /**
   * Update icons in menu.
   */
  void updateIcons();

  void aboutToShowMenu();

  void viewEvent();
  void checkInvisible();
  void checkAutoResponse();
  void customAutoResponse();
  void toggleFloaty();
  void removeContact();
  void selectKey();
  void viewHistory();
  void viewInfoGeneral();

  void send(QAction* action);
  void toggleMiscMode(QAction* action);
  void utility(QAction* action);

  void toggleUserGroup(QAction* action);
  void toggleSystemGroup(QAction* action);
  void setServerGroup(QAction* action);

private:
  // Current contact
  QString myId;
  unsigned long myPpid;

  // Internal numbering of send sub menu entries
  enum SendModes
  {
    SendMessage = mnuUserSendMsg,
    SendUrl = mnuUserSendUrl,
    SendChat = mnuUserSendChat,
    SendFile = mnuUserSendFile,
    SendContact = mnuUserSendContact,
    SendSms = mnuUserSendSms,
    SendAuthorize,
    SendReqAuthorize,
    RequestUpdateInfoPlugin,
    RequestUpdateStatusPlugin,
    RequestPhoneFollowMeStatus,
    RequestIcqphoneStatus,
    RequestFileServerStatus,
    SendKey
  };

  // Internal numbering of misc modes sub menu entries
  enum MiscModes
  {
    ModeAcceptInAway,
    ModeAcceptInNa,
    ModeAcceptInOccupied,
    ModeAcceptInDnd,
    ModeAutoFileAccept,
    ModeAutoChatAccept,
    ModeAutoSecure,
#ifdef HAVE_LIBGPGME
    ModeUseGpg,
#endif
    ModeUseRealIp,
    ModeStatusOnline,
    ModeStatusAway,
    ModeStatusNa,
    ModeStatusOccupied,
    ModeStatusDnd
  };

  // Actions not in any sub menu
  QAction* myViewEventAction;
  QAction* myCheckInvisibleAction;
  QAction* myCheckArAction;
  QAction* myCustomArAction;
  QAction* myToggleFloatyAction;
  QAction* myRemoveUserAction;
#ifdef HAVE_LIBGPGME
  QAction* mySetKeyAction;
#endif
  QAction* myViewHistoryAction;
  QAction* myViewGeneralAction;

  // Sub menus
  QMenu* mySendMenu;
  QMenu* myMiscModesMenu;
  QMenu* myUtilitiesMenu;
  QMenu* myGroupsMenu;
  QMenu* myServerGroupsMenu;
  QAction* myGroupSeparator;

  // Containers for the group sub menu entries
  QActionGroup* myUserGroupActions;
  QActionGroup* mySystemGroupActions;
  QActionGroup* myServerGroupActions;

  // Maps for holding sub menu entries
  QMap<SendModes, QAction*> mySendActions;
  QMap<MiscModes, QAction*> myMiscModesActions;
};

} // namespace LicqQtGui

#endif