// -*- c-basic-offset: 2 -*-
/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 1999-2009 Licq developers
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

#include "mainwin.h"

#include "config.h"

#include <boost/foreach.hpp>
#include <list>
#include <map>
#include <cctype>

#ifdef USE_KDE
#include <KDE/KApplication>
#include <KDE/KGlobal>
#include <KDE/KGlobalSettings>
#include <KDE/KStandardDirs>
#include <KDE/KWindowSystem>
#include <KDE/KIconLoader>
#include <KDE/KUrl>
#else
#include <QApplication>
#endif

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QMenu>
#include <QMoveEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QShortcut>
#include <QStyle>
#include <QStyleFactory>
#include <QTextCodec>
#include <QTextEdit>
#include <QVBoxLayout>

#if defined(Q_WS_X11)
#include <QX11Info>
#endif /* defined(Q_WS_X11) */

#include <licq_log.h>
#include <licq_translate.h>
#include <licq_user.h>
#include <licq/daemon.h>
#include <licq/icq.h>
#include <licq/icqdefines.h>

#include "config/contactlist.h"
#include "config/general.h"
#include "config/iconmanager.h"
#include "config/shortcuts.h"
#include "config/skin.h"

#include "contactlist/contactlist.h"

#include "dockicons/dockicon.h"

#include "dialogs/aboutdlg.h"
#include "dialogs/adduserdlg.h"
#include "dialogs/awaymsgdlg.h"
#include "dialogs/hintsdlg.h"
#include "dialogs/historydlg.h"
#include "dialogs/logwindow.h"
#include "dialogs/ownermanagerdlg.h"
#include "dialogs/showawaymsgdlg.h"
#include "dialogs/statsdlg.h"
#include "dialogs/userselectdlg.h"

#include "helpers/support.h"
#include "helpers/usercodec.h"

#include "widgets/skinnablebutton.h"
#include "widgets/skinnablecombobox.h"
#include "widgets/skinnablelabel.h"

#include "views/userview.h"

#include "messagebox.h"
#include "gui-defines.h"
#include "licqgui.h"
#include "signalmanager.h"
#include "systemmenu.h"
#include "usermenu.h"

#ifdef USE_KDE
// TODO
// #include "licqkimiface.h"
// #include "dcopclient.h"
#endif

using namespace std;
using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::MainWindow */

MainWindow* LicqQtGui::gMainWindow = NULL;

