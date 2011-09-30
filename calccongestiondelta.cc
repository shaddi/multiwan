// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "calccongestiondelta.hh"
#include <click/confparse.hh>
#include <stdio.h>
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

  _prev_my_timestamp = time / CLICK_CALCCONGESTIONDELTA_SENSITIVITY;
  _prev_o_timestamp = time / CLICK_CALCCONGESTIONDELTA_SENSITIVITY;
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
    add_read_handler("get_congestion_score", static_get_congestion_score,
                     (void *) 0);
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
  uint64_t new_my_timestamp = p->timestamp_anno().sec() * ((uint64_t) 1000000) +
    p->timestamp_anno().usec();

  //TODO: This is not cross platform independent, fix at some point.
  uint64_t new_o_timestamp = *((uint64_t*) (p->data()+_offset));
  // for (int i = 0; i < 8; i++)
  //   new_o_timestamp |= (((p->data())[_offset+i]) << ((8-i-1)*8));

#ifdef CLICK_CALCCONGESTIONDELTA_DEBUG
    click_chatter("[CALCCONGESTIONDELTA] timestamps - my %llu o %llu\n",
                  new_my_timestamp, new_o_timestamp);
    // click_chatter("[CALCCONGESTIONDELTA] new_o_timestamp %x%x %x%x %x%x %x%x",
    //               (p->data()+_offset)[0],
    //               (p->data()+_offset)[1],
    //               (p->data()+_offset)[2],
    //               (p->data()+_offset)[3],
    //               (p->data()+_offset)[4],
    //               (p->data()+_offset)[5],
    //               (p->data()+_offset)[6],
    //               (p->data()+_offset)[7]);
#endif

  new_my_timestamp = new_my_timestamp / CLICK_CALCCONGESTIONDELTA_SENSITIVITY;
  new_o_timestamp = new_o_timestamp / CLICK_CALCCONGESTIONDELTA_SENSITIVITY;

#ifdef CLICK_CALCCONGESTIONDELTA_DEBUG
    click_chatter("[CALCCONGESTIONDELTA] timestamps converted - my:%llu o:%llu\n",
                  new_my_timestamp, new_o_timestamp);
#endif

  _listTail->my_delta = new_my_timestamp - _prev_my_timestamp;
  _listTail->o_delta = new_o_timestamp - _prev_o_timestamp;
  _listTail->congested = _listTail->my_delta > _listTail->o_delta;

#ifdef CLICK_CALCCONGESTIONDELTA_DEBUG
    click_chatter("[CALCCONGESTIONDELTA] delta - my:%lu o:%lu congested:%s\n",
                  _listTail->my_delta, _listTail->o_delta,
                  _listTail->congested ? "T" : "F");
#endif

  _prev_my_timestamp = new_my_timestamp;
  _prev_o_timestamp = new_o_timestamp;
  CongestionDeltaElem *tmp = _listTail;
  _listTail = _listTail->prev;
  _listHead = tmp;

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
    click_chatter("[CALCCONGESTIONDELTA] Something happened to the list! i=%d should be 16", i);

    char binary[17];
    binary[16] = '\0';
    i = 0;
    curr = _listHead;
    do {
        if (curr->congested)
            binary[i] = '1';
        else
            binary[i] = '0';
        
        curr = curr->next;
        i++;
    } while (curr != _listHead);

    click_chatter("[CALCCONGESTIONDELTA] Binary %s", binary);
#endif

  return tmp;
}

String
CalcCongestionDelta::static_get_congestion_score(Element *e, void *)
{
    CalcCongestionDelta *ccd = (CalcCongestionDelta*) e;
    unsigned short cs = ccd->get_congestion_score();

    // char binary[18];
    // binary[16] = '\n';
    // binary[17] = 0;
    // CongestionDeltaElem *curr = ccd->_listHead;
    // int i = 0;

    // do {
    //     if (curr->congested)
    //         binary[i] = '1';
    //     else
    //         binary[i] = '0';
        
    //     curr = curr->next;
    //     i++;
    // } while (curr != ccd->_listHead);
    
    // return String(binary);

    char str[20];
    sprintf(str, "%x\n", cs);

    return String(str);
}


CLICK_ENDDECLS
EXPORT_ELEMENT(CalcCongestionDelta)
