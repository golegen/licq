/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 1999-2006 Licq developers
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

#ifndef GUIDEFINES_H
#define GUIDEFINES_H

#include "config.h"

#define QTGUI_DIR "qt4-gui/"
#define DOCK_DIR "dock/"
#define EMOTICONS_DIR "emoticons/"
#define EXTICONS_DIR "exticons/"
#define ICONS_DIR "icons/"
#define SKINS_DIR "skins/"
#define QTGUI_CONFIGFILE QTGUI_DIR "config.ini"

#define MAX_COLUMNCOUNT 4

#ifdef USE_KDE
# define PLUGIN_NAME "kde4-gui"
# define DEFAULT_URL_VIEWER tr("KDE default")
#else
# define PLUGIN_NAME "qt4-gui"
# define DEFAULT_URL_VIEWER "viewurl-mozilla.sh"
#endif

// Defined here until there is a way to get constant from daemon
#define MSN_PPID 0x4D534E5F

namespace LicqQtGui
{

enum UserFunctions {
  mnuUserView,
  mnuUserSendMsg,
  mnuUserSendUrl,
  mnuUserSendChat,
  mnuUserSendFile,
  mnuUserSendContact,
  mnuUserSendSms,
  mnuUserGeneral,
};

} // namespace LicqQtGui

#endif