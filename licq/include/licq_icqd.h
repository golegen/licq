/*
ICQ.H
header file containing all the main procedures to interface with the ICQ server at mirabilis
*/

#ifndef ICQD_H
#define ICQD_H

#include <vector>
#include <list>
#include <deque>
#include <stdarg.h>
#include <stdio.h>

#include "licq_events.h"
#include "licq_onevent.h"
#include "licq_user.h"
#include "licq_plugind.h"
#include "licq_color.h"

class CPlugin;
class CPacket;
class CPacketTcp;
class CLicq;
class CUserManager;
class ICQUser;
class CICQEventTag;
class TCPSocket;
class SrvSocket;
class INetSocket;
class ProxyServer;

const unsigned short IGNORE_MASSMSG    = 1;
const unsigned short IGNORE_NEWUSERS   = 2;
const unsigned short IGNORE_EMAILPAGER = 4;
const unsigned short IGNORE_WEBPANEL   = 8;

//-----Stats-----------------------------------------------------------------
class CDaemonStats
{
public:
  // Accessors
  unsigned long Total() { return m_nTotal; }
  unsigned long Today() { return m_nTotal - m_nOriginal; }
  const char *Name()    { return m_szName; }

protected:
  CDaemonStats();
  CDaemonStats(const char *, const char *);

  bool Dirty() { return m_nLastSaved != m_nTotal; }
  void ClearDirty() { m_nLastSaved = m_nTotal; }

  void Init();
  void Reset();
  void Inc() { m_nTotal++; }

  unsigned long m_nTotal;

  unsigned long m_nOriginal;
  unsigned long m_nLastSaved;
  char m_szTag[16];
  char m_szName[32];

friend class CICQDaemon;
};

typedef std::vector<CDaemonStats> DaemonStatsList;
#define STATS_EventsSent 0
#define STATS_EventsReceived 1
#define STATS_EventsRejected 2
#define STATS_AutoResponseChecked 3
// We will save the statistics to disk
#define SAVE_STATS


//=====CICQDaemon===============================================================
enum EDaemonStatus {STATUS_ONLINE, STATUS_OFFLINE_MANUAL, STATUS_OFFLINE_FORCED };

class CICQDaemon
{
public:
  CICQDaemon(CLicq *);
  ~CICQDaemon();
  int RegisterPlugin(unsigned long _nSignalMask);
  void UnregisterPlugin();
  bool Start();
  const char *Version();
  pthread_t *Shutdown();
  void SaveConf();

  // TCP (user) functions
  // Message
  unsigned long icqSendMessage(unsigned long nUin, const char *szMessage,
     bool bOnline, unsigned short nLevel, bool bMultipleRecipients = false,
     CICQColor *pColor = NULL);
  // Url
  unsigned long icqSendUrl(unsigned long nUin, const char *szUrl,
     const char *szDescription, bool bOnline, unsigned short nLevel,
     bool bMultipleRecipients = false, CICQColor *pColor = NULL);
  // Contact List
  unsigned long icqSendContactList(unsigned long nUin, UinList &uins,
     bool bOnline, unsigned short nLevel, bool bMultipleRecipients = false,
     CICQColor *pColor = NULL);
  // Auto Response
  unsigned long icqFetchAutoResponse(unsigned long nUin);
  // Chat Request
  unsigned long icqChatRequest(unsigned long nUin, const char *szReason,
     unsigned short nLevel);
  unsigned long icqMultiPartyChatRequest(unsigned long nUin,
     const char *szReason, const char *szChatUsers, unsigned short nPort,
     unsigned short nLevel);
  void icqChatRequestRefuse(unsigned long nUin, const char *szReason,
     unsigned long nSequence);
  void icqChatRequestAccept(unsigned long nUin, unsigned short nPort,
     unsigned long nSequence);
  void icqChatRequestCancel(unsigned long nUin, unsigned long nSequence);
  // File Transfer
  unsigned long icqFileTransfer(unsigned long nUin, const char *szFilename,
     const char *szDescription, unsigned short nLevel);
  void icqFileTransferRefuse(unsigned long nUin, const char *szReason,
     unsigned long nSequence);
  void icqFileTransferCancel(unsigned long nUin, unsigned long nSequence);
  void icqFileTransferAccept(unsigned long nUin, unsigned short nPort,
     unsigned long nSequence);
  unsigned long icqOpenSecureChannel(unsigned long nUin);
  unsigned long icqCloseSecureChannel(unsigned long nUin);
  void icqOpenSecureChannelCancel(unsigned long nUin, unsigned long nSequence);

