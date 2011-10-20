// -*- c-basic-offset: 4 -*-
#ifndef CLICK_PROCESSMWANHEADER2_HH
#define CLICK_PROCESSMWANHEADER2_HH
#include <click/element.hh>
#include <click/glue.hh>
#include <click/timer.hh>
#include "distroswitch.hh"
#include "flowagesplitter.hh"
CLICK_DECLS

//#define CLICK_PROCESSMWANHEADER2_DEBUG

/*
  =c

  ProcessMWanHeader2(DISTROSWITCH, PAINT_MAX, UPDATE_INT, DISTRIB_TOTAL,
  DISTRIB_INC, DISTRIB_MIN, FLOWAGESPLITTER, FLOWSPLIT_NUM, FLOWSPLIT_THRESHOLD)

  =s local

  Processes the MWan header.

  =d

  Processes the MWan header. Uses the congestion bool tag to calculate the
  distribution for the packets. Then tells the DistroSwitch what to do. 

  The packets must have the paint annotation set so that it can tell which line
  a packet is from. The paint value should match the output port for the
  ProgScheduler.

  Keyword arguments:

  =item DISTROSWITCH

  The ProgScheduler this element should control.

  =item PAINT_MAX

  The max number of different paint colors there will be.

  =item UPDATE_INT

  The interval between updates of the DistroSwitch, assumed to be in milliseconds.
 
  =item DISTRIB_TOTAL

  The total to distribute between the PAINT_MAX lines.
 
  =item DISTRIB_INC

  How much is moved from line A to ling B, when line A is congested and line B
  is not or less congested then the other lines.

  =item DISTRIB_MIN

  The minimum distribution given to a line.

  =item FLOWAGESPLITTER

  Optional: If this passed in whenever things seem congested, FlowAgeSplitter is
  called to move some flows to another line class.

  =item FLOWSPLIT_NUM

  Optional: If this passed in whenever things seem congested, FlowAgeSplitter is
  called to move this many flows to another line class (Default is 1)

  =item FLOWSPLIT_THRESHOLD

  Optional: If this passed in whenever the average congested score is above this
  threshold inclusive (range: 0-16, 0 no congestion) FlowAgeSplitter will be
  called to move FLOWSPLIT_NUM flows to another line class. (Default is 4)

 */

#define MAX_CONG_SCORE 16
#define FLOWSPLIT_NUM 1
#define FLOWSPLIT_THRESHOLD 4

class ProcessMWanHeader2 : public Element {
public:
    ProcessMWanHeader2();
    ~ProcessMWanHeader2();

    const char *class_name() const { return "ProcessMWanHeader2"; }
    const char *port_count() const { return PORTS_1_1; }
    const char *processing() const { return AGNOSTIC; }
  
    int configure_phase() const { return DistroSwitch::CONFIGURE_PHASE + 1; }
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    Packet *simple_action(Packet*);
    void run_timer(Timer*);

private:
    DistroSwitch *_elem_ds;
    FlowAgeSplitter *_elem_fas;
    Timer _timer;

    unsigned int _max_paint;
    uint32_t _update_int;
    unsigned short *_cong_deltas;
    unsigned short *_cong_seq_nums;
    unsigned short *_cong_seq_nums_last_update;
    unsigned short *_cong_scores;
    uint32_t *_distrib;
    uint32_t _distrib_total;
    uint32_t _distrib_inc;
    uint32_t _distrib_min;
    unsigned int _flowsplit_num;
    unsigned short _flowsplit_threshold;
    unsigned short _c_uniform_cong;
    uint32_t _c_cong_mask;

    void update_cong_scores();
    void update_distribution(); // update_cong_scores() needs to be called first.
    void bump_flows_if_need(); // update_cong_scores() needs to be called first.

    uint32_t get_distrib_shift(int);

    static unsigned short static_count_ones(unsigned short num, uint32_t c_mask);
    static bool static_ary_equals(int size, int *a, int *b);
};

CLICK_ENDDECLS
#endif
