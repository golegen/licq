#ifndef USERHISTORY_H
#define USERHISTORY_H

#include <list.h>

class CUserEvent;

typedef list<CUserEvent *> HistoryList;
typedef list<CUserEvent *>::iterator HistoryListIter;

class CUserHistory
{
public:
  CUserHistory();
  ~CUserHistory();
  void SetFile(const char *, unsigned long);
  void Append(const char *);
  bool Load(HistoryList &);
  void Clear();
  void Save(const char *);
  const char *Description()  { return m_szDescription; }
  const char *FileName()     { return m_szFileName; }
protected:
  char *m_szFileName;
  char *m_szDescription;
};

#endif



