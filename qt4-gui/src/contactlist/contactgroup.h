// -*- c-basic-offset: 2 -*-
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

#ifndef CONTACTGROUP_H
#define CONTACTGROUP_H

#include <QList>
#include <QString>
#include <QVariant>

#include "contactitem.h"
#include "contactlist.h"

namespace LicqQtGui
{
class ContactUser;
class ContactBar;

/**
 * A group in the contact list
 *
 * This class is used internally by ContactList and should not be accessed from any other class
 */
class ContactGroup : public ContactItem
{
  Q_OBJECT

public:
  /**
   * Constructor, creates an empty group
   *
   * @param id Group id
   * @param name Group name
   */
  ContactGroup(unsigned short id, QString name);

  /**
   * Destructor, will delete all user instances in the group
   */
  virtual ~ContactGroup();

  /**
   * Get the Licq id for this group
   */
  unsigned short groupId() const
  { return myGroupId; }

  /**
   * Get number of items in this group (users and separator bars)
   */
  int rowCount() const;

  /**
   * Get the position of a user in this group
   *
   * @param user A user in this group
   * @return The row index of the user
   */
  int indexOf(ContactUser* user) const;

  /**
   * Get an item from this group (user or separator bar)
   *
   * @param row index of the item to return
   * @return The item for the requested row if present or 0 otherwise
   */
  ContactItem* item(int row) const;

  /**
   * Get a user from this group
   *
   * @param user The userdata to get a user instance for
   * @return The user for the requested row if present or 0 otherwise
   */
  ContactUser* user(ContactUserData* user) const;

  /**
   * Add a user to this sub group
   * Note: Do not call this method directly, it should only be called from the user instance constructor.
   *
   * @param user The user instance to add
   * @param subGroup The initial sub group to place the user in
   */
  void addUser(ContactUser* user, ContactListModel::SubGroupType subGroup);

  /**
   * Remove a user from this sub group
   * Note: Do not call this method directly, it should only be called from the user instance destructor.
   *
   * @param user The user instance to remove
   * @param subGroup The current sub group of the user
   */
  void removeUser(ContactUser* user, ContactListModel::SubGroupType subGroup);

  /**
   * Update the sub group counters
   *
   * @param oldSubGroup The previous sub group
   * @param newSubGroup The new sub group
   * @param eventCounter Current number of events for contact that needs to be moved between the subgroups counters
   */
  void updateSubGroup(ContactListModel::SubGroupType oldSubGroup, ContactListModel::SubGroupType newSubGroup, int eventCounter);

  /**
   * Update unread event counter for group
   *
   * @param counter Number to increase or decrease event counter by
   * @param subGroup Current subgroup for user that also should be updated
   */
  void updateNumEvents(int counter, ContactListModel::SubGroupType subGroup);

  /**
   * Update visibility counter
   *
   * @param increase True if counter should be increased
   */
  void updateVisibility(bool increase);

  /**
   * Get data for this group
   *
   * @param column A valid column in the contact list
   * @param role The qt role to get data for
   * @return Data for this group
   */
  QVariant data(int column, int role) const;

signals:
  /**
   * Signal emitted when data for the group has changed
   */
  void dataChanged(ContactGroup* group);

  /**
   * Signal emitted when data for a bar has changed
   */
  void barDataChanged(ContactBar* bar, int row);

private:
  unsigned short myGroupId;
  QString myName;
  QString mySortKey;
  int myEvents;
  QList<ContactUser*> myUsers;
  ContactBar* myBars[3];
  int myVisibleContacts;
};

} // namespace LicqQtGui

#endif
