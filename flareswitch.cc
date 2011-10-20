// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "flareswitch.hh"
#include <click/error.hh>
#include <click/confparse.hh>
#include <clicknet/tcp.h>
#include <clicknet/udp.h>
#include <click/vector.hh>
CLICK_DECLS

/*
 */

FlareSwitch::FlareSwitch()
    : _distrib(0), _tokens(0), _total_ports(0), _distrib_sum(0), _mtbs(0),
      _token_reset_int(FLARESWITCH_TOKEN_RESET_DEFAULT_INT),_hshFIdFD(0),
      _timer(this)
{
}

FlareSwitch::~FlareSwitch()
{
}

int
FlareSwitch::configure(Vector<String> &conf, ErrorHandler *errh)
{
    unsigned int tmp = 0;
    if (cp_va_kparse(conf, this, errh,
		     "START_MTBS", cpkM, cpUnsigned, &_mtbs,
		     "TOKEN_RESET_INT", cpkN, cpUnsigned, &tmp,
		     cpEnd) < 0)
	return -1;

    if (tmp > 0)
        _token_reset_int = tmp;

#ifdef CLICK_FLARESWITCH_DEBUG
    click_chatter("[FLARESWITCH] configure");
#endif

    return 0;
}

int
FlareSwitch::initialize(ErrorHandler *)
{
    _total_ports = noutputs();
    _distrib = new uint32_t[_total_ports];
    _tokens = new int64_t[_total_ports];
    for (int i = 0; i < _total_ports; i++) {
        _distrib[i] = 1;
        _tokens[i] = 0;
    }

    _distrib_sum = _total_ports;

    _hshFIdFD = HashMap<unsigned short, FlowData*>();

    _timer.initialize(this);
    _timer.schedule_now();

#ifdef CLICK_FLARESWITCH_DEBUG
    click_chatter("[FLARESWITCH] initialize");
#endif

    return 0;
}

void
FlareSwitch::add_handlers()
{
    add_write_handler("set_distribution", static_set_distribution, 0);
    add_write_handler("set_mtbs", static_set_mtbs, 0);
}

void
FlareSwitch::cleanup(CleanupStage)
{
#ifdef CLICK_FLARESWITCH_DEBUG
    click_chatter("[FLARESWITCH] cleanup");
#endif

    delete[] _distrib;
    delete[] _tokens;

    HashMap<unsigned short, FlowData*>::iterator iter = _hshFIdFD.begin();
    for (; iter != _hshFIdFD.end(); iter++)
        delete iter.value();
}

void
FlareSwitch::push(int, Packet *p)
{
#ifdef CLICK_FLARESWITCH_DEBUG
    //    click_chatter("[FLARESWITCH] push");
#endif

    update_tokens(p->length());

#ifdef CLICK_FLARESWITCH_DEBUG
    //    click_chatter("[FLARESWITCH] here1");
#endif

    const click_ip *iph = p->ip_header();

    FlowTuple ft;
    ft.protocol = iph->ip_p;
    ft.ip_src = iph->ip_src;
    ft.ip_dst = iph->ip_dst;
    if (ft.protocol == TCP) {
        ft.port_src = p->tcp_header()->th_sport;
        ft.port_dst = p->tcp_header()->th_dport;
    } else if (ft.protocol == UDP) {
        ft.port_src = p->udp_header()->uh_sport;
        ft.port_dst = p->udp_header()->uh_dport;
    } else {
        ft.port_src = -1;
        ft.port_dst = -1;
    }

    unsigned short flowID = crc16((char*) &ft, sizeof(FlowTuple));

    FlowData *fd = _hshFIdFD[flowID];

#ifdef CLICK_FLARESWITCH_DEBUG
    //    click_chatter("[FLARESWITCH] here2");
#endif

    int port;
    uint64_t new_time = p->timestamp_anno().sec() * ((uint64_t) 1000000) +
        p->timestamp_anno().usec();

    if (fd) {
        if (new_time < (fd->last_seen_time + _mtbs))
            port = fd->port;
        else
            port = max_tokens_port();
    } else {
        port = max_tokens_port();

        fd = new FlowData();
        fd->flow_tuple.protocol = ft.protocol;
        fd->flow_tuple.ip_src = ft.ip_src;
        fd->flow_tuple.ip_dst = ft.ip_dst;
        fd->flow_tuple.port_src = ft.port_src;
        fd->flow_tuple.port_dst = ft.port_dst;
        fd->flowID = flowID;

        _hshFIdFD.insert(flowID, fd);
    }

    fd->last_seen_time = new_time;
    fd->port = port; 

    _tokens[port] -= p->length();
    output(port).push(p);
}

