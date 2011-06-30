// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "progswitch.hh"
#include <click/error.hh>
#include <click/confparse.hh>
CLICK_DECLS

/*
 */

ProgSwitch::ProgSwitch()
{
}

ProgSwitch::~ProgSwitch()
{
}

int
ProgSwitch::configure(Vector<String> &, ErrorHandler *)
{
  return 0;
}

int
ProgSwitch::initialize(ErrorHandler *)
{
  _total_ports = 0;
  _map = new int[_total_ports];
  return 0;
}

void
ProgSwitch::add_handlers()
{
  add_write_handler("set_switch_map", static_set_switch_map, 0);
}

void
ProgSwitch::cleanup(CleanupStage)
{
  delete[] _map;
}

void
ProgSwitch::push(int port, Packet *p)
{

  if (port >= _total_ports) {
    output(port).push(p);
#ifdef CLICK_PROGSWITCH_DEBUG
    click_chatter("DEFAULT: packet port %d -> %d\n", port, port);
#endif
  } else {
    output(_map[port]).push(p);
#ifdef CLICK_PROGSWITCH_DEBUG
    click_chatter("PROGRAMMED: packet port %d -> %d\n", port, _map[port]);
#endif
  }
}

void
ProgSwitch::set_switch_map(int total, const int map[])
{
  int *tmp_map = new int[total];
  for (int i = 0; i < total; i++)
    tmp_map[i] = map[i];

  int *tmp_map2 = _map;
  _total_ports = total;
  _map = tmp_map;
  delete[] tmp_map2;

#ifdef CLICK_PROGSWITCH_DEBUG
  click_chatter("NEW MAP: ");
  for (int i = 0; i < _total_ports; i++)
    click_chatter(" %d", _map[i]);
#endif
}

int
ProgSwitch::static_set_switch_map(const String &data, Element *element, void*,
			   ErrorHandler *errh)
{
  String str = data;
  Vector<String> conf;
  cp_spacevec(data, conf);

#ifdef CLICK_PROGSWITCH_DEBUG
  click_chatter("string input length %d\n", conf.size());
  for(int i = 0; i < conf.size(); i++)
    click_chatter("value %d '%s'\n", i, conf[i].c_str());
#endif

  int total = 0;
  if(!cp_integer(conf[0], &total))
    return errh->error("Total port numbers in map is not a number");

#ifdef CLICK_PROGSWITCH_DEBUG
  click_chatter("total = %d\n", total);
#endif

  int tmp_map[total];
  for (int i = 0; i < total; i++)
    tmp_map[i] = 0;

  int i = 0;
  int tmp = -1;
  while(i < total && cp_integer(conf[i+1], &tmp)) {
    tmp_map[i] = tmp;
    i++;
  }
 
  ProgSwitch *ps = (ProgSwitch *)element;
  ps->set_switch_map(total, tmp_map);
  
  return 0;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ProgSwitch)