MainWindow::MainWindow(bool bStartHidden, QWidget* parent)
  : QWidget(parent),
    myInMiniMode(false)
{
  Support::setWidgetProps(this, "MainWindow");
  setAttribute(Qt::WA_AlwaysShowToolTips, true);

  assert(gMainWindow == NULL);
  gMainWindow = this;

  Config::General* conf = Config::General::instance();

  connect(conf, SIGNAL(mainwinChanged()), SLOT(updateConfig()));
  connect(Config::ContactList::instance(),
      SIGNAL(currentListChanged()), SLOT(updateCurrentGroup()));

  myCaption = "Licq";
  const ICQOwner* o = gUserManager.FetchOwner(LICQ_PPID, LOCK_R);
  if (o != NULL)
  {
    myCaption += QString(" (%1)").arg(QString::fromUtf8(o->GetAlias()));
    gUserManager.DropOwner(o);
  }
  setWindowTitle(myCaption);

  // Group Combo Box
  myUserGroupsBox = new SkinnableComboBox(this);
  connect(myUserGroupsBox, SIGNAL(activated(int)), SLOT(setCurrentGroup(int)));

  // Widgets controlled completely by the skin
  mySystemButton = NULL;
  myMessageField = NULL;
  myStatusField = NULL;
  myMenuBar = NULL;
  connect(IconManager::instance(),
      SIGNAL(statusIconsChanged()), SLOT(updateStatus()));

  mySystemMenu = new SystemMenu(this);

  QActionGroup* userFuncGroup = new QActionGroup(this);
  userFuncGroup->setExclusive(false);
  connect(userFuncGroup,
      SIGNAL(triggered(QAction*)), SLOT(callUserFunction(QAction*)));
#define ADD_USERFUNCACTION(var, data) \
  var = new QAction(userFuncGroup); \
  var->setData(data);

  ADD_USERFUNCACTION(myViewEventAction, -1)
  ADD_USERFUNCACTION(mySendMessageAction, MessageEvent)
  ADD_USERFUNCACTION(mySendUrlAction, UrlEvent)
  ADD_USERFUNCACTION(mySendFileAction, ChatEvent)
  ADD_USERFUNCACTION(mySendChatRequestAction, FileEvent)
#undef ADD_USERFUNCACTION
  addActions(userFuncGroup->actions());

  myCheckUserArAction = new QAction(this);
  addAction(myCheckUserArAction);
  connect(myCheckUserArAction, SIGNAL(triggered()), SLOT(checkUserAutoResponse()));
  myViewHistoryAction = new QAction(this);
  addAction(myViewHistoryAction);
  connect(myViewHistoryAction, SIGNAL(triggered()), SLOT(showUserHistory()));
  QShortcut* shortcut;
  shortcut = new QShortcut(Qt::CTRL + Qt::Key_Delete, this);
  connect(shortcut, SIGNAL(activated()), SLOT(removeUserFromList()));
  shortcut = new QShortcut(Qt::Key_Delete, this);
  connect(shortcut, SIGNAL(activated()), SLOT(removeUserFromGroup()));

  updateShortcuts();
  connect(Config::Shortcuts::instance(), SIGNAL(shortcutsChanged()), SLOT(updateShortcuts()));

  CreateUserView();

  usprintfHelp = tr(
      "<ul>"
      "<li><tt>%a - </tt>user alias</li>"
      "<li><tt>%e - </tt>email</li>"
      "<li><tt>%f - </tt>first name</li>"
      "<li><tt>%h - </tt>phone number</li>"
      "<li><tt>%i - </tt>user ip</li>"
      "<li><tt>%l - </tt>last name</li>"
      "<li><tt>%L - </tt>local time</li>"
      "<li><tt>%m - </tt># pending messages</li>"
      "<li><tt>%M - </tt># pending messages (if any)</li>"
      "<li><tt>%n - </tt>full name</li>"
      "<li><tt>%o - </tt>last seen online</li>"
      "<li><tt>%O - </tt>online since</li>"
      "<li><tt>%p - </tt>user port</li>"
      "<li><tt>%s - </tt>full status</li>"
      "<li><tt>%S - </tt>abbreviated status</li>"
      "<li><tt>%u - </tt>uin</li>"
      "<li><tt>%w - </tt>webpage</li></ul>");

  connect(gGuiSignalManager, SIGNAL(updatedList(unsigned long, int, const Licq::UserId&)),
      SLOT(slot_updatedList(unsigned long)));
  connect(gGuiSignalManager, SIGNAL(updatedUser(const Licq::UserId&, unsigned long, int, unsigned long)),
      SLOT(slot_updatedUser(const Licq::UserId&, unsigned long, int)));
  connect(gGuiSignalManager, SIGNAL(updatedStatus(unsigned long)),
      SLOT(updateStatus()));
  connect(gGuiSignalManager, SIGNAL(ownerAdded(const Licq::UserId&)),
      SLOT(updateStatus()));
  connect(gGuiSignalManager, SIGNAL(ownerRemoved(const Licq::UserId&)),
      SLOT(updateStatus()));
  connect(gGuiSignalManager, SIGNAL(doneOwnerFcn(const LicqEvent*)),
      SLOT(slot_doneOwnerFcn(const LicqEvent*)));
  connect(gGuiSignalManager, SIGNAL(logon()),
      SLOT(slot_logon()));
  connect(gGuiSignalManager, SIGNAL(protocolPlugin(unsigned long)),
      SLOT(slot_protocolPlugin(unsigned long)));
  connect(gGuiSignalManager, SIGNAL(ownerAdded(const Licq::UserId&)),
      mySystemMenu, SLOT(addOwner(const Licq::UserId&)));
  connect(gGuiSignalManager, SIGNAL(ownerRemoved(const Licq::UserId&)),
      mySystemMenu, SLOT(removeOwner(const Licq::UserId&)));

  if (conf->mainwinRect().isValid())
    setGeometry(conf->mainwinRect());
  else
  {
    QSize newSize = myUserView->sizeHint();
    if (newSize.width() > newSize.height())
      newSize.transpose();
    myUserView->resize(newSize);
    adjustSize();
  }
  updateSkin();
  connect(Config::Skin::active(), SIGNAL(changed()), SLOT(updateSkin()));
  connect(Config::General::instance(), SIGNAL(styleChanged()), SLOT(updateSkin()));

  updateGroups(true);

  setMiniMode(conf->miniMode());
  setVisible(!conf->mainwinStartHidden() && !bStartHidden);

  // verify we exist
  if (gUserManager.NumOwners() == 0)
    OwnerManagerDlg::showOwnerManagerDlg();
  else
  {
    // Do we need to get a password
    o = gUserManager.FetchOwner(LICQ_PPID, LOCK_R);
    if (o != NULL)
    {
     if (o->password().empty())
     {
       gUserManager.DropOwner(o);
       new UserSelectDlg();
     }
     else
       gUserManager.DropOwner(o);
    }
  }

#ifdef USE_KDE
  /* TODO
  kdeIMInterface = new LicqKIMIface(KApplication::dcopClient()->appId(), this);
  connect(kdeIMInterface,
      SIGNAL(sendMessage(const Licq::UserId&, unsigned long, const QString&)),
      gLicqGui, SLOT(sendMsg(const Licq::UserId&, unsigned long, const QString&)));
  connect(kdeIMInterface,
      SIGNAL(sendFileTransfer(const Licq::UserId&, unsigned long,
          const QString&, const QString&)),
      gLicqGui, SLOT(sendFileTransfer(const Licq::UserId&, unsigned long,
          const QString&, const QString&)));
  connect(kdeIMInterface,
      SIGNAL(sendChatRequest(const Licq::UserId&, unsigned long)),
      gLicqGui, SLOT(sendChatRequest(const Licq::UserId&, unsigned long)));
  connect(kdeIMInterface,
      SIGNAL(addUser(const Licq::UserId&)),
      SLOT(addUser(const Licq::UserId&)));
  */
#endif

  list<unsigned long> protocolIds;
  FOR_EACH_OWNER_START(LOCK_R)
  {
    unsigned long ppid = pOwner->ppid();
#ifdef USE_KDE
    // TODO
    // kdeIMInterface->addProtocol(protocol->getName(), ppid);
#endif
    protocolIds.push_back(ppid);
  }
  FOR_EACH_OWNER_END

  BOOST_FOREACH(unsigned long protocolId, protocolIds)
    slot_protocolPlugin(protocolId);

  // Check if MainWin should be sticky
  if (Config::General::instance()->mainwinSticky())
    setMainwinSticky(true);
}

