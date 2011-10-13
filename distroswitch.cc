// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "distroswitch.hh"
#include <click/error.hh>
#include <click/confparse.hh>
CLICK_DECLS

/*
 */

DistroSwitch::DistroSwitch()
    : _distrub(0), _total_ports(0), _distrib_sum(0)
{
}

DistroSwitch::~DistroSwitch()
{
}

int
DistroSwitch::configure(Vector<String> &, ErrorHandler *)
{
    return 0;
}

int
DistroSwitch::initialize(ErrorHandler *)
{
    _total_ports = noutputs();
    _distrib = new uint32_t[_total_ports];
    for (int i = 0; i < _total_ports; i++)
        _distrib[i] = 1;

    _distrib_sum = _total_ports;

    return 0;
}

void
DistroSwitch::add_handlers()
{
    add_write_handler("set_distribution", static_set_distribution, 0);
}

void
DistroSwitch::cleanup(CleanupStage)
{
    delete[] _distrib;
}

void
DistroSwitch::push(int, Packet *p)
{
  int port = -1;
  uint32_t num = click_random(1,_distrib_sum);
  uint32_t sum = 0;
  for (int i = 0; i < _total_ports; i++) {
    sum += _distrib[i];
    if (num <= sum) {
      port = i;
      break;
    }
  }

#ifdef CLICK_DISTROSWITCH_DEBUG
    // click_chatter("[DISTROSWITCH] Num:%d Distrib_Sum:%d Packet sent port %d\n",
    //               num, _distrib_sum, port);
#endif
    output(port).push(p);
}

void
DistroSwitch::set_distribution(unsigned int total, const uint32_t distrib[])
{
    uint32_t *tmp_distrib = new uint32_t[total];
    int sum = 0;
    for (unsigned int i = 0; i < total; i++) {
        tmp_distrib[i] = distrib[i];
        sum += distrib[i];
    }

    uint32_t *tmp_distrib2 = _distrib;
    _total_ports = total;
    _distrib_sum = sum;
    _distrib = tmp_distrib;
    delete[] tmp_distrib2;

#ifdef CLICK_DISTROSWITCH_DEBUG
    click_chatter("[DISTROSWITCH] NEW DISTRIBUTION: ");
    for (int i = 0; i < _total_ports; i++)
        click_chatter("[DISTROSWITCH]  %d", _distrib[i]);
#endif
}

int
DistroSwitch::static_set_distribution(const String &data, Element *element, void*,
                                      ErrorHandler *errh)
{
    String str = data;
    Vector<String> conf;
    cp_spacevec(data, conf);

#ifdef CLICK_DISTROSWITCH_DEBUG
    click_chatter("[DISTROSWITCH] string input length %d\n", conf.size());
    for(int i = 0; i < conf.size(); i++)
        click_chatter("[DISTROSWITCH] value %d '%s'\n", i, conf[i].c_str());
#endif

    int total = 0;
    if(!cp_integer(conf[0], &total))
        return errh->error("Total port numbers in schedule is not a number");

#ifdef CLICK_DISTROSWITCH_DEBUG
    click_chatter("[DISTROSWITCH] total = %d\n", total);
#endif

    uint32_t tmp_distrib[total];
    for (int i = 0; i < total; i++)
        tmp_distrib[i] = 0;

    int i = 0;
    int tmp = -1;
    while(i < total && cp_integer(conf[i+1], &tmp)) {
        tmp_distrib[i] = tmp;
        i++;
    }
 
    DistroSwitch *ps = (DistroSwitch *)element;
    ps->set_distribution(total, tmp_distrib);
  
    return 0;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(DistroSwitch)
