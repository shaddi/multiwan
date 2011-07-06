// -*- c-basic-offset: 4 -*-
#ifndef CLICK_PROGSCHEDULER_HH
#define CLICK_PROGSCHEDULER_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

//#define CLICK_PROGSCHEDULER_DEBUG

/*
  =c

  ProgScheduler()

  =s

  A programmable scheduler.

  =d

  A programmable schedular, it is passed an array by another element that
  specifies how many pakcets go out each port before starting over again.

  Example: If given an array [1,2,3], one packet will go out port 0, two out
  port 1, and three out port 2. It will then start at port 0 again and follow
  the same pattern until passed a new array.

  Default: Initialized with one packet out each port than start over.
*/

class ProgScheduler : public Element {
public:
    ProgScheduler();
    ~ProgScheduler();

    const char *class_name() const { return "ProgScheduler"; }
    const char *port_count() const { return "1-/="; }
    const char *processing() const { return PUSH; }

    enum { CONFIGURE_PHASE = CONFIGURE_PHASE_DEFAULT };
    int configure_phase() const { return CONFIGURE_PHASE; }
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    void push(int, Packet *);
    void set_schedule(int, const int[]);

private:

    int _total_ports;
    int *_schedule;
    int _curr_port;
    int _curr_port_count;

    static int static_set_schedule(const String&, Element*, void*, ErrorHandler*);
};

CLICK_ENDDECLS
#endif