MainWindow::~MainWindow()
{
  delete myUserView;

  gMainWindow = NULL;
}

void MainWindow::updateConfig()
{
  Config::General* generalConfig = Config::General::instance();

  setMainwinSticky(generalConfig->mainwinSticky());
  setMiniMode(generalConfig->miniMode());

  // Redraw group/event label with new settings
  updateEvents();
}

void MainWindow::updateShortcuts()
{
  Config::Shortcuts* shortcuts = Config::Shortcuts::instance();

  myViewEventAction->setShortcut(shortcuts->getShortcut(Config::Shortcuts::MainwinUserViewMessage));
  mySendMessageAction->setShortcut(shortcuts->getShortcut(Config::Shortcuts::MainwinUserSendMessage));
  mySendUrlAction->setShortcut(shortcuts->getShortcut(Config::Shortcuts::MainwinUserSendUrl));
  mySendFileAction->setShortcut(shortcuts->getShortcut(Config::Shortcuts::MainwinUserSendFile));
  mySendChatRequestAction->setShortcut(shortcuts->getShortcut(Config::Shortcuts::MainwinUserSendChatRequest));
  myCheckUserArAction->setShortcut(shortcuts->getShortcut(Config::Shortcuts::MainwinUserCheckAutoresponse));
  myViewHistoryAction->setShortcut(shortcuts->getShortcut(Config::Shortcuts::MainwinUserViewHistory));
}

void MainWindow::trayIconClicked()
{
  if (isVisible() && !isMinimized() && isActiveWindow())
  {
    hide();
  }
  else
  {
    show();
#ifdef USE_KDE
    KWindowSystem::setOnDesktop(winId(), KWindowSystem::currentDesktop());
#endif
    if (isMaximized())
      showMaximized();
    else
      showNormal();

    // Sticky state is lost when window is hidden so restore it now
    if (Config::General::instance()->mainwinSticky())
      setMainwinSticky(true);

    activateWindow();
    raise();
  }
}