  // Server functions
  void icqRegister(const char *_szPasswd);
  unsigned long icqFetchAutoResponseServer(unsigned long);
  unsigned long icqLogon(unsigned short logonStatus);
  unsigned long icqUserBasicInfo(unsigned long);
  unsigned long icqUserExtendedInfo(unsigned long);
  unsigned long icqRequestMetaInfo(unsigned long);

  unsigned long icqUpdateBasicInfo(const char *, const char *, const char *,
                                       const char *, bool);
  unsigned long icqUpdateExtendedInfo(const char *, unsigned short, const char *,
                                unsigned short, char, const char *,
                                const char *, const char *_sAbout, const char *);
  unsigned long icqSetWorkInfo(const char *_szCity, const char *_szState,
                           const char *_szPhone,
                           const char *_szFax, const char *_szAddress,
			   const char *_szZip, unsigned short _nCompanyCountry,
                           const char *_szName, const char *_szDepartment,
                           const char *_szPosition, const char *_szHomepage);
  unsigned long icqSetGeneralInfo(const char *szAlias, const char *szFirstName,
                              const char *szLastName, const char *szEmailPrimary,
                              const char *szCity,
                              const char *szState, const char *szPhoneNumber,
                              const char *szFaxNumber, const char *szAddress,
                              const char *szCellularNumber, const char *szZipCode,
                              unsigned short nCountryCode, bool bHideEmail);
  unsigned long icqSetEmailInfo(const char *szEmailSecondary, const char *szEmailOld);
  unsigned long icqSetMoreInfo(unsigned short nAge,
                           char nGender, const char *szHomepage,
                           unsigned short nBirthYear, char nBirthMonth,
                           char nBirthDay, char nLanguage1,
                           char nLanguage2, char nLanguage3);
  unsigned long icqSetSecurityInfo(bool bAuthorize, bool bHideIp, bool bWebAware);
  unsigned long icqSetAbout(const char *szAbout);
  unsigned long icqSetPassword(const char *szPassword);
  unsigned long icqSetStatus(unsigned short newStatus);
  unsigned long icqSetRandomChatGroup(unsigned long nGroup);
  unsigned long icqRandomChatSearch(unsigned long nGroup);
  unsigned long icqSearchWhitePages(const char *szFirstName,
                            const char *szLastName, const char *szAlias,
                            const char *szEmail, unsigned short nMinAge,
                            unsigned short nMaxAge, char nGender,
                            char nLanguage, const char *szCity,
                            const char *szState, unsigned short nCountryCode,
                            const char *szCoName, const char *szCoDept,
                            const char *szCoPos, const char *szKeyword,
			    bool bOnlineOnly);
  unsigned long icqSearchByUin(unsigned long);

  void icqLogoff();
  void icqRelogon(bool bChangeServer = false);
  unsigned long icqAuthorizeGrant(unsigned long nUin, const char *szMessage);
  unsigned long icqAuthorizeRefuse(unsigned long nUin, const char *szMessage);
  void icqRequestAuth(unsigned long _nUin, const char *_szMessage);
  void icqAlertUser(unsigned long _nUin);
  void icqAddUser(unsigned long, bool _bServerOnly = false);
  void icqAddGroup(const char *);
  void icqRemoveUser(unsigned long _nUin);
  void icqRemoveGroup(const char *);
  void icqRenameGroup(unsigned short _nGSID);
  void icqExportUsers(UinList &);
  void icqExportGroups(GroupList &);
  void icqUpdateContactList();

