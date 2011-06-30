// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "stateprocessor.hh"
#include <click/confparse.hh>
#include <click/straccum.hh>
#include <click/error.hh>
#include <list>
#include <math.h>
#include "connectionstate.hh"
CLICK_DECLS

StateProcessor::StateProcessor()
    : _outfile(0)
{
}

StateProcessor::~StateProcessor()
{
}

int
StateProcessor::configure(Vector<String> &conf, ErrorHandler *errh)
{
    Element *ps_element = 0;
    if (cp_va_kparse(conf, this, errh,
		     "PROGSWITCH", cpkM, cpElement, &ps_element,
                     "OUTFILE", 0, cpFilename, &_outfilename,
                     cpEnd) < 0)
	return -1;

    ProgSwitch *ps = 0;
    if (ps_element && !(ps = (ProgSwitch *)(ps_element->cast("ProgSwitch"))))
	return errh->error("ProgSwitch must be a ProgSwitch element");
    else if (ps)
	_progswitch = ps;
    else
	return -1;

    return 0;
}

int
StateProcessor::initialize(ErrorHandler *errh)
{
    if (_outfilename) {
        _outfile = fopen(_outfilename.c_str(), "wb");
        if (!_outfile)
            return errh->error("%s: %s", _outfilename.c_str(), strerror(errno));
    }

    return 0;
}

void
StateProcessor::cleanup(CleanupStage)
{
}

void
StateProcessor::push(int, Packet *p)
{
    const unsigned char *data = p->data();

    uint32_t size = *((uint32_t*) data);

    ConnectionState *csp = (ConnectionState*) (data + sizeof(uint32_t));

    String header = "STATE_PROCESSOR ";
    Timestamp now = Timestamp::now();

    std::list<IDWeight> list;
    for (uint32_t i = 0; i < size; i++) {
	double loss_rate = csp[i].loss_rate != 0 ? csp[i].loss_rate : .000001;
	double weight = 1 / (sqrt(loss_rate) * csp[i].rtt);
#ifdef CLICK_STATEPROCESSOR_DEBUG
        StringAccum sa;
        sa << header << now.unparse() << " ID " << csp[i].id << " LOSS_RATE ";
        sa << csp[i].loss_rate << " RTT " << csp[i].rtt << " WEIGHT " << weight;
        if (_outfile) {
            sa << "\n";
            fwrite(sa.data(), 1, sa.length(), _outfile);
        } else {
            click_chatter("%s", sa.c_str());
        }
#endif
	list.push_front(IDWeight(csp[i].id, weight));
    }

    list.sort();

    std::list<IDWeight>::iterator it;
    int port = 0;
    int port_map[size+1];
    uint32_t count = 0;
    for (it = list.begin(); it != list.end(); it++) {
#ifdef CLICK_STATEPROCESSOR_DEBUG
        StringAccum sa;
        sa << header << now.unparse() << " MAPPING PORT " << port;
        sa << " ID " << it->id;
        if (_outfile) {
            sa << "\n";
            fwrite(sa.data(), 1, sa.length(), _outfile);
        } else {
            click_chatter("%s", sa.c_str());
        }
#endif
	port_map[port] = it->id;
	port = it->id + 1;
	count++;
    }

    port_map[port] = size;
#ifdef CLICK_STATEPROCESSOR_DEBUG
        StringAccum sa;
        sa << header << now.unparse() << " MAPPING PORT " << port;
        sa << " ID " << size;
        if (_outfile) {
            sa << "\n";
            fwrite(sa.data(), 1, sa.length(), _outfile);
        } else {
            click_chatter("%s", sa.c_str());
        }
#endif

    assert(count == size);

    _progswitch->set_switch_map(size+1, port_map);

    p->kill();
}

CLICK_ENDDECLS
EXPORT_ELEMENT(StateProcessor)
