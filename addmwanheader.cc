// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "addmwanheader.hh"
#include <click/confparse.hh>
CLICK_DECLS

AddMWanHeader::AddMWanHeader()
{
}

AddMWanHeader::~AddMWanHeader()
{
}

int
AddMWanHeader::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if (cp_va_kparse(conf, this, errh,
		     "DEFAULT_BW", cpkM, cpUnsigned, &_default_BW,
		     cpEnd) < 0)
	return -1;

    return 0;
}

int
AddMWanHeader::initialize(ErrorHandler *)
{
    _default_BW = 0;
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
    // Timestamp + BW
    WritablePacket *q = p->push(8+4);
    if (!q)
        return 0;

    // Timestamp
    Timestamp now = Timestamp::now();
    uint64_t now_us = now.sec() * ((uint64_t)(1000000));
    now_us += now.usec();

    uint64_t *tmp = (uint64_t*) q->data();
    *tmp = now_us;

    // Bandwidth
    uint32_t *p_bw = (uint32_t*) (q->data() + 8);
    *p_bw = get_bandwidth();
    return q;
}

uint32_t
AddMWanHeader::get_bandwidth()
{
    return _default_BW;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(AddMWanHeader)