  // Visible/Invisible list functions
  void icqAddToVisibleList(unsigned long nUin);
  void icqRemoveFromVisibleList(unsigned long nUin);
  void icqToggleVisibleList(unsigned long nUin);
  void icqAddToInvisibleList(unsigned long nUin);
  void icqRemoveFromInvisibleList(unsigned long nUin);
  void icqToggleInvisibleList(unsigned long nUin);

  void PluginList(PluginsList &l);
  void PluginShutdown(int);
  void PluginEnable(int);
  void PluginDisable(int);
  bool PluginLoad(const char *, int, char **);

  void PluginUIViewEvent(unsigned long nUin) {
  	PushPluginSignal(new CICQSignal(SIGNAL_UI_VIEWEVENT, 0, nUin, 0, 0));
  }
  void PluginUIMessage(unsigned long nUin ) {
  	PushPluginSignal(new CICQSignal(SIGNAL_UI_MESSAGE,0,nUin, 0,0));
  }

  void UpdateAllUsers();
  void UpdateAllUsersInGroup(GroupType, unsigned short);
  void SwitchServer();
  void CancelEvent(unsigned long );
  void CancelEvent(ICQEvent *);
  bool OpenConnectionToUser(unsigned long nUin, TCPSocket *sock,
     unsigned short nPort);
  bool OpenConnectionToUser(const char *szAlias, unsigned long nIp,
     unsigned long nIntIp, TCPSocket *sock, unsigned short nPort,
     bool bSendIntIp);
  int StartTCPServer(TCPSocket *);
  void CheckBirthdays(UinList &);
  unsigned short BirthdayRange() { return m_nBirthdayRange; }
  void BirthdayRange(unsigned short r) { m_nBirthdayRange = r; }

  bool AddUserToList(unsigned long _nUin, bool bNotify = true);
  void AddUserToList(ICQUser *);
  void RemoveUserFromList(unsigned long _nUin);

  // SMS
  unsigned long icqSendSms(unsigned long nUin, const char *szMessage);
  
  // NOT MT SAFE
  const char *getUrlViewer();
  void setUrlViewer(const char *s);

  bool ViewUrl(const char *url);

  // ICQ Server options
  const char *ICQServer() {  return m_szICQServer;  }
  void SetICQServer(const char *s) {  SetString(&m_szICQServer, s);  }
  unsigned short ICQServerPort() {  return m_nICQServerPort;  }
  void SetICQServerPort(unsigned short p) {  m_nICQServerPort = p; }
  
  // Firewall options
  bool TCPEnabled();
  void SetTCPEnabled(bool b);

  // Proxy options
  void InitProxy();
  bool ProxyEnabled() {  return m_bProxyEnabled;  }
  void SetProxyEnabled(bool b) {  m_bProxyEnabled = b;  }
  unsigned short ProxyType() {  return m_nProxyType;  }
  void SetProxyType(unsigned short t) {  m_nProxyType = t;  }
  const char *ProxyHost() {  return m_szProxyHost;  }
  void SetProxyHost(const char *s) {  SetString(&m_szProxyHost, s);  }
  unsigned short ProxyPort() {  return m_nProxyPort;  }
  void SetProxyPort(unsigned short p) {  m_nProxyPort = p;  }
  bool ProxyAuthEnabled() {  return m_bProxyAuthEnabled;  }
  void SetProxyAuthEnabled(bool b) {  m_bProxyAuthEnabled = b;  }
  const char *ProxyLogin() {  return m_szProxyLogin;  }
  void SetProxyLogin(const char *s) {  SetString(&m_szProxyLogin, s);  }
  const char *ProxyPasswd() {  return m_szProxyPasswd;  }
  void SetProxyPasswd(const char *s) {  SetString(&m_szProxyPasswd, s);  }
  
  unsigned short TCPPortsLow() { return m_nTCPPortsLow; }
  unsigned short TCPPortsHigh() { return m_nTCPPortsHigh; }
  void SetTCPPorts(unsigned short p, unsigned short r);
  static bool CryptoEnabled();

