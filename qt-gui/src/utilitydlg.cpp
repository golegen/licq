#include <stdio.h>

#include "utilitydlg.h"
#include "icqd.h"
#include "user.h"


CUtilityDlg::CUtilityDlg(CUtility *u, unsigned long _nUin, CICQDaemon *_server)
  : QWidget(NULL, NULL)
{
  m_nUin = _nUin;
  m_xUtility = u;
  server = _server;
  fsCommand = NULL;
  m_bIntWin = false;

  m_xUtility->SetFields(m_nUin);

  char sz[64];
  sprintf(sz, "Licq Utility: %s", m_xUtility->Name());
  setCaption(sz);

  nfoUtility = new CInfoField(10, 10, 60, 5, width() - 85, "Command:", true, this);
  nfoUtility->setData(m_xUtility->FullCommand());
  nfoWinType = new CInfoField(10, 35, 60, 5, width() - 85, "Window:", true, this);
  switch (m_xUtility->WinType())
  {
  case UtilityWinGui: nfoWinType->setData("GUI"); break;
  case UtilityWinTerm: nfoWinType->setData("Terminal"); break;
  case UtilityWinLicq: nfoWinType->setData("Internal"); break;
  }
  nfoDesc = new CInfoField(10, 60, 60, 5, width() - 80, "Description:", true, this);
  nfoDesc->setData(m_xUtility->Description());
  chkEditFinal = new QCheckBox(tr("Edit final command"), this);
  boxFields = new QGroupBox("User Fields", this);
  mleCommand = new QMultiLineEdit(boxFields);
  mleCommand->setReadOnly(true);
  mleCommand->hide();
  for (unsigned short i = 0; i < m_xUtility->NumUserFields(); i++)
  {
    sprintf(sz, "%s (%%%d):", m_xUtility->UserField(i)->Title(), i + 1);
    lblFields.push_back(new QLabel(sz, boxFields));
    QLineEdit *edt = new QLineEdit(boxFields);
    edt->setText(m_xUtility->UserField(i)->FullDefault());
    edtFields.push_back(edt);
  }
  if (m_xUtility->NumUserFields() == 0) boxFields->hide();
  btnRun = new QPushButton("Run", this);
  btnCancel = new QPushButton("Cancel", this);

  connect(btnRun, SIGNAL(clicked()), SLOT(slot_run()));
  connect(btnCancel, SIGNAL(clicked()), SLOT(slot_cancel()));

  setGeometry(100, 100, 400, 160 + m_xUtility->NumUserFields() * 25
                             + (m_xUtility->NumUserFields() == 0 ? 0 : 30));
  show();
}


CUtilityDlg::~CUtilityDlg(void)
{
}


void CUtilityDlg::slot_cancel()
{
  if (m_bIntWin)
  {
    if (fsCommand != NULL)
    {
      mleCommand->append("EOF");
      m_bIntWin = false;
      disconnect(snCommand, SIGNAL(activated(int)), this, SLOT(slot_command()));
      fclose(fsCommand);
      fsCommand = NULL;
    }
    nfoUtility->setTitle(tr("Done:"));
    btnCancel->setText(tr("Close"));
    m_bIntWin = false;
  }
  else
    hide();
}

void CUtilityDlg::hide()
{
   QWidget::hide();
   if (fsCommand != NULL)
     fclose(fsCommand);
   delete this;
}


void CUtilityDlg::resizeEvent(QResizeEvent *e)
{
  nfoUtility->setGeometry(10, 10, 80, 5, width() - 105);
  nfoWinType->setGeometry(10, 35, 80, 5, width() - 105);
  nfoDesc->setGeometry(10, 60, 80, 5, width() - 105);
  boxFields->setGeometry(10, 90, width() - 20, height() - 170);
  mleCommand->setGeometry(10, 20, boxFields->width() - 20, boxFields->height() - 30);
  for (unsigned short i = 0; i < m_xUtility->NumUserFields(); i++)
  {
    lblFields[i]->setGeometry(10, (i+1) * 25 - 5, 140, 20);
    edtFields[i]->setGeometry(150, (i+1) * 25 - 5, boxFields->width() - 160, 20);
  }
  chkEditFinal->setGeometry(10, height() - 75, width() - 20, 30);
  btnRun->setGeometry(width() / 2 - 100, height() - 40, 80, 30);
  btnCancel->setGeometry(width() / 2 + 20, height() - 40, 80, 30);
}

void CUtilityDlg::slot_run()
{
  // Use the readonly state of the nfoUtility as a flag for whether or
  // not we are editing it
  if (nfoUtility->ReadOnly())
  {
    // Set the user fields
    vector<const char *> vszFields(m_xUtility->NumUserFields());
    vector<QLineEdit *>::iterator iter;
    unsigned short i = 0;
    for (iter = edtFields.begin(); iter != edtFields.end(); iter++)
    {
      vszFields[i++] = (*iter)->text();
    }
    m_xUtility->SetUserFields(vszFields);
    nfoUtility->setData(m_xUtility->FullCommand());
    if (chkEditFinal->isChecked())
    {
      nfoUtility->setTitle(tr("Edit:"));
      nfoUtility->SetReadOnly(false);
      chkEditFinal->setEnabled(false);
      return;
    }
  }

  nfoUtility->SetReadOnly(true);
  nfoUtility->setTitle(tr("Running:"));

  // Run the command
  int nSystemResult = 0;
  QString cmd = nfoUtility->text();
  switch(m_xUtility->WinType())
  {
  case UtilityWinGui:
  {
    m_xUtility->SetBackgroundTask();
    nSystemResult = system(cmd);
    break;
  }
  case UtilityWinTerm:
  {
    char* szCmd = new char[cmd.length() + strlen(server->Terminal()) + 4];
    sprintf(szCmd, "%s %s &", server->Terminal(), (const char *)cmd);
    nSystemResult = system(szCmd);
    delete szCmd;
    break;
  }
  case UtilityWinLicq:
  {
    for (unsigned short i = 0; i < m_xUtility->NumUserFields(); i++)
    {
      lblFields[i]->hide();
      edtFields[i]->hide();
    }
    boxFields->setTitle("Command Window");
    boxFields->show();
    mleCommand->show();
    resize(width(), 300);
    fsCommand = popen(cmd, "r");
    if (fsCommand != NULL)
    {
      snCommand = new QSocketNotifier(fileno(fsCommand), QSocketNotifier::Read, this);
      connect(snCommand, SIGNAL(activated(int)), SLOT(slot_command()));
      nSystemResult = 0;
      m_bIntWin = true;
    }
    else
      nSystemResult = -1;
    break;
  }
  } // switch

  if (nSystemResult == -1)
  {
    nfoUtility->setTitle(tr("Failed:"));
    m_xUtility->SetFields(m_nUin);
  }
  else
  {
    btnRun->setEnabled(false);
    btnCancel->setText(tr("Done"));
  }
}

void CUtilityDlg::slot_command()
{
  char buf[1024];
  if (fgets(buf, 1024, fsCommand) == NULL)
  {
    mleCommand->append("EOF");
    if (fsCommand != NULL)
    {
      fclose(fsCommand);
      fsCommand = NULL;
    }
    m_bIntWin = false;
    btnCancel->setText("Close");
    disconnect(snCommand, SIGNAL(activated(int)), this, SLOT(slot_command()));
  }
  else
  {
    if (buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = '\0';
    mleCommand->append(buf);
  }
}

#include "moc/moc_utilitydlg.h"
