// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "progscheduler.hh"
#include <click/error.hh>
#include <click/confparse.hh>
CLICK_DECLS

/*
 */

ProgScheduler::ProgScheduler()
{
}

ProgScheduler::~ProgScheduler()
{
}

int
ProgScheduler::configure(Vector<String> &, ErrorHandler *)
{
    return 0;
}

int
ProgScheduler::initialize(ErrorHandler *)
{
    _total_ports = noutputs();
    _schedule = new int[_total_ports];
    for (int i = 0; i < _total_ports; i++)
        _schedule[i] = 1;

    _curr_port = 0;
    _curr_port_count = 0;
    return 0;
}

void
ProgScheduler::add_handlers()
{
    add_write_handler("set_schedule", static_set_schedule, 0);
}

void
ProgScheduler::cleanup(CleanupStage)
{
    delete[] _schedule;
}

void
ProgScheduler::push(int, Packet *p)
{

#ifdef CLICK_PROGSWITCH_DEBUG
    click_chatter("Packet sent port %d count %d\n", _curr_port,
                  _curr_port_count+1);
#endif
    output(_curr_port).push(p);
    
    _curr_port_count++;

    while (_curr_port_count >= _schedule[_curr_port]) {
        _curr_port = (_curr_port+1) % _total_ports;
        _curr_port_count = 0;
    }
}

void
ProgScheduler::set_schedule(int total, const int schedule[])
{
    int *tmp_sch = new int[total];
    for (int i = 0; i < total; i++)
        tmp_sch[i] = schedule[i];

    int *tmp_sch2 = _schedule;
    _total_ports = total;
    _schedule = tmp_sch;
    delete[] tmp_sch2;

#ifdef CLICK_PROGSCHEDULE_DEBUG
    click_chatter("NEW SCHEDULE: ");
    for (int i = 0; i < _total_ports; i++)
        click_chatter(" %d", _schedule[i]);
#endif
}

int
ProgScheduler::static_set_schedule(const String &data, Element *element, void*,
                                   ErrorHandler *errh)
{
    String str = data;
    Vector<String> conf;
    cp_spacevec(data, conf);

#ifdef CLICK_PROGSCHEDULE_DEBUG
    click_chatter("string input length %d\n", conf.size());
    for(int i = 0; i < conf.size(); i++)
        click_chatter("value %d '%s'\n", i, conf[i].c_str());
#endif

    int total = 0;
    if(!cp_integer(conf[0], &total))
        return errh->error("Total port numbers in schedule is not a number");

#ifdef CLICK_PROGSCHEDULE_DEBUG
    click_chatter("total = %d\n", total);
#endif

    int tmp_sch[total];
    for (int i = 0; i < total; i++)
        tmp_sch[i] = 0;

    int i = 0;
    int tmp = -1;
    while(i < total && cp_integer(conf[i+1], &tmp)) {
        tmp_sch[i] = tmp;
        i++;
    }
 
    ProgScheduler *ps = (ProgScheduler *)element;
    ps->set_schedule(total, tmp_sch);
  
    return 0;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ProgScheduler)
