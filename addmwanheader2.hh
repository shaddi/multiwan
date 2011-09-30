// -*- c-basic-offset: 4 -*-
#ifndef CLICK_ADDMWANHEADER2_HH
#define CLICK_ADDMWANHEADER2_HH
#include <click/element.hh>
#include <click/glue.hh>
#include "calccongestiondelta.hh"
CLICK_DECLS

#define CLICK_ADDMWANHEADER2_DEBUG

/*
  =c

  AddMWanHeader2(CALC_CONGESTION_DELTA)

  =s local

  Adds a multiwan header to the front of the packet

  =d

  Adds a multiwan header to the front of the packet. The header is as follows:

  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                 Timestamp in microseconds                   |
  |                                                             |
  |-------------------------------------------------------------|
  |        congestion bool        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Keyword arguments:

  =item CALC_CONGESTION_DELTA

  Click element that calculates the congestion delta for the line it is on.

*/

#define CLICK_ADDMWANHEADER2_MAX_LIST_ELEM 16

class AddMWanHeader2 : public Element {
public:
    AddMWanHeader2();
    ~AddMWanHeader2();

    const char *class_name() const { return "AddMWanHeader2"; }
    const char *port_count() const { return PORTS_1_1; }
    const char *processing() const { return AGNOSTIC; }
  
    int configure_phase() const { return CalcCongestionDelta::CONFIGURE_PHASE + 1; }
    int configure(Vector<String> &conf, ErrorHandler *errh);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    Packet *simple_action(Packet*);

private:
    CalcCongestionDelta *_elem_ccd;
};

CLICK_ENDDECLS
#endif