void MainWindow::updateSkin()
{
  Config::Skin* skin = Config::Skin::active();

#define CLEAR(elem) \
  if ((elem) != NULL) \
  { \
    delete (elem); \
    (elem) = NULL; \
  }

  // Set the background pixmap and mask
  if (skin->frame.pixmap.isNull())
    setPalette(QPalette());

  if (skin->frame.mask.isNull())
    clearMask();

  // System Button
  CLEAR(mySystemButton);
  CLEAR(myMenuBar);

  if (skin->frame.hasMenuBar ||
      skin->btnSys.rect.isNull())
  {
#ifdef USE_KDE
    myMenuBar = new KMenuBar(this);
#else
    myMenuBar = new QMenuBar(this);
#endif
    mySystemMenu->setTitle(skin->btnSys.caption.isNull() ?
        tr("&System") : skin->btnSys.caption);
    myMenuBar->addMenu(mySystemMenu);
    myMenuBar->setMinimumWidth(mySystemMenu->width());
    myMenuBar->show();
#ifndef Q_WS_MAC
    skin->AdjustForMenuBar(myMenuBar->height());
#endif
  }
  else
  {
    mySystemButton = new SkinnableButton(skin->btnSys, tr("System"), this);
    mySystemButton->setMenu(mySystemMenu);
    mySystemButton->show();
  }

  unsigned minHeight = skin->frame.border.top + skin->frame.border.bottom;
  setMinimumHeight(minHeight);
  setMaximumHeight(Config::General::instance()->miniMode() ?
      minHeight : QWIDGETSIZE_MAX);

  // Group Combo Box
  myUserGroupsBox->applySkin(skin->cmbGroups);
  myUserGroupsBox->setVisible(!skin->cmbGroups.rect.isNull());

  // Message Label
  CLEAR(myMessageField);
  if (!skin->lblMsg.rect.isNull())
  {
    myMessageField = new SkinnableLabel(skin->lblMsg,
        mySystemMenu->getGroupMenu(), this);
    connect(myMessageField, SIGNAL(doubleClicked()),
        gLicqGui, SLOT(showNextEvent()));
    connect(myMessageField, SIGNAL(wheelDown()), SLOT(nextGroup()));
    connect(myMessageField, SIGNAL(wheelUp()), SLOT(prevGroup()));
    myMessageField->setToolTip(tr("Right click - User groups\n"
          "Double click - Show next message"));
    myMessageField->show();
  }

  // Status Label
  CLEAR(myStatusField);
  if (!skin->lblStatus.rect.isNull())
  {
    myStatusField = new SkinnableLabel(skin->lblStatus,
        mySystemMenu->getStatusMenu(), this);
    connect(myStatusField, SIGNAL(doubleClicked()), SLOT(showAwayMsgDlg()));
    myStatusField->setToolTip(tr("Right click - Status menu\n"
          "Double click - Set auto response"));
    myStatusField->show();
  }

#undef CLEAR

  // update geometry and contents of the elements
  resizeEvent(NULL);
  updateEvents();
  updateStatus();
}

void MainWindow::CreateUserView()
{
  myUserView = new UserView(gGuiContactList, this);
  connect (myUserView, SIGNAL(userDoubleClicked(const Licq::UserId&)),
      gLicqGui, SLOT(showDefaultEventDialog(const Licq::UserId&)));
}

void MainWindow::resizeEvent(QResizeEvent* /* e */)
{
  Config::Skin* skin = Config::Skin::active();

  myUserView->setGeometry(skin->frame.border.left, skin->frame.border.top,
                        width() - skin->frameWidth(), height() - skin->frameHeight());

  // Resize the background pixmap and mask
  QPixmap pixmap = skin->mainwinPixmap(width(), height());
  if (!pixmap.isNull())
  {
    QPalette pal(palette());
    pal.setBrush(backgroundRole(), pixmap);
    setPalette(pal);
  }
  QPixmap mask = skin->mainwinMask(width(), height());
  if (!mask.isNull())
    setMask(mask);

#define UPDATE(wid, group) \
  if ((wid) != NULL) \
    (wid)->setGeometry(skin->group.borderToRect(this));

  // Set geometry of our widgets
  UPDATE(myUserGroupsBox, cmbGroups);
  UPDATE(myMessageField, lblMsg);
  UPDATE(myStatusField, lblStatus);
  UPDATE(mySystemButton, btnSys)
  else
    myMenuBar->resize(contentsRect().width(), myMenuBar->height());

#undef UPDATE

  if (isVisible())
    saveGeometry();
}

void MainWindow::saveGeometry()
{
  Config::General* conf = Config::General::instance();
  QRect geom = geometry();

  if (myInMiniMode)
    geom.setHeight(conf->mainwinRect().height());

  conf->setMainwinRect(geom);
}

void MainWindow::moveEvent(QMoveEvent* /* e */)
{
  saveGeometry();
}

void MainWindow::closeEvent(QCloseEvent* e)
{
  e->ignore();

  if (gLicqGui->dockIcon() != NULL)
    hide();
  else
    slot_shutdown();
}

void MainWindow::removeUserFromList()
{
  UserId userId = myUserView->currentUserId();

  gLicqGui->removeUserFromList(userId, this);
}

void MainWindow::removeUserFromGroup()
{
  int groupId = Config::ContactList::instance()->groupId();

  // Removing "All users" is the same as removing user from the list
  if (groupId == ContactListModel::AllUsersGroupId ||
      groupId == ContactListModel::AllGroupsGroupId)
  {
    removeUserFromList();
    return;
  }

  // Get currently selected user
  UserId userId = myUserView->currentUserId();

  gLicqGui->setUserInGroup(userId, groupId, false);
}

void MainWindow::callUserFunction(QAction* action)
{
  int index = action->data().toInt();
  UserId userId = myUserView->currentUserId();

  if (index == -1)
    gLicqGui->showViewEventDialog(userId);
  else
    gLicqGui->showEventDialog(index, userId);
}

void MainWindow::checkUserAutoResponse()
{
  UserId userId = myUserView->currentUserId();
  if (USERID_ISVALID(userId))
    new ShowAwayMsgDlg(userId, true);
}

