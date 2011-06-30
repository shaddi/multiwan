// -*- c-basic-offset: 4 -*-
#ifndef CLICK_TCPREORDERER_HH
#define CLICK_TCPREORDERER_HH
#include <click/element.hh>
#include <click/glue.hh>
#include <clicknet/tcp.h>
#include <click/timer.hh>
#include <click/timestamp.hh>
#include "statesource.hh"
CLICK_DECLS

//#define CLICK_TCPREORDERER_DEBUG 

/*
=c

TCPReorderer(STATESOURCE)

=s tcp

Reorders tcp packets based on their sequence number.

=d

Reorders tcp packets based on their sequence number. Expects TCP
packets with aggregate annotations set as if by AggregateIPFlows. So
must be downstream from one.

Keyword arguments:

=item STATESOURCE

The StateSource so it can get the max RTT.

 */

class TCPReorderer : public Element {
public:
    TCPReorderer();
    ~TCPReorderer();

    const char *class_name() const { return "TCPReorderer"; }
    const char *port_count() const { return PORTS_1_1; }
    const char *processing() const { return PUSH; }

    int configure_phase() const { return StateSource::CONFIGURE_PHASE + 1; }
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    void push(int, Packet *);
    void run_timer(Timer *);

    //void set_rtt(int);

private:
 
    typedef struct FlowNode {
	uint32_t flow_num;
	tcp_seq_t seq_num; // Next expected sequence number
	Timestamp::seconds_type timestamp; // Last time node changed
	FlowNode *next;
	Packet *pkt; // doubly linked list lowest to highest seq num,
		     // head->prev = 0 and tail->next = 0
	Packet *tail; // tail of the pkt list
    };

    enum { REAP_TIMEOUT = 30 }; // seconds

    enum { CLEAN_MAP_SANITY = 4, NMAP = 256 };

    FlowNode *_map[NMAP];
    Timer _timer;
    int _sanity_count;
    StateSource *_ss;

    void clean_map(FlowNode **map, const Timestamp now, double max_rtt);
    void add_flow_node(Packet *);
    FlowNode *get_flow_node(Packet *);
    void insert_pkt_into_flow_node(FlowNode *, Packet *);
    Packet *remove_first_pkt(FlowNode *);
    uint32_t get_max_rtt();

    //    static int static_set_rtt(const String&, Element*, void*, ErrorHandler*);
    static uint32_t static_get_seq_num(Packet *);
    static uint32_t static_get_data_length(Packet *);
};

CLICK_ENDDECLS
#endif