void
FlareSwitch::run_timer(Timer *timer)
{
    assert(timer == &_timer);

#ifdef CLICK_FLARESWITCH_DEBUG
    //    click_chatter("[FLARESWITCH] run_timer");
#endif

    for (int i = 0; i < _total_ports; i++)
        _tokens[i] = 0;

    Vector<unsigned short> keys_to_delete;

    HashMap<unsigned short, FlowData*>::iterator iter = _hshFIdFD.begin();
    Timestamp now = Timestamp::now();
    uint64_t now_us = now.sec() * ((uint64_t)(1000000));
    now_us += now.usec();

    for (; iter != _hshFIdFD.end(); iter++) {
        if ((now_us - iter.value()->last_seen_time) > 120000)
            keys_to_delete.push_back(iter.pair()->key);
    }

    Vector<unsigned short>::iterator iterV = keys_to_delete.begin();
    for (; iterV != keys_to_delete.end(); iterV++) {
        FlowData *p_fd = _hshFIdFD[*iterV];
        _hshFIdFD.remove(*iterV);
        delete p_fd;
    }

    _timer.reschedule_after_msec(_token_reset_int);
}

void
FlareSwitch::set_distribution(unsigned int total, const uint32_t distrib[])
{
#ifdef CLICK_FLARESWITCH_DEBUG
    click_chatter("[FLARESWITCH] set_distribution");
#endif
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

#ifdef CLICK_FLARESWITCH_DEBUG
    click_chatter("[FLARESWITCH] NEW DISTRIBUTION: ");
    for (int i = 0; i < _total_ports; i++)
        click_chatter("[FLARESWITCH]  %d", _distrib[i]);
#endif
}

void
FlareSwitch::set_mtbs(unsigned int mtbs)
{
#ifdef CLICK_FLARESWITCH_DEBUG
    //    click_chatter("[FLARESWITCH] set_mtbs");
#endif
    _mtbs = mtbs;
}

void
FlareSwitch::update_tokens(uint32_t length)
{
#ifdef CLICK_FLARESWITCH_DEBUG
    //    click_chatter("[FLARESWITCH] update_tokens");
#endif
    for (int i = 0; i < _total_ports; i++) {
        int add = (length * _distrib[i])/_distrib_sum;
        _tokens[i] += add;
    }
}

int
FlareSwitch::max_tokens_port()
{
#ifdef CLICK_FLARESWITCH_DEBUG
    //    click_chatter("[FLARESWITCH] max_tokens_port");
#endif

    int index = 0;
    int64_t max = _tokens[0];
    for (int i = 0; i < _total_ports; i++)
        if (max < _tokens[i]) {
            max = _tokens[i];
            index = i;
        }

    return index;
}

int
FlareSwitch::static_set_distribution(const String &data, Element *element, void*,
                                      ErrorHandler *errh)
{
    String str = data;
    Vector<String> conf;
    cp_spacevec(data, conf);

#ifdef CLICK_FLARESWITCH_DEBUG
    click_chatter("[FLARESWITCH] string input length %d\n", conf.size());
    for(int i = 0; i < conf.size(); i++)
        click_chatter("[FLARESWITCH] value %d '%s'\n", i, conf[i].c_str());
#endif

    int total = 0;
    if(!cp_integer(conf[0], &total))
        return errh->error("Total port numbers in schedule is not a number");

#ifdef CLICK_FLARESWITCH_DEBUG
    click_chatter("[FLARESWITCH] total = %d\n", total);
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
 
    FlareSwitch *ps = (FlareSwitch *)element;
    ps->set_distribution(total, tmp_distrib);
  
    return 0;
}

int
FlareSwitch::static_set_mtbs(const String &data, Element *element, void*,
                                      ErrorHandler *errh)
{
    unsigned int tmp = 0;
    if (!cp_integer(data, &tmp))
        return errh->error("Value for mtbs is not a number");

    FlareSwitch *fs = (FlareSwitch *) element;
    fs->set_mtbs(tmp);

    return 0;
}

#define POLY 0x8408
/*
//                                      16   12   5
// this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
// This works out to be 0x1021, but the way the algorithm works
// lets us use 0x8408 (the reverse of the bit pattern).  The high
// bit is always assumed to be set, thus we only use 16 bits to
// represent the 17 bit value.
*/
unsigned short
FlareSwitch::crc16(char *data_p, unsigned short length)
{

    unsigned char i;
    unsigned int data;
    unsigned int crc = 0xffff;

    if (length == 0)
        return (~crc);

    do
        {
            for (i=0, data=(unsigned int)0xff & *data_p++;
                 i < 8; 
                 i++, data >>= 1)
                {
                    if ((crc & 0x0001) ^ (data & 0x0001))
                        crc = (crc >> 1) ^ POLY;
                    else  crc >>= 1;
                }
        } while (--length);

    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xff);

    return (crc);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(FlareSwitch)
