// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "flowloadbalancer.hh"
#include <click/packet_anno.hh>
CLICK_DECLS

FlowLoadBalancer::FlowLoadBalancer()
  : _port_count(0)
{
}

FlowLoadBalancer::~FlowLoadBalancer()
{
}

int
FlowLoadBalancer::configure(Vector<String>&, ErrorHandler*)
{
  return 0;
}

int
FlowLoadBalancer::initialize(ErrorHandler*)
{
  _port_count = noutputs();
  return 0;
}

void
FlowLoadBalancer::cleanup(CleanupStage)
{
}

void
FlowLoadBalancer::push(int, Packet *p)
{
  int flow_num = AGGREGATE_ANNO(p);

#ifdef CLICK_FLOWLOADBALANCER_DEBUG
  click_chatter("FLOW BALANCER: flow(%d) port(%d)", flow_num, flow_num%_port_count);
#endif

  output(flow_num%_port_count).push(p);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(FlowLoadBalancer)
