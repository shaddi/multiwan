// -*- c-basic-offset: 4 -*-
#ifndef CLICK_ADDSEQNUM_HH
#define CLICK_ADDSEQNUM_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

#define CLICK_ADDSEQNUM_DEBUG

/*
=c

AddSeqNum()

=s local

Adds a 32 bit sequence number infront of the packet

=d

Adds 32 bits to the very front of the packet, this means a header(s) needs to be
put infront to send it. The sequence number is unsigned and starts at 0 and
increments with each new packet.

Handler
reset_seq_num() - will reset the sequence number to 0.

 */

class AddSeqNum : public Element {
public:
  AddSeqNum();
  ~AddSeqNum();

  const char *class_name() const { return "AddSeqNum"; }
  const char *port_count() const { return PORTS_1_1; }
  const char *processing() const { return AGNOSTIC; }
  
  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void add_handlers();
  void cleanup(CleanupStage);

  Packet *simple_action(Packet*);

private:

  uint32_t _seq_num;

  static int static_reset_seq_num(const String&, Element*, void*, ErrorHandler*);
};

CLICK_ENDDECLS
#endif
