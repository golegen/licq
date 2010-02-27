#ifndef OSCARSERVICE_H
#define OSCARSERVICE_H

#include <list>
#include <pthread.h>

#include <boost/shared_array.hpp>

class CBuffer;
class CPacket;
class LicqEvent;
class ProxyServer;

namespace Licq
{
class UserId;
}

enum EOscarServiceStatus {STATUS_UNINITIALIZED, STATUS_SERVICE_REQ_SENT,
                          STATUS_SERVICE_REQ_ACKED, STATUS_CONNECTED,
                          STATUS_SRV_READY_RECV, STATUS_SRV_VER_RECV,
                          STATUS_SRV_RATE_RECV, STATUS_READY};

void *OscarServiceSendQueue_tep(void *p);

class COscarService
{
public:
  COscarService(unsigned short Fam);
  ~COscarService();
  bool Initialize();
  bool ProcessPacket(CBuffer &packet);
  unsigned long SendEvent(const Licq::UserId& userId, unsigned short SubType, bool Request);
  void ClearQueue();

  void SetConnectCredential(char *Server, unsigned short Port,
                            char *Cookie, unsigned short CookieLen);
  void ChangeStatus(EOscarServiceStatus s);
  int GetSocketDesc() { return mySocketDesc; }
  void ResetSocket() { mySocketDesc = -1; }
  unsigned short GetFam() { return myFam; }

protected:
  unsigned short myFam;
  int mySocketDesc;
  ProxyServer *myProxy;
  EOscarServiceStatus myStatus;
  char *myServer;
  boost::shared_array<char> myCookie;
  unsigned short myPort, myCookieLen;
  std::list<LicqEvent*> mySendQueue;
  pthread_mutex_t mutex_sendqueue;
  pthread_cond_t cond_sendqueue;
  pthread_mutex_t mutex_status;
  pthread_cond_t cond_status;

  bool SendPacket(CPacket *packet);
  bool WaitForStatus(EOscarServiceStatus s);
  bool SendBARTFam(LicqEvent* event);
  void ProcessNewChannel(CBuffer &packet);
  void ProcessDataChannel(CBuffer &packet);
  void ProcessServiceFam(CBuffer &packet, unsigned short SubType, unsigned long RequestId);
  void ProcessBARTFam(CBuffer &packet, unsigned short SubType, unsigned long RequestId);

  friend void *OscarServiceSendQueue_tep(void *p);
};

#endif
