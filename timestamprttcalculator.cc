// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "timestamprttcalculator.hh"
#include <click/timestamp.hh>
#include <click/confparse.hh>
#include <click/error.hh>
#include "statesource.hh"
CLICK_DECLS

TimestampRTTCalculator::TimestampRTTCalculator()
    : _rtt(0.0), _id(0)
{
}

TimestampRTTCalculator::~TimestampRTTCalculator()
{
}

int
TimestampRTTCalculator::configure(Vector<String> &conf, ErrorHandler *errh)
{
    Element *ss_element = 0;
    if (cp_va_kparse(conf, this, errh,
		     "ID", cpkM, cpUnsigned, &_id,
		     "STATESOURCE", cpkM, cpElement, &ss_element,
		     cpEnd) < 0)
	return -1;

    StateSource *ss = 0;
    if (ss_element && !(ss = (StateSource *)(ss_element->cast("StateSource"))))
	return errh->error("STATESOURCE must be a StateSource element");
    else if (ss)
	ss->add_state_provider(_id, StateSource::RTT, (void*) this);

    return 0;
}

int
TimestampRTTCalculator::initialize(ErrorHandler *)
{
    _rtt = 150;
    return 0;
}

void
TimestampRTTCalculator::cleanup(CleanupStage)
{
}

void
TimestampRTTCalculator::add_handlers()
{
    add_read_handler("get_rtt", static_get_rtt, 0);
}

void
TimestampRTTCalculator::push(int, Packet *p)
{
    Timestamp now = Timestamp::now();
    uint64_t now_ms = now.sec() * ((uint64_t)1000);
    now_ms += now.msec();

    uint64_t timestamp = *((uint64_t*) p->data());
  
    int diff = now_ms - timestamp;
    _rtt = _rtt * 0.9 + diff * 0.1;

#ifdef CLICK_TIMESTAMPRTTCALCULATOR_DEBUG
    click_chatter("RECV now(%llu) timestamp(%llu) diff(%d) new_rtt(%f) ID(%u)\n",
		  now_ms, timestamp, diff, _rtt, _id);
#endif

    p->kill();
}

double
TimestampRTTCalculator::get_rtt()
{
    return _rtt;
}

String
TimestampRTTCalculator::static_get_rtt(Element *e, void *)
{
    TimestampRTTCalculator *trc = (TimestampRTTCalculator *)e;
    return String(trc->get_rtt());
}

CLICK_ENDDECLS
EXPORT_ELEMENT(TimestampRTTCalculator)
