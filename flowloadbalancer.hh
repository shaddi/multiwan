// -*- c-basic-offset: 4 -*-
#ifndef CLICK_FLOWLOADBALANCER_HH
#define CLICK_FLOWLOADBALANCER_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

#define CLICK_FLOWLOADBALANCER_DEBUG

/*
  =c

  FlowLoadBalancer

  =s local

  Load balances packets based on flow.

  =d

  Using the flow number label on each packet given by AggregateIPFlows
  it sends it out a particular port. Simply sends the packet out port
  flow_num%total_ports.
*/

class FlowLoadBalancer : public Element { public:

    FlowLoadBalancer();
  ~FlowLoadBalancer();

  const char *class_name() const { return "FlowLoadBalancer"; }
  const char *port_count() const { return "1/1-"; }
  const char *processing() const { return PUSH; }

  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void cleanup(CleanupStage);

  void push(int, Packet*);

private:

  int _port_count;
};

CLICK_ENDDECLS
#endif
