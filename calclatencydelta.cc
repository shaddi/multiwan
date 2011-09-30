// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "calclatencydelta.hh"
#include <click/error.hh>
#include <click/confparse.hh>
#include <click/packet_anno.hh>
CLICK_DECLS

CalcLatencyDelta::CalcLatencyDelta()
    : _deltas(0), _last_latency(0), _max_paint(0), _offset(0)
{
}

CalcLatencyDelta::~CalcLatencyDelta()
{
}

int
CalcLatencyDelta::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if (cp_va_kparse(conf, this, errh,
		     "OFFSET", cpkM, cpUnsigned, &_offset,
		     "MAX_PAINT", cpkM, cpUnsigned, &_max_paint,
		     cpEnd) < 0)
	return -1;

    _last_latency = new uint64_t[_max_paint];

    _deltas = new int *[_max_paint];
    for (unsigned int i = 0; i < _max_paint; i++) {
        _deltas[i] = new int[_max_paint];

        _last_latency[i] = 0;
    }

    return 0;
}

int
CalcLatencyDelta::initialize(ErrorHandler *)
{
    return 0;
}

void
CalcLatencyDelta::add_handlers()
{
    add_read_handler("get_max_delta", static_get_max_delta, (void *) 0);
}

void
CalcLatencyDelta::cleanup(CleanupStage)
{
    for (unsigned int i = 0; i < _max_paint; i++)
        delete _deltas[i];
    delete _deltas;
    
    delete _last_latency;
}

Packet *
CalcLatencyDelta::simple_action(Packet *p)
{
    // _deltas[i][j] = latency_i - latency_j
    int paint = PAINT_ANNO(p);
    // TODO: This is a hack, not cross-platform, fix in the future.
    uint64_t src_timestamp = *((uint64_t*) p->data());
    // uint64_t src_timestamp = 0;
    // for (int i = 0; i < 8; i++)
    //     src_timestamp |= (((p->data())[_offset+i]) << ((8-i-1)*8));
    
    uint64_t dst_timestamp = p->timestamp_anno().sec() * ((uint64_t) 1000000);
    dst_timestamp += p->timestamp_anno().usec();

    uint64_t latency = dst_timestamp - src_timestamp;
    _last_latency[paint] = latency;

    for (unsigned int i = 0; i < _max_paint; i++) {
        if (_last_latency[i] != 0) {
            //TODO: Now just replace latency delta, in future rolling change? 
            _deltas[paint][i] = latency - _last_latency[i];
            _deltas[i][paint] = _last_latency[i] - latency;
        }
    }

    return p;
}

// Make sure to delete/free the data returned
int**
CalcLatencyDelta::get_deltas()
{
    int **deltas = new int *[_max_paint];
    for (unsigned int i = 0; i < _max_paint; i++) {
        deltas[i] = new int[_max_paint];

        for (unsigned int j = 0; j < _max_paint; j++)
            deltas[i][j] = _deltas[i][j];
    }

    return deltas;
}

int
CalcLatencyDelta::get_max_delta()
{
    int max = 0;
    for (unsigned int i = 0; i < _max_paint; i++) {
        for (unsigned int j = 0; j < _max_paint; j++)
            if (_deltas[i][j] > max)
                max = _deltas[i][j];
    }

    return max;    
}

String
CalcLatencyDelta::static_get_max_delta(Element *e, void*)
{
    return String(((CalcLatencyDelta*)e)->get_max_delta());
}

CLICK_ENDDECLS
EXPORT_ELEMENT(CalcLatencyDelta)
