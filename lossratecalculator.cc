// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "lossratecalculator.hh"
#include <click/error.hh>
#include <click/confparse.hh>
CLICK_DECLS

LossRateCalculator::LossRateCalculator()
    : _timer(this), _trc(0), _li_head(0), _li_tail(0), _loss_interval_count(0),
      _update_loss_rate(true), _in_loss_event(false), _seq_start(0), _seq_curr(0),
      _loss_rate(0.0), _id(0)
{
    for (int i = 0; i < NWEIGHTS/2; i++)
	_weights[i] = 1.0;

    for (int i = NWEIGHTS/2; i < NWEIGHTS; i++)
	_weights[i] = 1 - ((i+1) - NWEIGHTS/2)/(NWEIGHTS/2 + 1);
}

LossRateCalculator::~LossRateCalculator()
{
}

int 
LossRateCalculator::configure(Vector<String> &conf, ErrorHandler *errh)
{
    Element *ss_element = 0;
    Element *trc_element = 0;
    if (cp_va_kparse(conf, this, errh,
		     "ID", cpkM, cpUnsigned, &_id,
		     "STATESOURCE", cpkM, cpElement, &ss_element,
		     "RTTCALCULATOR", cpkM, cpElement, &trc_element,
		     cpEnd) < 0)
	return -1;

    TimestampRTTCalculator *trc = 0;
    if (trc_element && !(trc = (TimestampRTTCalculator *)(trc_element->cast("TimestampRTTCalculator"))))
	return errh->error("RTTCALCULATOR incorrect.");
    else if (trc)
	_trc = trc;

    StateSource *ss = 0;
    if (ss_element && !(ss = (StateSource *)(ss_element->cast("StateSource"))))
	return errh->error("STATESOURCE must be a StateSource element");
    else if (ss)
	ss->add_state_provider(_id, StateSource::LOSS_RATE, (void*) this);

    return 0;
}

int
LossRateCalculator::initialize(ErrorHandler*)
{
    _timer.initialize(this);
    return 0;
}

void
LossRateCalculator::cleanup(CleanupStage)
{
    LossInterval *li = _li_head;
    while (li) {
	LossInterval *tmp = li->next;
	delete li;
	li = tmp;
    }
}

void
LossRateCalculator::add_handlers()
{
    add_read_handler("get_loss_rate", static_get_loss_rate, 0);
}

Packet *
LossRateCalculator::simple_action(Packet *p)
{
    uint32_t seq_num = *((uint32_t *) p->data());

    // Warning: This does not take into account sequence number wrapping around
    if (seq_num <= (_seq_curr+1)) {
	_seq_curr = seq_num == (_seq_curr+1) ? seq_num : _seq_curr;
    } else if (_in_loss_event) { // loss event, but still within rtt of last loss
	_seq_curr = _seq_start = seq_num;
    } else { // new loss event
	new_loss_event(seq_num);
    }

    return p;
}

void
LossRateCalculator::run_timer(Timer *timer)
{
    assert(&_timer == timer);

    _in_loss_event = false;
}

double
LossRateCalculator::get_loss_rate()
{
    return calculate_loss_rate();
}

String
LossRateCalculator::static_get_loss_rate(Element *e, void*)
{
    LossRateCalculator *lrc = (LossRateCalculator *) e;
    return String(lrc->get_loss_rate());
}

double
LossRateCalculator::calculate_loss_rate()
{
    double weight_total1 = 0.0;
    double weight_total2 = 0.0;
    double sum1 = 0.0;
    double sum2 = 0.0;
    LossInterval *curr = _li_head;

    sum2 = (_seq_curr - _seq_start) * _weights[0];
    weight_total2 = _weights[0];

    for (int i = 0; i < NWEIGHTS && curr; i++, curr = curr->next) {
	sum1 += (curr->seq_end - curr->seq_start) * _weights[i];
	weight_total1 += _weights[i];
        if (i < NWEIGHTS - 1) {
            sum2 += (curr->seq_end - curr->seq_start) * _weights[i+1];
            weight_total2 += _weights[i+1];
        }
    }

    double loss_interval1 = weight_total1 ? sum1/weight_total1 : 0;
    double loss_interval2 = weight_total2 ? sum2/weight_total2 : 0;

    if (loss_interval1 == 0 && loss_interval2 == 0)
        _loss_rate = 0;
    else if (loss_interval1 == 0)
        _loss_rate = loss_interval2 ? 1/loss_interval2 : 0;
    else if (loss_interval2 == 0)
        _loss_rate = loss_interval1 ? 1/loss_interval1 : 0;
    else
        _loss_rate = loss_interval1 > loss_interval2 ? 1/loss_interval1 : 1/loss_interval2;

#ifdef CLICK_LOSSRATECALCULATOR_DEBUG
    click_chatter("ID (%d) SUM1 (%f) WEIGHT1 (%f) LOSS_INTERVAL1 (%f) LOSS_RATE (%f)", _id, sum1, weight_total1, loss_interval1, _loss_rate);
    click_chatter("ID (%d) SUM1 (%f) WEIGHT2 (%f) LOSS_INTERVAL2 (%f)", _id, sum2, weight_total2, loss_interval2);
    if (_loss_rate >= 1.0)
	click_chatter("!!!!!!!Loss rate is (%f)!!!!!!", _loss_rate);
#endif

    _update_loss_rate = false;

    return _loss_rate;
}

void
LossRateCalculator::new_loss_event(uint32_t seq_num)
{
    LossInterval *newLI = new LossInterval(_seq_start, _seq_curr);

    _seq_start = _seq_curr = seq_num;
    _update_loss_rate = true;
    _in_loss_event = true;

    if (!_li_head) {
	_li_head = newLI;
	_li_tail = newLI;
	_loss_interval_count = 1;
    } else {
	newLI->next = _li_head;
	newLI->next->prev = newLI;
	_li_head = newLI;
	_loss_interval_count++;
	while (_loss_interval_count > NWEIGHTS) {
	    LossInterval *tmp = _li_tail->prev;
	    delete _li_tail;
	    _li_tail = tmp;
	    _li_tail->next = 0;
	    _loss_interval_count--;
	}
    }

    _timer.schedule_after_msec((int) _trc->get_rtt());
}

CLICK_ENDDECLS
EXPORT_ELEMENT(LossRateCalculator)
