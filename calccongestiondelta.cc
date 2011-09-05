// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "calccongestiondelta.hh"
#include <click/confparse.hh>
CLICK_DECLS

CalcCongestionDelta::CalcCongestionDelta()
  :_offset(0)
{
  CongestionDeltaElem *curr = new CongestionDeltaElem;
  curr->my_delta = 0;
  curr->o_delta = 0;
  curr->congested = false;
  curr->next = NULL;
  curr->prev = NULL;

  _listHead = curr;

  for (int i = 1; i < CLICK_CALCCONGESTIONDELTA_MAX_DELTAS; i++) {
    CongestionDeltaElem *tmp = new CongestionDeltaElem;
    tmp->my_delta = 0;
    tmp->o_delta = 0;
    tmp->congested = false;
    tmp->next = NULL;
    tmp->prev = curr;

    curr->next = tmp;
    curr = tmp;
    _listTail = curr;
  }

  _listTail->next = _listHead;
  _listHead->prev = _listTail;

  Timestamp now = Timestamp::now();
  uint64_t time = (now.sec()*((uint64_t)(1000000)));
  time += now.usec();

  _prev_my_timestamp = time;
  _prev_o_timestamp = time;
}

CalcCongestionDelta::~CalcCongestionDelta()
{
}

int
CalcCongestionDelta::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if (cp_va_kparse(conf, this, errh,
		     "OFFSET", cpkM, cpUnsigned, &_offset,
		     cpEnd) < 0)
	return -1;

  return 0;
}

int
CalcCongestionDelta::initialize(ErrorHandler *)
{
  return 0;
}

void
CalcCongestionDelta::add_handlers()
{
  //TODO: add get_congestion_score handler
}

void
CalcCongestionDelta::cleanup(CleanupStage)
{
  CongestionDeltaElem *curr = _listHead;
  do {
    CongestionDeltaElem *tmp = curr->next;

    delete curr;
    curr = tmp;
  } while (curr != _listHead);
}

Packet*
CalcCongestionDelta::simple_action(Packet *p)
{
  //TODO: implement simple_action
  uint64_t new_my_timestamp = 0;
  uint64_t new_o_timestamp = 0;

  new_my_timestamp = p->timestamp_anno().sec() * ((uint64_t) 1000000) +
    p->timestamp_anno().usec();
  for (int i = 0; i < 8; i++)
    new_o_timestamp |= (((p->data())[_offset+i]) << ((8-i-1)*8));

  _listTail->my_delta = new_my_timestamp - _prev_my_timestamp;
  _listTail->o_delta = new_o_timestamp - _prev_o_timestamp;
  _listTail->congested = _listTail->my_delta > _listTail->o_delta;

  _prev_my_timestamp = new_my_timestamp;
  _prev_o_timestamp = new_o_timestamp;
  _listTail = _listTail->prev;
  _listHead = _listTail;

  return p;
}

unsigned short
CalcCongestionDelta::get_congestion_score()
{
  unsigned short tmp = 0;
  int i = 0;
  CongestionDeltaElem *curr = _listHead;

  do {
    int offset = CLICK_CALCCONGESTIONDELTA_MAX_DELTAS - i - 1;
    i++;
    if (curr->congested)
      tmp |= (1 << offset);
    else
      tmp |= (0 << offset);

    curr = curr->next;
  } while (curr != _listHead);

#ifdef CLICK_CALCCONGESTIONDELTA_DEBUG
  if (i != CLICK_CALCCONGESTIONDELTA_MAX_DELTAS)
    click_chatter("CALCCONGESTIONDELTA: Something happened to the list! i=%d should be 16", i);
#endif

  return tmp;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(CalcCongestionDelta)
