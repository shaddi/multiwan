// -*- c-basic-offset: 4 -*-
#ifndef CLICK_TIMESTAMPRTTCALCULATOR_HH
#define CLICK_TIMESTAMPRTTCALCULATOR_HH
#include <click/element.hh>
#include <click/glue.hh>
#include "statesource.hh"
CLICK_DECLS

//#define CLICK_TIMESTAMPRTTCALCULATOR_DEBUG

/*
  =c
  TimestampRTTCalculator(ID, STATESOURCE)

  =s local

  Calculates the rtt using packets from TimestampSource

  =d

  Expects packets from TimestampSource with all headers stripped off. Takes the
  timestamp in the packet and calculates the rtt using an exponentially weighted
  moving average. Alpha = .1 (meaning 10% weight on the new rtt). When done with
  the new packet it will be deleted.

  Equation used: new_avg_rtt = new_rtt * .1 + old_avg_rtt * .9

  Handlers:
  get_rtt(read-only) - returns the current rtt weighted moving average in ms

  Keyword arguments:

  =item ID

  The 32 bit ID of this element that matches a LossRateCalculator
  element's ID. Also passed in to the StateSource so it can match
  them.

  =item STATESOURCE

  The StateSource element that this element needs to register itself with.
*/

class TimestampRTTCalculator : public Element { public:

	TimestampRTTCalculator();
    ~TimestampRTTCalculator();

    const char *class_name() const { return "TimestampRTTCalculator"; }
    const char *port_count() const { return PORTS_1_0; }
    const char *processing() const { return PUSH; }

    enum { CONFIGURE_PHASE = (StateSource::CONFIGURE_PHASE + 1) };
    int configure_phase() const { return CONFIGURE_PHASE; }
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void cleanup(CleanupStage);
    void add_handlers();

    void push(int, Packet*);
    double get_rtt();

private:

    double _rtt;
    uint32_t _id;

    static String static_get_rtt(Element *, void *);

};

CLICK_ENDDECLS
#endif
