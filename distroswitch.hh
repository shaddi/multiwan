// -*- c-basic-offset: 4 -*-
#ifndef CLICK_DISTROSWITCH_HH
#define CLICK_DISTROSWITCH_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

//#define CLICK_DISTROSWITCH_DEBUG

/*
  =c

  DistroSwitch()

  =s local

  Load balance switch based on distribution. Default is equal distribution
  between all the output ports.

  =d

  Sends packets out it's ports based on a distribution given to it.

*/

class DistroSwitch : public Element {
public:
  DistroSwitch();
  ~DistroSwitch();

  const char *class_name() const { return "DistroSwitch"; }
  const char *port_count() const { return "1/1-"; }
  const char *processing() const { return PUSH; }
  
  enum { CONFIGURE_PHASE = CONFIGURE_PHASE_DEFAULT };
  int configure(Vector<String> &conf, ErrorHandler *errh);
  int initialize(ErrorHandler *);
  void add_handlers();
  void cleanup(CleanupStage);

  void push(int, Packet *);
  void set_distribution(unsigned int, const uint32_t[]);

  uint32_t get_pkt_count(int);
  void reset_pkt_count(int);

private:
  uint32_t *_distrib;
  uint32_t *_cpkt_line;
  int _total_ports;
  int _distrib_sum;

  static int static_set_distribution(const String&, Element*, void*, ErrorHandler*);
};

CLICK_ENDDECLS
#endif
