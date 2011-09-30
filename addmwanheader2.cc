// -basic-offset: 4 -*-
#include <click/config.h>
#include "addmwanheader2.hh"
#include <click/confparse.hh>
#include <click/error.hh>
CLICK_DECLS

AddMWanHeader2::AddMWanHeader2()
    : _elem_ccd(0)
{
}

AddMWanHeader2::~AddMWanHeader2()
{
}

int
AddMWanHeader2::configure(Vector<String> &conf, ErrorHandler *errh)
{
    Element *ccd_element = 0;
    if (cp_va_kparse(conf, this, errh,
		     "CALC_CONGESTION_DELTA", cpkM, cpElement, &ccd_element,
		     cpEnd) < 0)
	return -1;

    CalcCongestionDelta *ccd = 0;
    if (ccd_element && 
        !(ccd = (CalcCongestionDelta *)(ccd_element->cast("CalcCongestionDelta"))))
    	return errh->error("CALC_CONGESTION_DELTA must be a CalcCongestionDelta element");
    else if (ccd)
	_elem_ccd = ccd;

    return 0;
}

int
AddMWanHeader2::initialize(ErrorHandler *)
{
    return 0;
}

void
AddMWanHeader2::add_handlers()
{
}

void
AddMWanHeader2::cleanup(CleanupStage)
{
}

Packet *
AddMWanHeader2::simple_action(Packet *p)
{
    // Timestamp + 2 bytes of congestion bools
// #ifdef CLICK_ADDMWANHEADER2_DEBUG
//     click_chatter("[ADDMWANHEADER2] packet->push %d", 8+2);
// #endif
    WritablePacket *q = p->push(8+2);
    if (!q)
        return 0;

    // Timestamp
    Timestamp now = Timestamp::now();
    uint64_t now_us = now.sec() * ((uint64_t)(1000000));
    now_us += now.usec();

    uint64_t *tmp = (uint64_t*) q->data();
    // *tmp = 0;
    *tmp = now_us;

    // uint64_t mult = 1;
    // for (int i = 0; i < 64; i++) {
    //   mult = mult << 1;
    //   *tmp |= ((now_us % mult) > 0 ? 1<<i : 0);
    // }

#ifdef CLICK_ADDMWANHEADER2_DEBUG
    click_chatter("[ADDMWANHEADER2] timestamp %llu", now_us);
    // click_chatter("[ADDMWANHEADER2] tmp %x%x %x%x %x%x %x%x", q->data()[0],
    //               q->data()[1],
    //               q->data()[2],
    //               q->data()[3],
    //               q->data()[4],
    //               q->data()[5],
    //               q->data()[6],
    //               q->data()[7]);
#endif

    // TODO:Congestion bits
    unsigned short *p_cb = (unsigned short*) (q->data() + 8);
    *p_cb = _elem_ccd->get_congestion_score();

    return q;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(AddMWanHeader2)