  const char *Terminal();
  void SetTerminal(const char *s);
  bool Ignore(unsigned short n)      { return m_nIgnoreTypes & n; }
  void SetIgnore(unsigned short, bool);

  COnEventManager *OnEventManager()  { return &m_xOnEventManager; }
  bool AlwaysOnlineNotify();
  void SetAlwaysOnlineNotify(bool);
  CICQSignal *PopPluginSignal();
  ICQEvent *PopPluginEvent();

  // Server Side List functions
  bool UseServerContactList()         { return m_bUseSS; }
  void SetUseServerContactList(bool b)  { m_bUseSS = b; }

  // Statistics
  CDaemonStats *Stats(unsigned short n) { return n < 3 ? &m_sStats[n] : NULL; }
  DaemonStatsList &AllStats() { return m_sStats; }
  time_t ResetTime() { return m_nResetTime; }
  time_t StartTime() { return m_nStartTime; }
  time_t Uptime() { return time(NULL) - m_nStartTime; }
  void ResetStats();

protected:
  CLicq *licq;
  COnEventManager m_xOnEventManager;
  int pipe_newsocket[2], fifo_fd;
  FILE *fifo_fs;
  EDaemonStatus m_eStatus;
  char m_szConfigFile[MAX_FILENAME_LEN];

  char *m_szUrlViewer,
       *m_szTerminal,
       *m_szRejectFile;
  unsigned long m_nDesiredStatus,
                m_nIgnoreTypes;
  unsigned short m_nTCPPortsLow,
                 m_nTCPPortsHigh,
                 m_nMaxUsersPerPacket,
                 m_nServerSequence,
                 m_nErrorTypes,
                 m_nBirthdayRange;
  char m_szErrorFile[64];
  int m_nTCPSrvSocketDesc,
      m_nTCPSocketDesc;
  bool m_bShuttingDown,
       m_bLoggingOn,
       m_bRegistering,
       m_bOnlineNotifies,
       m_bAlwaysOnlineNotify;
  time_t m_tLogonTime;
  
  // ICQ Server
  char *m_szICQServer;
  unsigned short m_nICQServerPort;
  
  // Proxy
  bool m_bProxyEnabled;
  unsigned short m_nProxyType;
  char *m_szProxyHost;
  unsigned short m_nProxyPort;
  bool m_bProxyAuthEnabled;
  char *m_szProxyLogin;
  char *m_szProxyPasswd;
  ProxyServer *m_xProxy;
  
  // SS List
  bool m_bUseSS;

  // Statistics
  void FlushStats();
  DaemonStatsList m_sStats;
  time_t m_nStartTime, m_nResetTime;

  std::list <ICQEvent *> m_lxRunningEvents;
  pthread_mutex_t mutex_runningevents;
  std::list <ICQEvent *> m_lxExtendedEvents;
  pthread_mutex_t mutex_extendedevents;
  pthread_t thread_monitorsockets,
            thread_ping;

  pthread_cond_t cond_serverack;
  pthread_mutex_t mutex_serverack;
  unsigned short m_nServerAck;

  void ChangeUserStatus(ICQUser *u, unsigned long s);
  bool AddUserEvent(ICQUser *, CUserEvent *);
  void RejectEvent(unsigned long, CUserEvent *);
  ICQUser *FindUserForInfoUpdate(unsigned long nUin, ICQEvent *e, const char *);

  void icqRegisterFinish();
  void icqPing();
  void icqSendVisibleList();
  void icqSendInvisibleList();
  void icqRequestSystemMsg();
  ICQEvent* icqSendThroughServer(unsigned long nUin, unsigned char format, char *_sMessage, CUserEvent* );
  void SaveUserList();

