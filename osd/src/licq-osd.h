/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2011 Licq developers
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

#ifndef LICQOSD_H
#define LICQOSD_H

#include <licq/generalplugin.h>


class OsdPlugin : public Licq::GeneralPlugin
{
public:
  OsdPlugin(Params& p);

  // From Licq::GeneralPlugin
  std::string name() const;
  std::string description() const;
  std::string version() const;
  std::string usage() const;
  std::string configFile() const;
  bool isEnabled() const;

protected:
  bool init(int argc, char** argv);
  int run();
};

#endif