void MainWindow::showUserHistory()
{
  UserId userId = myUserView->currentUserId();
  if (USERID_ISVALID(userId))
    new HistoryDlg(userId);
}

void MainWindow::hide()
{
  if (gLicqGui->dockIcon() != NULL)
    QWidget::hide();
}

void MainWindow::mousePressEvent(QMouseEvent* m)
{
   myMouseX = m->x();
   myMouseY = m->y();
}

/*! \brief Drags the mainwindow around
 *
 * If the appropriate option is set (EnableMainwinMouseMovement = 1)
 * this drags the mainwindow around when moving the mouse and left
 * button is pressed.
 */
void MainWindow::mouseMoveEvent(QMouseEvent* m)
{
  if (Config::General::instance()->mainwinDraggable() && (m->buttons() == Qt::LeftButton))
  {
    int deltaX = m->x() - myMouseX;
    int deltaY = m->y() - myMouseY;
    move(x() + deltaX, y() + deltaY);
  }
}

void MainWindow::slot_updatedUser(const Licq::UserId& userId, unsigned long subSignal, int argument)
{
  switch(subSignal)
  {
    case USER_EVENTS:
    {
      // Skip all this if it was just an away message check
      if (argument == 0)
        break;

      // Otherwise an event was added or removed
      updateEvents();
      // autoRaise if needed
      if (Config::General::instance()->autoRaiseMainwin() &&
          argument > 0)
        raise();

      // Fall through
    }
    case USER_STATUS:
#ifdef USE_KDE
      // TODO
      // kdeIMInterface->userStatusChanged(id, ppid);
#endif
    case USER_BASIC: // for alias
    case USER_SETTINGS: // for online notify
    case USER_SECURITY:
    case USER_TYPING:
    {
      if (gUserManager.isOwner(userId))
      {
        if (subSignal == USER_STATUS ||
            subSignal == USER_SETTINGS)
          break;

        myCaption = "Licq (|)";
        const LicqUser* o = gUserManager.fetchUser(userId, LOCK_R);
        if (o != NULL)
        {
          myCaption.replace("|", QString::fromUtf8(o->GetAlias()));
          gUserManager.DropUser(o);
        }
        else
        {
          myCaption.replace("|", tr("Error! No owner set"));
        }
        QString caption = myCaption;
        if (windowTitle()[0] == '*')
          caption.prepend("* ");

        setWindowTitle(caption);
        break;
      }

      const LicqUser* u = gUserManager.fetchUser(userId, LOCK_R);
      if (u == NULL)
      {
        gLog.Warn("%sMainWindow::slot_updatedUser(): Invalid user received: %s\n",
            L_ERRORxSTR, USERID_TOSTR(userId));
        break;
      }

      if (subSignal == USER_STATUS &&
          argument == 1 &&
          Config::General::instance()->trayMsgOnlineNotify())
      {
        // User on notify list went online -> show popup at systray icon
        if (gLicqGui->dockIcon() != NULL && u->OnlineNotify())
        {
          QString alias = QString::fromUtf8(u->GetAlias());
          QPixmap px = IconManager::instance()->iconForUser(u);
          gLicqGui->dockIcon()->popupMessage(alias, tr("is online"), px, 4000);
        }
      }

      gUserManager.DropUser(u);

      break;
    }
  }
}

void MainWindow::slot_updatedList(unsigned long subSignal)
{
  switch(subSignal)
  {
    case LIST_REMOVE:
      updateEvents();
      break;
  }
}

void MainWindow::updateEvents()
{
  QString szCaption;
  unsigned short nNumOwnerEvents = 0;

  FOR_EACH_OWNER_START(LOCK_R)
  {
    nNumOwnerEvents += pOwner->NewMessages();
  }
  FOR_EACH_OWNER_END

  unsigned short nNumUserEvents = LicqUser::getNumUserEvents() - nNumOwnerEvents;

  if (myMessageField != NULL)
    myMessageField->setBold(false);
  QString s, l;

  if (nNumOwnerEvents > 0)
  {
    s = tr("SysMsg");
    l = tr("System Message");
    if (Config::General::instance()->boldOnMsg() &&
        myMessageField != NULL)
      myMessageField->setBold(true);
    szCaption = "* " + myCaption;
  }
  else if (nNumUserEvents > 0)
  {
    s = tr("%1 msg%2").arg(nNumUserEvents).arg(nNumUserEvents == 1 ? tr(" ") : tr("s"));
    l = tr("%1 message%2").arg(nNumUserEvents).arg(nNumUserEvents == 1 ? tr(" ") : tr("s"));
    if (Config::General::instance()->boldOnMsg() &&
        myMessageField != NULL)
      myMessageField->setBold(true);
    szCaption = "* " + myCaption;
  }
  else
  {
    // Update the msg label if necessary
    if (Config::General::instance()->showGroupIfNoMsg() && LicqUser::getNumUserEvents() == 0)
    {
      s = myUserGroupsBox->currentText();
      l = myUserGroupsBox->currentText();
    }
    else
    {
      s = tr("No msgs");
      l = tr("No messages");
    }
    szCaption = myCaption;
  }

  setWindowTitle(szCaption);

  if (myMessageField != NULL)
  {
    if (myMessageField->fontMetrics().width(l) +
        myMessageField->margin() > myMessageField->width())
      myMessageField->setText(s);
    else
      myMessageField->setText(l);
    myMessageField->update();
  }

  if (gLicqGui->dockIcon() != NULL)
    gLicqGui->dockIcon()->updateIconMessages(nNumUserEvents, nNumOwnerEvents);
}

