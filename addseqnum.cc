// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "addseqnum.hh"

AddSeqNum::AddSeqNum()
{
}

AddSeqNum::~AddSeqNum()
{
}

int
AddSeqNum::configure(Vector<String> &, ErrorHandler *)
{
  return 0;
}

int
AddSeqNum::initialize(ErrorHandler *)
{
  _seq_num = 0;

  return 0;
}

void
AddSeqNum::add_handlers()
{
  add_write_handler("reset_seq_num", static_reset_seq_num, 0);
}

void
AddSeqNum::cleanup(CleanupStage)
{
}

Packet *
AddSeqNum::simple_action(Packet *p)
{
  WritablePacket *q = p->push(4);
  if (!q)
    return 0;

  uint32_t *tmp = (uint32_t*) q->data();
  *tmp = _seq_num++;

  return q;
}

int
AddSeqNum::static_reset_seq_num(const String&, Element *e, void*, ErrorHandler*)
{
  AddSeqNum *as = (AddSeqNum *) e;

  as->_seq_num = 0;

  return 0;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(AddSeqNum)
