// -*- c-basic-offset: 2 -*-
/* ----------------------------------------------------------------------------
 * Licq - A ICQ Client for Unix
 * Copyright (C) 1998 - 2009 Licq developers
 *
 * This program is licensed under the terms found in the LICENSE file.
 */

#include "config.h"

#include <assert.h>
#include <cstring>

#include <licq/packet.h>
#include "licq_events.h"
#include "licq_log.h"

#include "contactlist/user.h"

using namespace std;
using Licq::StringList;
using Licq::UserId;

//-----CSearchAck------------------------------------------------------------
CSearchAck::CSearchAck(const UserId& userId)
  : myUserId(userId)
{
  m_szAlias = m_szFirstName = m_szLastName = m_szEmail = NULL;
}

CSearchAck::~CSearchAck()
{
  free(m_szAlias);
  free(m_szFirstName);
  free(m_szLastName);
  free(m_szEmail);
}


//-----CExtendedAck----------------------------------------------------------
CExtendedAck::CExtendedAck(bool bAccepted, unsigned short nPort, const char* szResponse)
{
  m_bAccepted = bAccepted;
  m_nPort = nPort;
  m_szResponse = strdup(szResponse);
}

CExtendedAck::~CExtendedAck()
{
  free(m_szResponse);
}


//-----ICQEvent::constructor----------------------------------------------------
LicqEvent::LicqEvent(unsigned long id, int _nSocketDesc, Licq::Packet* p,
    ConnectType _eConnect, const UserId& userId, CUserEvent *e)
//   : m_xBuffer(p.getBuffer())
{
  // set up internal variables
  m_bCancelled = false;
  m_Deleted = false;
  m_NoAck = false;
  if (p)
  {
    m_pPacket = p;
    m_nChannel = p->Channel();
    m_nCommand = p->Command();
    m_nSNAC = p->SNAC();
    m_nSubCommand = p->SubCommand();
    m_nSequence = p->Sequence();
    m_nSubSequence = p->SubSequence();
    m_nSubType = (p->SNAC() & 0xFFFF);
    m_nExtraInfo = p->ExtraInfo();
  } else
  {
    m_pPacket = NULL;
    m_nChannel = 0;
    m_nCommand = 0;
    m_nSNAC = 0;
    m_nSubCommand = 0;
    m_nSequence = 0;
    m_nSubSequence = 0;
    m_nSubType = 0;
    m_nExtraInfo = 0;
  }
  myUserId = userId;
  m_eConnect = _eConnect;
  m_pUserEvent = e;
  m_nSocketDesc = _nSocketDesc;
  m_pExtendedAck = NULL;
  m_pSearchAck = NULL;
  m_pUnknownUser = NULL;
  m_nSubResult = ICQ_TCPxACK_ACCEPT;
  thread_plugin = pthread_self();
  thread_running = false;

  m_nEventId = id;
}

//-----ICQEvent::constructor----------------------------------------------------
LicqEvent::LicqEvent(const LicqEvent* e)
//   : m_xBuffer(e->m_xBuffer)
{
  m_nEventId = e->m_nEventId;

  // set up internal variables
  m_pPacket = NULL;
  m_Deleted = false;
  m_NoAck = false;
  m_bCancelled = e->m_bCancelled;
  m_nChannel = e->m_nChannel;
  m_nSNAC = e->m_nSNAC;
  m_nCommand = e->m_nCommand;
  m_nSubCommand = e->m_nSubCommand;
  m_nSequence = e->m_nSequence;
  m_nSubSequence = e->m_nSubSequence;
  m_nSubType = e->m_nSubType;
  m_nExtraInfo = e->m_nExtraInfo;
  myUserId = e->myUserId;
  m_eConnect = e->m_eConnect;
  m_eResult = e->m_eResult;
  m_nSubResult = e->m_nSubResult;
  if (e->m_pUserEvent != NULL)
    m_pUserEvent = e->m_pUserEvent->Copy();
  else
    m_pUserEvent = NULL;
  m_nSocketDesc = e->m_nSocketDesc;
  m_pExtendedAck = NULL;
  m_pSearchAck = NULL;
  m_pUnknownUser = NULL;

  thread_plugin = e->thread_plugin;
  thread_send = e->thread_send;
  thread_running = e->thread_running;
}



//-----ICQEvent::destructor-----------------------------------------------------
LicqEvent::~LicqEvent()
{
  assert(!m_Deleted);
  m_Deleted = true;

  delete m_pPacket;
  delete m_pUserEvent;
  delete m_pExtendedAck;
  delete m_pSearchAck;
  delete m_pUnknownUser;
}


//-----ICQEvent::AttachPacket---------------------------------------------------
void ICQEvent::AttachPacket(Licq::Packet* p)
{
  m_pPacket = p;
  m_nChannel = p->Channel();
  m_nCommand = p->Command();
  m_nSNAC = p->SNAC();
  m_nSubCommand = p->SubCommand();
  m_nSequence = p->Sequence();
  m_nSubSequence = p->SubSequence();
  m_nSubType = (p->SNAC() & 0xFFFF);
  m_nExtraInfo = p->ExtraInfo();
}
  
//-----ICQEvent::CompareEvent---------------------------------------------------
bool ICQEvent::CompareEvent(int sockfd, unsigned short _nSequence) const
{
   return(m_nSocketDesc == sockfd && m_nSequence == _nSequence);
}

bool ICQEvent::CompareEvent(unsigned short nEventId) const
{
  return (m_nEventId == nEventId);
}

bool ICQEvent::CompareSubSequence(unsigned long _nSubSequence) const
{
  return (m_nSubSequence == _nSubSequence);
}

bool ICQEvent::Equals(unsigned long nEventId) const
{
  return (this == NULL && nEventId == 0) || CompareEvent(nEventId);
}


unsigned long ICQEvent::EventId() const
{
  return this == NULL ? 0 : m_nEventId;
}

const Licq::User* LicqEvent::UnknownUser() const
{
  return m_pUnknownUser;
}

// Returns the event and transfers ownership to the calling function
CUserEvent *ICQEvent::GrabUserEvent()
{
  CUserEvent *e = m_pUserEvent; m_pUserEvent = NULL; return e;
}

CSearchAck *ICQEvent::GrabSearchAck()
{
  CSearchAck *a = m_pSearchAck; m_pSearchAck = NULL; return a;
}

Licq::User* LicqEvent::GrabUnknownUser()
{
  Licq::User* u = m_pUnknownUser;
  m_pUnknownUser = NULL;
  return u;
}
