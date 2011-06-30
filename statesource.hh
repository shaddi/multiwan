// -*- c-basic-offset: 4 -*-
#ifndef CLICK_STATESOURCE_HH
#define CLICK_STATESOURCE_HH
#include <click/element.hh>
#include <click/glue.hh>
#include <click/timer.hh>
#include "connectionstate.hh"
#include <map>
CLICK_DECLS

#define CLICK_STATESOURCE_DEBUG

/* 
   =c

   StateSource(INTERVAL)

   =s local

   Creates a packet about Internet connection state every INTERVAL seconds

   =d

   Creates a packet containing information about the state of each line
   and pushes it out of port 0. It only does this for lines that have
   both their TimestampRTTCalculator and LossRateCalculator elements
   registered to this element. Which is done by passing this element to
   those.

   Keyword arguments:

   =item INTERVAL

   Amount of time between sending a packet. Unit: seconds This is also
   how often it updates the max_rtt that it feeds to the
   LossRateCalculator, which isn't a good idea, but leaving it for
   now.

   =item OUTFILE

   String. Only available at user level. PrintV<> information to the
   file specified by OUTFILE instead of standard error.

*/

class StateSource : public Element { public:

    StateSource();
    ~StateSource();

    const char *class_name() const { return "StateSource"; }
    const char *port_count() const { return PORTS_0_1; }
    const char *processing() const { return PUSH; }

    enum { CONFIGURE_PHASE = CONFIGURE_PHASE_DEFAULT };
    int configure_phase() const { return CONFIGURE_PHASE; }
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void cleanup(CleanupStage);

    void run_timer(Timer *);

    enum stateprovider_t { LOSS_RATE, RTT };
    void add_state_provider(uint32_t id, stateprovider_t type, void *p);
    double get_max_rtt();

private:

    std::map<uint32_t, void*> *_sp_map_loss_rate;
    std::map<uint32_t, void*> *_sp_map_rtt;
    Timer _timer;
    Timestamp _interval;
    double _max_rtt;
    String _outfilename;
    FILE *_outfile;

    Packet *create_state_packet();    
};

CLICK_ENDDECLS
#endif
