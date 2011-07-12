// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "addmwanheader.hh"
#include <click/confparse.hh>
CLICK_DECLS

AddMWanHeader::AddMWanHeader()
{
    _default_BW = 0;
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

#ifdef CLICK_ADDMWANHEADER_DEBUG
    click_chatter("ADDMWANHEADER: Default banwdith is %d", _default_BW);
#endif

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
    // Timestamp + BW
#ifdef CLICK_ADDMWANHEADER_DEBUG
    click_chatter("ADDMWANHEADER: packet->push %d", 8+4);
#endif
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
#ifdef CLICK_ADDMWANHEADER_DEBUG
    click_chatter("ADDMWANHEADER: Before p_bw = %d", *p_bw);
#endif
    uint32_t bw_tmp = get_bandwidth();
#ifdef CLICK_ADDMWANHEADER_DEBUG
    click_chatter("ADDMWANHEADER: bw_tmp = %d", bw_tmp);
#endif
    *p_bw = bw_tmp;
#ifdef CLICK_ADDMWANHEADER_DEBUG
    click_chatter("ADDMWANHEADER: After p_bw = %d", *p_bw);
#endif
    return q;
}

uint32_t
AddMWanHeader::get_bandwidth()
{
    return _default_BW;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(AddMWanHeader)
