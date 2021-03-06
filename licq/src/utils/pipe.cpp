/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2010-2012 Licq developers <licq-dev@googlegroups.com>
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

#include <licq/pipe.h>

#include <fcntl.h>

using namespace Licq;

Pipe::Pipe()
{
  ::pipe(myFds);
}

Pipe::~Pipe()
{
  ::close(myFds[0]);
  ::close(myFds[1]);
}

ssize_t Pipe::read(void* buf, size_t count)
{
  return ::read(getReadFd(), buf, count);
}

ssize_t Pipe::write(const void* buf, size_t count)
{
  return ::write(getWriteFd(), buf, count);
}

void Pipe::setWriteBlocking(bool block)
{
  int f = ::fcntl(getWriteFd(), F_GETFL);
  ::fcntl(getWriteFd(), F_SETFL, (block ? (f & ~O_NONBLOCK) : (f | O_NONBLOCK)));
}