void MainWindow::setCurrentGroup(int index)
{
  int groupId = myUserGroupsBox->itemData(index).toInt();

  Config::ContactList::instance()->setGroup(groupId);
}

void MainWindow::nextGroup()
{
  int curGroupId = Config::ContactList::instance()->groupId();
  int groupId = 0;

  // Threaded view is selected, set all users
  if (curGroupId == ContactListModel::AllGroupsGroupId)
  {
    Config::ContactList::instance()->setGroup(ContactListModel::AllUsersGroupId);
    return;
  }

  {
    Licq::GroupListGuard groupList;
    BOOST_FOREACH(Licq::Group *g, **groupList)
    {
      Licq::GroupReadGuard group(g);

      // If current selection is all users, select first group in list
      if (groupId == 0 && curGroupId == ContactListModel::AllUsersGroupId)
      {
        Config::ContactList::instance()->setGroup(group->id());
        return;
      }

      // If previous group is selected, select current group
      if (groupId != 0 && curGroupId == groupId)
      {
        Config::ContactList::instance()->setGroup(group->id());
        return;
      }

      groupId = group->id();
    }
  }

  // Last user group is currently selected, set first system group
  if (groupId != 0 && curGroupId == groupId)
  {
    Config::ContactList::instance()->setGroup(ContactListModel::SystemGroupOffset + 0);
    return;
  }

  // No users groups exist and current selection is all users, set first system group
  if (groupId == 0 && curGroupId == ContactListModel::AllUsersGroupId)
  {
    Config::ContactList::instance()->setGroup(ContactListModel::SystemGroupOffset + 0);
    return;
  }

  groupId = 0;

  for (int i = ContactListModel::SystemGroupOffset; i <= ContactListModel::LastSystemGroup; ++i)
  {
    // If previous system group is selected, set current group
    if (groupId != 0 && groupId == curGroupId)
    {
      Config::ContactList::instance()->setGroup(i);
      return;
    }
    groupId = i;
  }

  // Last system group is currently selected or selection not found, set thread view
  Config::ContactList::instance()->setGroup(ContactListModel::AllGroupsGroupId);
}


void MainWindow::prevGroup()
{
  int curGroupId = Config::ContactList::instance()->groupId();
  int groupId = 0;

  // All users is selected, set thread view
  if (curGroupId == ContactListModel::AllUsersGroupId)
  {
    Config::ContactList::instance()->setGroup(ContactListModel::AllGroupsGroupId);
    return;
  }

  {
    Licq::GroupListGuard groupList;
    BOOST_FOREACH(Licq::Group* g, **groupList)
    {
      Licq::GroupReadGuard group(g);

      // If current group is selected, set previous group
      if (curGroupId == group->id())
      {
        if (groupId == 0)
          Config::ContactList::instance()->setGroup(ContactListModel::AllUsersGroupId);
        else
          Config::ContactList::instance()->setGroup(groupId);
        return;
      }

      groupId = group->id();
    }
  }

  // If first system group is selected, set last user group
  if (groupId != 0 && curGroupId == ContactListModel::SystemGroupOffset + 0)
  {
    if (groupId == 0)
      Config::ContactList::instance()->setGroup(ContactListModel::AllUsersGroupId);
    else
      Config::ContactList::instance()->setGroup(groupId);
    return;
  }
  groupId = 0;

  for (int i = ContactListModel::SystemGroupOffset; i <= ContactListModel::LastSystemGroup; ++i)
  {
    // If current system group is selected, set previous group
    if (curGroupId == i)
    {
      Config::ContactList::instance()->setGroup(groupId);
      return;
    }
    groupId = i;
  }

  // If current selection is thread view or selection not found, set last system group
  Config::ContactList::instance()->setGroup(ContactListModel::LastSystemGroup);
}
void MainWindow::updateCurrentGroup()
{
  int groupId = Config::ContactList::instance()->groupId();

  // Update the combo box
  int index = myUserGroupsBox->findData(groupId);
  if (index == -1)
    return;
  myUserGroupsBox->setCurrentIndex(index);

  // Update the msg label if necessary
  if (myMessageField != NULL &&
      Config::General::instance()->showGroupIfNoMsg() &&
      LicqUser::getNumUserEvents() == 0)
    myMessageField->setText(myUserGroupsBox->currentText());
}

