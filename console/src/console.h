#ifndef LICQCON_H
#define LICQCON_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "icqd.h"
#include "log.h"
#include "window.h"
#include "user.h"

#define MAX_CON 8
#define MAX_CMD_HISTORY 100
#define SCROLLBACK_BUFFER 20
const unsigned short USER_WIN_WIDTH = 30;
const char L_CONSOLExSTR[] = "[CON] ";
const char CANCEL_KEY = 'C';

struct SColorMap
{
  char szName[16];
  int nColor;
  int nAttr;
};


class CLicqConsole
{
public:
  CLicqConsole(int, char **);
  ~CLicqConsole(void);
  int Run(CICQDaemon *);
  void Shutdown(void);
protected:
  int m_nPipe;
  bool m_bExit;

  // Set'able variables
  bool m_bShowOffline, m_bShowDividers;
  const struct SColorMap *m_cColorOnline, *m_cColorOffline,
                   *m_cColorAway, *m_cColorGroupList, *m_cColorNew,
                   *m_cColorQuery, *m_cColorInfo, *m_cColorError;
  char m_szOnlineFormat[128];
  char m_szAwayFormat[128];
  char m_szOfflineFormat[128];

  unsigned short m_nCurrentGroup, m_nCon;
  GroupType m_nGroupType;
  list<char *> m_lCmdHistory;
  list<char *>::iterator m_lCmdHistoryIter;

  CICQDaemon *licqDaemon;
  CWindow *winMain, *winStatus, *winPrompt, *winLog, *winCon[MAX_CON + 1],
          *winConStatus, *winUsers, *winBar;
  CPluginLog *log;

public:
  void ProcessPipe(void);
  void ProcessSignal(CICQSignal *);
  void ProcessEvent(ICQEvent *);
  void ProcessDoneEvent(CWindow *win, ICQEvent *e);
  void ProcessStdin(void);
  void ProcessLog(void);
  char *CurrentGroupName(void);
  void SwitchToCon(unsigned short nCon);

  void InputCommand(int cIn);
  void InputLogWindow(int cIn);
  void InputMessage(int cIn);
  void InputUrl(int cIn);
  void InputRemove(int cIn);
  void InputAutoResponse(int cIn);
  char *Input_MultiLine(char *sz, unsigned short &n, int cIn);
  char *Input_Line(char *sz, unsigned short &n, int cIn);

  void PrintPrompt(void);
  void PrintStatus(void);
  void PrintBadInput(const char *);
  void PrintGroups(void);
  void PrintBoxTop(const char *_szTitle, short _nColor, short _nLength);
  void PrintBoxBottom(short _nLength);
  void PrintBoxRight(short _nLength);
  void PrintBoxLeft(void);
  void PrintVariable(unsigned short);
  void PrintUsers(void);
  void PrintHelp(void);
  void PrintHistory(HistoryList &, unsigned short, unsigned short, const char *);

  void MenuHelp(char *);
  void MenuContactList(char *);
  void MenuGroup(char *);
  void MenuQuit(char *);
  void MenuUser(char *);
  void MenuOwner(char *);
  void MenuStatus(char *);
  void MenuPlugins(char *);
  void MenuSet(char *);
  void MenuLast(char *);

  void TabUser(char *, struct STabCompletion &);
  void TabOwner(char *, struct STabCompletion &);
  void TabCommand(char *, struct STabCompletion &);
  void TabStatus(char *, struct STabCompletion &);
  void TabSet(char *, struct STabCompletion &);
  void TabLast(char *, struct STabCompletion &);

  void UserCommand_Info(unsigned long nUin, char *);
  void UserCommand_Msg(unsigned long nUin, char *);
  void UserCommand_View(unsigned long nUin, char *);
  void UserCommand_Url(unsigned long nUin, char *);
  void UserCommand_History(unsigned long nUin, char *);
  void UserCommand_Remove(unsigned long nUin, char *);
  void UserCommand_FetchAutoResponse(unsigned long nUin, char *);
  void UserCommand_SetAutoResponse(unsigned long nUin, char *);

  void Beep(void) { printf("\a"); fflush(stdout); }
};


struct STabCompletion
{
  vector <char *> vszPartialMatch;
  char szPartialMatch[32];
};

struct SCommand
{
  char *szName;
  void (CLicqConsole::*fProcessCommand)(char *);
  void (CLicqConsole::*fProcessTab)(char *, struct STabCompletion &);
  char *szHelp;
};
extern const unsigned short NUM_COMMANDS;
extern const struct SCommand aCommands[];

struct SStatus
{
  char szName[16];
  unsigned short nId;
};
extern const unsigned short NUM_STATUS;
extern const struct SStatus aStatus[];

struct SUserCommand
{
  char szName[16];
  void (CLicqConsole::*fProcessCommand)(unsigned long, char *);
};
extern const unsigned short NUM_USER_COMMANDS;
extern const struct SUserCommand aUserCommands[];

struct SOwnerCommand
{
  char szName[16];
  void (CLicqConsole::*fProcessCommand)(unsigned long, char *);
};
extern const unsigned short NUM_OWNER_COMMANDS;
extern const struct SOwnerCommand aOwnerCommands[];

typedef enum { INT, BOOL, STRING, COLOR } VarType;
struct SVariable
{
  char szName[32];
  VarType nType;
  void *pData;
};

extern const unsigned short NUM_COLORMAPS;
extern const struct SColorMap aColorMaps[];

extern const unsigned short NUM_VARIABLES;
extern struct SVariable aVariables[];

#define STRIP(x) while(isspace(*(x)) && *(x) != '\0') (x)++;

#endif
