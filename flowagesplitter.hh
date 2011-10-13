// -*- c-basic-offset: 4 -*-
#ifndef CLICK_FLOWAGESPLITTER_HH
#define CLICK_FLOWAGESPLITTER_HH
#include <click/element.hh>
#include <click/glue.hh>
#include <click/hashmap.hh>
#include <click/timer.hh>
CLICK_DECLS

//#define CLICK_FLOWAGESPLITTER_DEBUG

/*
  =c

  FlowAgeSplitter()

  =s local

  Sends all packets out port 0 except a certain number of the oldest flows out
  port 1.

  =d

  This needs a AggregateIPFlow element before it (which needs a MarkIPHeader
  before it). It takes the flow ID the AggregateIPFlow element gives it and
  sends the packet out port 0 or 1 depending on when it first saw that flow.

  Call bump_flows(int n) to move the n oldest flows going out port 0 to go out
  port 1.

*/

#define FLOWAGESPLITTER_CLEANUP_INTERVAL 120 //In seconds

class FlowAgeSplitter : public Element {
public:
    FlowAgeSplitter();
    ~FlowAgeSplitter();

    const char *class_name() const { return "FlowAgeSplitter"; }
    const char *port_count() const { return "1/2"; }
    const char *processing() const { return PUSH; }
  
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    void push(int, Packet *);
    void run_timer(Timer*);
    void bump_flows(unsigned int);

private:
    struct FlowDataNode {
        uint32_t flowID;
        uint64_t orderID;
        bool delete_me;
        FlowDataNode *next;
        FlowDataNode *prev;
    };

    Timer _timer;
    FlowDataNode *_head;
    FlowDataNode *_tail;
    FlowDataNode *_curr_thresh_node;
    uint64_t _curr_thresh_order;

    HashMap<uint32_t, FlowDataNode*> _hshFIdFDNode;

    static int static_bump_flows(const String&, Element*, void*, ErrorHandler*);
};

CLICK_ENDDECLS
#endif
