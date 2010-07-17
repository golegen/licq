#ifndef IFACE_H
#define IFACE_H

#include <string>
#include <map>
#include <time.h>

#include <aosd.h>
#include <aosd-text.h>
#undef Status

#include <licq/pluginsignal.h>

class Conf;

class Iface
{
public:
  Iface();
  ~Iface();

  void processSignal(Licq::PluginSignal* sig);
  void updateTextRenderData();

private:
  Aosd* aosd;
  TextRenderData trd;
  Conf* conf;
  std::map<unsigned long, time_t> ppidTimers;

  bool filterSignal(Licq::PluginSignal* sig, unsigned long ppid);

  void displayLayout(std::string& msg, bool control);
};

#endif

/* vim: set ts=2 sw=2 et : */