void MainWindow::updateGroups(bool initial)
{
  if (!initial)
  {
    // update group list in user and system menus
    mySystemMenu->updateGroups();
  }

  // update the combo box
  myUserGroupsBox->clear();

#define ADD_SYSTEMGROUP(groupId) \
  myUserGroupsBox->addItem(ContactListModel::systemGroupName(groupId), groupId);

  ADD_SYSTEMGROUP(ContactListModel::AllGroupsGroupId);
  ADD_SYSTEMGROUP(ContactListModel::AllUsersGroupId);

  FOR_EACH_GROUP_START_SORTED(LOCK_R)
  {
    myUserGroupsBox->addItem(QString::fromLocal8Bit(pGroup->name().c_str()), pGroup->id());
  }
  FOR_EACH_GROUP_END

  for (int i = ContactListModel::SystemGroupOffset; i <= ContactListModel::LastSystemGroup; i++)
  {
    ADD_SYSTEMGROUP(i);
  }

#undef ADD_SYSTEMGROUP

  updateCurrentGroup();
}

void MainWindow::updateStatus()
{
  if (gLicqGui->dockIcon() != NULL)
    gLicqGui->dockIcon()->updateIconStatus();

  if (myStatusField == NULL)
    return;

  IconManager* iconman = IconManager::instance();
  Config::Skin* skin = Config::Skin::active();
  QColor theColor = skin->offlineColor;

  myStatusField->clearPixmaps();
  myStatusField->clearPrependPixmap();
  myStatusField->setText(QString::null);

  {
    Licq::OwnerListGuard ownerList;
    Licq::OwnerList::const_iterator it = ownerList->begin();
    switch (ownerList->size())
    {
      case 0:
        break;
      case 1:
      {
        Licq::OwnerReadGuard o(*it);
        myStatusField->setText(Licq::User::statusToString(o->status()).c_str());
        myStatusField->setPrependPixmap(iconman->iconForUser(*o));
        if (o->status() == Licq::User::OfflineStatus)
          theColor = skin->offlineColor;
        else if (o->status() & Licq::User::AwayStatuses)
          theColor = skin->awayColor;
        else
          theColor = skin->onlineColor;
        break;
      }
      default:
        BOOST_FOREACH(Licq::Owner* owner, **ownerList)
        {
          Licq::OwnerReadGuard o(owner);
          myStatusField->addPixmap(iconman->iconForUser(*o));
        }
    }
  }

  myStatusField->update();

  // set the color if it isn't set by the skin
  if (!skin->lblStatus.foreground.isValid() && theColor.isValid())
  {
    QPalette pal(myStatusField->palette());
    pal.setColor(QPalette::WindowText, theColor);
    myStatusField->setPalette(pal);
  }
}

void MainWindow::showAwayMsgDlg()
{
  //TODO iterate all owners that support fetching away message
  const ICQOwner* o = gUserManager.FetchOwner(LICQ_PPID, LOCK_R);

  if (o == NULL)
    return;

  unsigned status = o->status();
  gUserManager.DropOwner(o);

  AwayMsgDlg::showAwayMsgDlg(status);
}

void MainWindow::slot_shutdown()
{
  Licq::gDaemon.Shutdown();
}

void MainWindow::slot_logon()
{
  updateStatus();
}

void MainWindow::slot_protocolPlugin(unsigned long nPPID)
{
  UserId userId = gUserManager.ownerUserId(nPPID);
  if (USERID_ISVALID(userId))
    mySystemMenu->addOwner(userId);

  updateStatus();

#ifdef USE_KDE
  // TODO
  // let KDE IM interface know about the new protocol
  // kdeIMInterface->addProtocol(QString(pName), nPPID);
#endif
}

void MainWindow::slot_doneOwnerFcn(const LicqEvent* e)
{
  updateStatus();
  switch (e->SNAC())
  {
    case MAKESNAC(ICQ_SNACxFAM_SERVICE, ICQ_SNACxSRV_SETxSTATUS):
      if (e->Result() != EVENT_SUCCESS)
        WarnUser(this, tr("Logon failed.\nSee network window for details."));
      break;

/*
    case ICQ_MDxSND_AUTHORIZE:
       if (e->Result() != EVENT_ACKED)
         WarnUser(this, tr("Error sending authorization."));
       else
         InformUser(this, tr("Authorization granted."));
       break;
*/
    default:
       break;
  }
}

void MainWindow::slot_updateContactList()
{
  gLicqDaemon->icqUpdateContactList();
}

void MainWindow::showAboutBox()
{
  new AboutDlg(this);
}