  void FailEvents(int sd, int err);
  ICQEvent *DoneServerEvent(unsigned long, EventResult);
  ICQEvent *DoneEvent(ICQEvent *e, EventResult _eResult);
  ICQEvent *DoneEvent(int _nSD, unsigned long _nSequence, EventResult _eResult);
  ICQEvent *DoneEvent(unsigned long tag, EventResult _eResult);
  ICQEvent *DoneExtendedServerEvent(const unsigned short, EventResult);
  ICQEvent *DoneExtendedEvent(ICQEvent *, EventResult);
  ICQEvent *DoneExtendedEvent(unsigned long tag, EventResult _eResult);
  void ProcessDoneEvent(ICQEvent *);
  void PushExtendedEvent(ICQEvent *);
  void PushPluginSignal(CICQSignal *);
  void PushPluginEvent(ICQEvent *);
  bool SendEvent(int nSD, CPacket &, bool);
  bool SendEvent(INetSocket *, CPacket &, bool);
  void SendEvent_Server(CPacket *packet);
  ICQEvent *SendExpectEvent_Server(unsigned long nUin, CPacket *, CUserEvent *);
  ICQEvent *SendExpectEvent_Client(ICQUser *, CPacket *, CUserEvent *);
  ICQEvent *SendExpectEvent(ICQEvent *, void *(*fcn)(void *));
  void AckTCP(CPacketTcp &, int);
  void AckTCP(CPacketTcp &, TCPSocket *);

  bool ProcessSrvPacket(CBuffer&);

  //--- Channels ---------
  bool ProcessCloseChannel(CBuffer&);
  void ProcessDataChannel(CBuffer&);

  //--- Families ---------
  void ProcessServiceFam(CBuffer&, unsigned short);
  void ProcessLocationFam(const CBuffer&, unsigned short);
  void ProcessBuddyFam(CBuffer&, unsigned short);
  void ProcessMessageFam(CBuffer&, unsigned short);
  void ProcessVariousFam(CBuffer&, unsigned short);
  void ProcessBOSFam(CBuffer&, unsigned short);
  void ProcessListFam(CBuffer &, unsigned short);
  void ProcessNewUINFam(CBuffer &, unsigned short);

  void ProcessSystemMessage(CBuffer &packet, unsigned long checkUin, unsigned short newCommand, time_t timeSent);
  void ProcessMetaCommand(CBuffer &packet, unsigned short nMetaCommand, ICQEvent *e);
  bool ProcessTcpPacket(TCPSocket *);
  bool ProcessTcpHandshake(TCPSocket *);
  void ProcessFifo(char *);

  static bool Handshake_Send(TCPSocket *, unsigned long, unsigned short, unsigned short);
  static bool Handshake_Recv(TCPSocket *, unsigned short);
  int ConnectToServer(const char* server, unsigned short port);
  int ConnectToLoginServer();
  int ConnectToUser(unsigned long);
  int ReverseConnectToUser(unsigned long nUin, unsigned long nUin,
     unsigned short nPort, unsigned short nVersion, unsigned short nFailedPort);

  void StupidChatLinkageFix();

  // Declare all our thread functions as friends
  friend void *Ping_tep(void *p);
  friend void *MonitorSockets_tep(void *p);
  friend void *ReverseConnectToUser_tep(void *p);
  friend void *ProcessRunningEvent_Client_tep(void *p);
  friend void *ProcessRunningEvent_Server_tep(void *p);
  friend void *Shutdown_tep(void *p);
  friend class ICQUser;
  friend class CSocketManager;
  friend class CChatManager;
  friend class CFileTransferManager;
  friend class COnEventManager;
	friend class CUserManager;
};

// Global pointer
extern CICQDaemon *gLicqDaemon;

// Helper functions for the daemon
bool ParseFE(char *szBuffer, char ***szSubStr, int nMaxSubStr);
unsigned long StringToStatus(char *_szStatus);
unsigned short VersionToUse(unsigned short);

// Data structure for passing information to the reverse connection thread
class CReverseConnectToUserData
{
public:
  CReverseConnectToUserData(unsigned long uin, unsigned long ip,
   unsigned short port, unsigned short version, unsigned short failedport) :
   nUin(uin), nIp(ip), nPort(port), nFailedPort(failedport),
   nVersion(version) {}

  unsigned long nUin;
  unsigned long nIp;
  unsigned short nPort;
  unsigned short nFailedPort;
  unsigned short nVersion;
};



#endif

