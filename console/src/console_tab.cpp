#include "console.h"

#include <ctype.h>


int StrMatchLen(const char *_sz1, const char *_sz2, unsigned short _nStartPos)
{
  int n = _nStartPos;
  while (_sz1[n] != '\0' && _sz2[n] != '\0'
         && tolower(_sz1[n]) == tolower(_sz2[n]))
    n++;
  return n;
}



/*---------------------------------------------------------------------------
 * CLicqConsole::TabCommand
 *-------------------------------------------------------------------------*/
void CLicqConsole::TabCommand(char *_szPartialMatch,
                              struct STabCompletion &_sTabCompletion)
{
  char szMatch[32] = "";
  unsigned short nLen = strlen(_szPartialMatch);
  for (unsigned short i = 0; i < NUM_COMMANDS; i++)
  {
    char szTempCmd[20];
    snprintf(szTempCmd, 20, "%c%s", m_szCommandChar[0], aCommands[i].szName);
    if (strncasecmp(_szPartialMatch, szTempCmd, nLen) == 0)
    {
      if (szMatch[0] == '\0')
        strcpy(szMatch, szTempCmd);
      else
        szMatch[StrMatchLen(szMatch, szTempCmd, nLen)] = '\0';
      _sTabCompletion.vszPartialMatch.push_back(strdup(szTempCmd));
    }
  }
  if (nLen == 0)
    _sTabCompletion.szPartialMatch[0] = '\0';
  else
    strcpy(_sTabCompletion.szPartialMatch, szMatch);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::TabUser
 *-------------------------------------------------------------------------*/
void CLicqConsole::TabUser(char *_szPartialMatch,
                           struct STabCompletion &_sTabCompletion)
{
  char szMatch[32] = "";
  unsigned short nLen;
/*  char *szSubCmd = NULL;

  if (_szPartialMatch[0] == '"')
  {
    _szPartialMatch++;
    szSubCmd = strchr(_szPartialMatch, '"');
    if (szSubCmd != NULL) szSubCmd++;
  }
  else
  {
    szSubCmd = strchr(_szPartialMatch, ' ');
  }

  if (szSubCmd == NULL)*/
  {
    nLen = strlen(_szPartialMatch);
    FOR_EACH_USER_START(LOCK_R)
    {
      if (strncasecmp(_szPartialMatch, pUser->GetAlias(), nLen) == 0)
      {
        if (szMatch[0] == '\0')
          strcpy(szMatch, pUser->GetAlias());
        else
          szMatch[StrMatchLen(szMatch, pUser->GetAlias(), nLen)] = '\0';
        _sTabCompletion.vszPartialMatch.push_back(strdup(pUser->GetAlias()));
      }
    }
    FOR_EACH_USER_END

    if (nLen == 0)
      _sTabCompletion.szPartialMatch[0] = '\0';
    else
      strcpy(_sTabCompletion.szPartialMatch, szMatch);
  }
/*  else // Sub command time
  {
    // Remove any leading spaces
    while(isspace(*szSubCmd) && szSubCmd[0] != '\0') szSubCmd++;
    nLen = strlen(szSubCmd);
    for (unsigned short i = 0; i < NUM_USER_COMMANDS; i++)
    {
      if (strncasecmp(szSubCmd, aUserCommands[i].szName, nLen) == 0)
      {
        if (szMatch[0] == '\0')
          strcpy(szMatch, aUserCommands[i].szName);
        else
          szMatch[StrMatchLen(szMatch, aUserCommands[i].szName, nLen)] = '\0';
        _sTabCompletion.vszPartialMatch.push_back(strdup(aUserCommands[i].szName));
      }
    }
    if (nLen == 0)
      _sTabCompletion.szPartialMatch[0] = '\0';
    else
      sprintf(_sTabCompletion.szPartialMatch, "%s%s", _szPartialMatch, &szMatch[nLen]);
  }*/
}




/*---------------------------------------------------------------------------
 * CLicqConsole::TabStatus
 *-------------------------------------------------------------------------*/
void CLicqConsole::TabStatus(char *_szPartialMatch,
                             struct STabCompletion &_sTabCompletion)
{
  char szMatch[32] = "";
  unsigned short nLen = strlen(_szPartialMatch);
  for (unsigned short i = 0; i < NUM_STATUS; i++)
  {
    if (strncasecmp(_szPartialMatch, aStatus[i].szName, nLen) == 0)
    {
      if (szMatch[0] == '\0')
        strcpy(szMatch, aStatus[i].szName);
      else
        szMatch[StrMatchLen(szMatch, aStatus[i].szName, nLen)] = '\0';
      _sTabCompletion.vszPartialMatch.push_back(strdup(aStatus[i].szName));
    }
  }
  if (nLen == 0)
    _sTabCompletion.szPartialMatch[0] = '\0';
  else
    strcpy(_sTabCompletion.szPartialMatch, szMatch);
}


/*---------------------------------------------------------------------------
 * CLicqConsole::TabSet
 *-------------------------------------------------------------------------*/
void CLicqConsole::TabSet(char *_szPartialMatch,
                          struct STabCompletion &_sTabCompletion)
{
  char szMatch[32] = "";
  unsigned short nLen = strlen(_szPartialMatch);
  for (unsigned short i = 0; i < NUM_VARIABLES; i++)
  {
    if (strncasecmp(_szPartialMatch, aVariables[i].szName, nLen) == 0)
    {
      if (szMatch[0] == '\0')
        strcpy(szMatch, aVariables[i].szName);
      else
        szMatch[StrMatchLen(szMatch, aVariables[i].szName, nLen)] = '\0';
      _sTabCompletion.vszPartialMatch.push_back(strdup(aVariables[i].szName));
    }
  }
  if (nLen == 0)
    _sTabCompletion.szPartialMatch[0] = '\0';
  else
    strcpy(_sTabCompletion.szPartialMatch, szMatch);
}

