// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "statesource.hh"
#include <click/confparse.hh>
#include <click/straccum.hh>
#include <click/error.hh>
#include <list>
#include "lossratecalculator.hh"
#include "timestamprttcalculator.hh"
CLICK_DECLS

StateSource::StateSource()
    : _sp_map_loss_rate(0), _sp_map_rtt(0), _timer(this),
      _interval(0, Timestamp::subsec_per_sec / 2), _max_rtt(0.0),
      _outfile(0)
{
    _sp_map_loss_rate = new std::map<uint32_t, void*>();
    _sp_map_rtt = new std::map<uint32_t, void*>();
}

StateSource::~StateSource()
{
}

int
StateSource::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if (cp_va_kparse(conf, this, errh,
		     "INTERVAL", cpkP, cpTimestamp, &_interval,
                     "OUTFILE", 0, cpFilename, &_outfilename,
		     cpEnd) < 0)
	return -1;

    return 0;
}

int
StateSource::initialize(ErrorHandler *errh)
{
    _timer.initialize(this);
    _timer.schedule_after(_interval);

    if (_outfilename) {
        _outfile = fopen(_outfilename.c_str(), "wb");
        if (!_outfile)
            return errh->error("%s: %s", _outfilename.c_str(), strerror(errno));
    }

    return 0;
}

void
StateSource::cleanup(CleanupStage)
{
    delete _sp_map_loss_rate;
    delete _sp_map_rtt;
}

void
StateSource::run_timer(Timer *timer)
{
    assert(timer == &_timer);

    Packet *p = create_state_packet();
    output(0).push(p);

    _timer.reschedule_after(_interval);
}

void
StateSource::add_state_provider(uint32_t id, stateprovider_t type, void *p)
{
    if (type == LOSS_RATE) {
	(*_sp_map_loss_rate)[id] = p;
    } else if (type == RTT) {
	(*_sp_map_rtt)[id] = p;
    }
}

double
StateSource::get_max_rtt()
{
    return _max_rtt;
}

Packet *
StateSource::create_state_packet()
{
    std::map<uint32_t, void*>::iterator it;
    std::list<ConnectionState> list;
    double max_rtt = 0;

    for (it = _sp_map_loss_rate->begin(); it != _sp_map_loss_rate->end(); it++) {
	if (_sp_map_rtt->find(it->first) != _sp_map_rtt->end()) {
	    LossRateCalculator *loss_rate = (LossRateCalculator*) it->second;
	    TimestampRTTCalculator *rtt = (TimestampRTTCalculator*) (*_sp_map_rtt)[it->first];

	    double tmp = rtt->get_rtt();
	    max_rtt = max_rtt > tmp ? max_rtt : tmp;

	    list.push_front(ConnectionState(it->first, loss_rate->get_loss_rate(), tmp));
	}
    }

    _max_rtt = max_rtt;

    int length = 4 + (list.size() * sizeof(ConnectionState));
    char *buffer = (char*) malloc(length);

    uint32_t *list_size = (uint32_t*) buffer;
    *list_size = static_cast<uint32_t>(list.size());

    String header = "STATE_SOURCE ";
    Timestamp now = Timestamp::now();
    ConnectionState *csp = (ConnectionState*) (buffer + 4);

    std::list<ConnectionState>::iterator itc;
    for (itc = list.begin(); itc != list.end(); itc++) {
	ConnectionState cs = *itc;
	*csp = cs;
        StringAccum sa;
#ifdef CLICK_STATESOURCE_DEBUG
        sa << header << now.unparse() << " ID " << cs.id << " LOSS_RATE " << cs.loss_rate << " RTT " << cs.rtt;
        if (_outfile) {
            sa << "\n";
            fwrite(sa.data(), 1, sa.length(), _outfile);
        } else {
            click_chatter("%s", sa.c_str());
        }
#endif
	csp ++;
    }

    Packet *p = Packet::make(buffer, length);

    free(buffer);

    return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(StateSource)