void MainWindow::showStats()
{
  new StatsDlg(this);
}

void MainWindow::setMiniMode(bool miniMode)
{
  if (miniMode == myInMiniMode)
    return;

  myInMiniMode = miniMode;

  if (miniMode)
    setMaximumHeight(minimumHeight());
  else
  {
    setMaximumHeight(QWIDGETSIZE_MAX);
    resize(width(), Config::General::instance()->mainwinRect().height());
  }

  if (myUserView != NULL)
    myUserView->setVisible(!miniMode);
}

void MainWindow::slot_pluginUnloaded(unsigned long _nPPID)
{
  UserId userId = gUserManager.ownerUserId(_nPPID);
  if (USERID_ISVALID(userId))
    mySystemMenu->removeOwner(userId);

#ifdef USE_KDE
  // TODO
  // kdeIMInterface->removeProtocol(_nPPID);
#endif
}

void MainWindow::showHints()
{
  QString hints = tr(
    "<h2>Hints for Using<br>the Licq Qt-GUI Plugin</h2><hr>"
    "<ul>"
    "<li>Change your status by right clicking on the status label.</li>"
    "<li>Change your auto response by double-clicking on the status label.</li>"
    "<li>View system messages by double clicking on the message label.</li>"
    "<li>Change groups by right clicking on the message label.</li>"
    "<li>Use the following shortcuts from the contact list:<ul>"
    "<li><tt>Ctrl-M : </tt>Toggle mini-mode</li>"
    "<li><tt>Ctrl-O : </tt>Toggle show offline users</li>"
    "<li><tt>Ctrl-T : </tt>Toggle group show</li>"
    "<li><tt>Ctrl-X : </tt>Exit</li>"
    "<li><tt>Ctrl-H : </tt>Hide</li>"
    "<li><tt>Ctrl-I : </tt>View the next message</li>"
    "<li><tt>Ctrl-V : </tt>View message</li>"
    "<li><tt>Ctrl-S : </tt>Send message</li>"
    "<li><tt>Ctrl-U : </tt>Send Url</li>"
    "<li><tt>Ctrl-C : </tt>Send chat request</li>"
    "<li><tt>Ctrl-F : </tt>Send File</li>"
    "<li><tt>Ctrl-A : </tt>Check Auto response</li>"
    "<li><tt>Ctrl-P : </tt>Popup all messages</li>"
    "<li><tt>Ctrl-L : </tt>Redraw user window</li>"
    "<li><tt>Delete : </tt>Delete user from current group</li>"
    "<li><tt>Ctrl-Delete : </tt>Delete user from contact list</li></ul>"
    "<li>Hold control while clicking on close in the function window to remove"
    "   the user from your contact list.</li>"
    "<li>Hit Ctrl-Enter from most text entry fields to select \"Ok\" or \"Accept\"."
    "   For example in the send tab of the user function window.</li>"
    "<li>Here is the complete list of user % options, which can be used in <b>OnEvent</b>"
    "   parameters, <b>auto responses</b>, and <b>utilities</b>:") + gMainWindow->usprintfHelp +
    "</li></ul><hr>" +
#ifndef USE_KDE
    "<h2>" + tr("Qt URL handling") + "</h2>"
    "<center><table border=1 cellpadding=5>"
    "<tr><th colspan=2>Web</th><th colspan=2>E-mail (mailto:)</th></tr>"
    "<tr><th>KDE</th><th>GNOME</th><th>GNOME</th><th>KDE</th></tr>"
    "<tr><td align=center colspan=4>xdg-open</td></tr>"
    "<tr><td align=center colspan=2>$DEFAULT_BROWSER</td>"
        "<td colspan=2 rowspan=2>&nbsp;</td></tr>"
    "<tr><td align=center colspan=2>$BROWSER</td></tr>"
    "<tr><td align=center>kfmclient openURL</td>"
        "<td align=center colspan=2>gnome-open</td>"
        "<td align=center>kfmclient exec</td></tr>"
    "<tr><td align=center colspan=4>firefox</td></tr>"
    "<tr><td align=center colspan=4>mozilla</td></tr>"
    "<tr><td align=center colspan=4>netscape</td></tr>"
    "<tr><td align=center colspan=4>opera</td></tr>"
    "</table></center><hr>" +
#endif
    tr("<p>For more information, see the Licq webpage (<tt>http://www.licq.org</tt>).</p>");

  new HintsDlg(hints);
}

void MainWindow::showAutoResponseHints(QWidget* parent)
{
  AwayMsgDlg::showAutoResponseHints(parent);
}

void MainWindow::addUser(const Licq::UserId& userId)
{
  if (!USERID_ISVALID(userId))
    return;

  new AddUserDlg(userId);
}

void MainWindow::setMainwinSticky(bool sticky)
{
  Support::changeWinSticky(winId(), sticky);
}
