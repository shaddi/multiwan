// -*- c-basic-offset: 4 -*-
#ifndef CLICK_CALCLATENCYDELTA_HH
#define CLICK_CALCLATENCYDELTA_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

#define CLICK_CALCLATENCYDELTA_DEBUG
#define LATENCY_THRESH 5000000 //Max latency of a line. 5s is enough I hope.

/*
  =c

  CalcLatencyDelta(OFFSET, PAINT_MAX)

  =s local

  Calculates latency deltas between different packet paints.

  =d

  Using the timestamp found in the packet at OFFSET as the timestamp source and
  the timestamp annotation as the dest timestamp it calculates the deltas between
  the different packet paints. The timestamp in the packet (src timestamp) is
  assumed to be a 64 bit timestamp in microseconds.

  Keyword arguments:

  =item OFFSET

  The offset into the packet where the 64 bit timestamp in microseconds is.

  =item PAINT_MAX

  The max number of different paint colors there will be.
 
*/

class CalcLatencyDelta : public Element {
public:
    CalcLatencyDelta();
    ~CalcLatencyDelta();

    const char *class_name() const { return "CalcLatencyDelta"; }
    const char *port_count() const { return PORTS_1_1; }
    const char *processing() const { return AGNOSTIC; }
  
    enum { CONFIGURE_PHASE = CONFIGURE_PHASE_DEFAULT };
    int configure_phase() const { return CONFIGURE_PHASE; }
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    Packet *simple_action(Packet*);
    int** get_deltas(); // Make sure to delete/free the data returned
    int get_max_delta();

private:
    int **_deltas;
    uint64_t *_last_latency;

    unsigned int _max_paint;
    unsigned int _offset;
};

CLICK_ENDDECLS
#endif
