// -*- c-basic-offset: 4 -*-
#ifndef CLICK_LOSSRATECALCULATOR_HH
#define CLICK_LOSSRATECALCULATOR_HH
#include <click/element.hh>
#include <click/glue.hh>
#include <click/timer.hh>
#include "timestamprttcalculator.hh"
#include "statesource.hh"
CLICK_DECLS

//#define CLICK_LOSSRATECALCULATOR_DEBUG

/*
  =c
  LossRateCalculator(ID, STATESOURCE, RTTCALCULATOR)

  =s local

  analyzes packets marked by AddSeqNum for loss rate

  =d

  Calculates the loss rate of the flow passing through it. Expects the
  packets to be marked by AddSeqNum and all headers before the sequence
  number inserted by AddSeqNum have been stripped off.

  Handler
  get_loss_rate(read-only) - returns the loss rate.

  Keyword arguments:

  =item ID

  The 32 bit ID of this element that matches a TimestampRTTCalculator
  element's ID. Also passed in to the StateSource so it can match
  them.

  =item STATESOURCE

  The StateSource element that this element needs to register itself with.

  =item RTTCALCULATOR

  The RTTCalculator so it knows how long an RTT is.
*/

class LossRateCalculator : public Element {

public:
    LossRateCalculator();
    ~LossRateCalculator();

    const char *class_name() const { return "LossRateCalculator"; }
    const char *port_count() const { return PORTS_1_1; }
    const char *processing() const { return AGNOSTIC; }

    // just after TimestampRTTCalculator
    int configure_phase() const {
	int a = TimestampRTTCalculator::CONFIGURE_PHASE + 1;
	int b = StateSource::CONFIGURE_PHASE + 1;
	return a > b ? a : b;
    }
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void cleanup(CleanupStage);
    void add_handlers();

    Packet *simple_action(Packet *);
    void run_timer(Timer *);

    double get_loss_rate();

private:

    typedef struct LossInterval {
	uint32_t seq_start;
	uint32_t seq_end;
	LossInterval *next;
	LossInterval *prev;
	LossInterval(uint32_t start, uint32_t end) {
	    seq_start = start;
	    seq_end = end;
	    next = 0;
	    prev = 0;
	}
    };

    enum { NWEIGHTS = 8 };

    double _weights[NWEIGHTS];

    Timer _timer;
    TimestampRTTCalculator *_trc;
    LossInterval *_li_head;
    LossInterval *_li_tail;
    int _loss_interval_count;
    bool _update_loss_rate;
    bool _in_loss_event;
    uint32_t _seq_start;
    uint32_t _seq_curr; // Highest sequence number seen
    double _loss_rate;
    uint32_t _id;

    static String static_get_loss_rate(Element*, void*);

    double calculate_loss_rate();
    void new_loss_event(uint32_t);

};

CLICK_ENDDECLS
#endif
