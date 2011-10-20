// -*- c-basic-offset: 4 -*-
#ifndef CLICK_ADDMWANHEADER3_HH
#define CLICK_ADDMWANHEADER3_HH
#include <click/element.hh>
#include <click/glue.hh>
#include "calccongestiondelta.hh"
#include "calclatencydelta.hh"
CLICK_DECLS

//#define CLICK_ADDMWANHEADER3_DEBUG

/*
  =c

  AddMWanHeader2(CALC_CONGESTION_DELTA, CALC_LATENCY_DELTA)

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
  |        congestion bitmap      |     congestion seq num      |
  |-------------------------------------------------------------|
  |        max latency delta      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Keyword arguments:

  =item CALC_CONGESTION_DELTA

  Click element that calculates the congestion delta for the line it is on.

  =item CALC_LATENCY_DELTA

  Click element that calculates the latency delta for the lines that feed into it.

*/

#define CLICK_ADDMWANHEADER3_MAX_LIST_ELEM 16

class AddMWanHeader3 : public Element {
public:
    AddMWanHeader3();
    ~AddMWanHeader3();

    const char *class_name() const { return "AddMWanHeader3"; }
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
    CalcLatencyDelta *_elem_cld;
};

CLICK_ENDDECLS
#endif
