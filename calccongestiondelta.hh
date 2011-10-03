// -*- c-basic-offset: 4 -*-
#ifndef CLICK_CALCCONGESTIONDELTA_HH
#define CLICK_CALCCONGESTIONDELTA_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

//#define CLICK_CALCCONGESTIONDELTA_DEBUG

/*
  =c

  CalcCongestionDelta(OFFSET)

  =s local

  Calculates the congestion on a link.

  =d

  Calculates the congestion on a link. Looks at the timestamp difference between
  the current packet and the previous one. Compares the difference between the
  timestamps in the header (X) and the timestamps in the packet's meta data
  (Y). If Y > X the link is considered congested. Keep a record of the past 16
  deltas.

  Meant to be used with AddMWanHeader2. Must have a SetTimestamp before this
  element.

  Keyword arguments:

  =item OFFSET

  The offset into the packet where the 64 bit timestamp in microseconds is.
*/

#define CLICK_CALCCONGESTIONDELTA_MAX_DELTAS 16
//#define CLICK_CALCCONGESTIONDELTA_SENSITIVITY 7000
#define CLICK_CALCCONGESTIONDELTA_SENSITIVITY 100


class CalcCongestionDelta : public Element {
public:
    CalcCongestionDelta();
    ~CalcCongestionDelta();

    const char *class_name() const { return "CalcCongestionDelta"; }
    const char *port_count() const { return PORTS_1_1; }
    const char *processing() const { return AGNOSTIC; }
  
    enum { CONFIGURE_PHASE = CONFIGURE_PHASE_DEFAULT };
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    Packet *simple_action(Packet*);

    unsigned short get_congestion_score();

private:
    struct CongestionDeltaElem {
        uint32_t my_delta;
        uint32_t o_delta;
        bool congested;
        CongestionDeltaElem *next;
        CongestionDeltaElem *prev;
    };

    CongestionDeltaElem *_listHead;
    CongestionDeltaElem *_listTail;

    uint64_t _prev_my_timestamp;
    uint64_t _prev_o_timestamp;

    unsigned int _offset;

    static String static_get_congestion_score(Element*, void*);
};

CLICK_ENDDECLS
#endif
