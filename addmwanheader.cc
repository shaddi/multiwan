// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "addmwanheader.hh"

AddMWanHeader::AddMWanHeader()
{
}

AddMWanHeader::~AddMWanHeader()
{
}

int
AddMWanHeader::configure(Vector<String> &, ErrorHandler *)
{
  return 0;
}

int
AddMWanHeader::initialize(ErrorHandler *)
{
  return 0;
}

void
AddMWanHeader::add_handlers()
{
}

void
AddMWanHeader::cleanup(CleanupStage)
{
}

Packet *
AddMWanHeader::simple_action(Packet *p)
{
  WritablePacket *q = p->push(8);
  if (!q)
    return 0;

  Timestamp now = Timestamp::now();
  uint64_t now_us = now.sec() * ((uint64_t)(1000000));
  now_us += now.usec();

  uint64_t *tmp = (uint64_t*) q->data();
  *tmp = now_us;

  return q;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(AddMWanHeader)
