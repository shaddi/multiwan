// -*- c-basic-offset: 4 -*-
#ifndef CLICK_PROCESSMWANHEADER_HH
#define CLICK_PROCESSMWANHEADER_HH
#include <click/element.hh>
#include <click/glue.hh>
#include "progscheduler.hh"
CLICK_DECLS

//#define CLICK_PROCESSMWANHEADER_DEBUG
#define MAX_SCH_VALUE 10
#define MIN_SCH_VALUE 1

/*
  =c

  ProcessMWanHeader(PROGSCHEDULER, PAINT_MAX)

  =s local

  Processes the MWan header.

  =d

  Processes the MWan header. Especially looks at the bandwidth value in the
  header so it can properly change the value of ProgScheduler.

  The packets must have the paint annotation set so that it can tell which line
  a packet is from. The paint value should match the output port for the
  ProgScheduler.

  Keyword arguments:

  =item PROGSCHEDULER

  The ProgScheduler this element should control.

  =item PAINT_MAX

  The max number of different paint colors there will be.
 
 */

class ProcessMWanHeader : public Element {
public:
    ProcessMWanHeader();
    ~ProcessMWanHeader();

    const char *class_name() const { return "ProcessMWanHeader"; }
    const char *port_count() const { return PORTS_1_1; }
    const char *processing() const { return AGNOSTIC; }
  
    int configure_phase() const { return ProgScheduler::CONFIGURE_PHASE + 1; }
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    Packet *simple_action(Packet*);

private:
    ProgScheduler *_elem_ps;

    unsigned int _max_paint;
    uint32_t *_bandwidths;
    int *_schedule;

    bool update_schedule(int size, uint32_t *bandwidths);

    static bool static_ary_equals(int size, int *a, int *b);
};

CLICK_ENDDECLS
#endif
