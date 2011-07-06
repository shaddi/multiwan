// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "tcpreorderer.hh"
#include <click/ipaddress.hh>
#include <click/error.hh>
#include <click/packet_anno.hh>
#include <click/confparse.hh>
CLICK_DECLS

TCPReorderer::TCPReorderer()
    : _timer(this), _sanity_count(0), _elem_cld(0)
{
    for (int i = 0; i < NMAP; i++)
        _map[i] = 0;
}

TCPReorderer::~TCPReorderer()
{
}

int
TCPReorderer::configure(Vector<String> &conf, ErrorHandler *errh)
{
    Element *cld_element = 0;
    if (cp_va_kparse(conf, this, errh,
		     "STATESOURCE", cpkM, cpElement, &cld_element,
		     cpEnd) < 0)
	return -1;

    CalcLatencyDelta *cld = 0;
    if (cld_element && 
        !(cld = (CalcLatencyDelta *)(cld_element->cast("StateSource"))))
	return errh->error("CALCLATENCYDELTA must be a CalcLatencyDelta element");
    else if (cld)
	_elem_cld = cld;

    return 0;
}

int
TCPReorderer::initialize(ErrorHandler *)
{
    _timer.initialize(this);
    _timer.schedule_now();
    return 0;
}

void
TCPReorderer::add_handlers()
{
    //    add_write_handler("set_rtt", static_set_rtt, 0);
}

void
TCPReorderer::cleanup(CleanupStage)
{
    for (int i = 0; i < NMAP; i++) {
        while (_map[i]) {
            while (_map[i]->pkt) {
                Packet *p = remove_first_pkt(_map[i]);
                p->kill();
            }
            FlowNode *fn = _map[i];
            _map[i] = fn->next;

            delete fn;
        }
    }
}

void
TCPReorderer::push(int, Packet *p)
{
    FlowNode *fn = get_flow_node(p);
    if (!fn) { // Never seen before, so add to map and forward
        add_flow_node(p);
        output(0).push(p);
    } else { // We have seen it before
        if (SEQ_GEQ(fn->seq_num, static_get_seq_num(p))) { // should forward
            tcp_seq_t tmp = static_get_seq_num(p) + static_get_data_length(p);
            fn->seq_num = fn->seq_num > tmp ? fn->seq_num : tmp;

#ifdef CLICK_TCPREORDERER_DEBUG
            click_chatter("INORDER: flow_num(%u) seq_num(%u) pkt_length(%u) next_seq_num(%u)",
                          fn->flow_num, static_get_seq_num(p),
                          static_get_data_length(p), fn->seq_num);
#endif
            output(0).push(p);
            // forward any packets right after just arrived packet
            while(fn->pkt && SEQ_GEQ(fn->seq_num, static_get_seq_num(fn->pkt))) {
                Packet *q = remove_first_pkt(fn);
#ifdef CLICK_TCPREORDERER_DEBUG
                click_chatter("FORWARDING packet was holding: seq_num(%u)",
                              static_get_seq_num(q));
#endif
                output(0).push(q);
            }
            fn->timestamp = Timestamp::now().sec();
        } else {
#ifdef CLICK_TCPREORDERER_DEBUG
            click_chatter("OUT OF ORDER: flow_num(%u) pkt_seq_num(%u) seq_num(%u)",
                          fn->flow_num, static_get_seq_num(p), fn->seq_num);
#endif
            insert_pkt_into_flow_node(fn, p);
        }
    }

    _sanity_count++;
    if (_sanity_count >= CLEAN_MAP_SANITY) {
#ifdef CLICK_TCPREORDERER_DEBUG_OFF
	click_chatter("------SANITY CLEAN-------");
#endif
	clean_map(_map, Timestamp::now(), get_max_delta());
	_sanity_count = 0;
    }
}

void
TCPReorderer::run_timer(Timer *timer)
{
    assert(timer == &_timer);

#ifdef CLICK_TCPREORDERER_DEBUG_OFF
    click_chatter("------TIME CLEAN-------");
#endif

    int max_delta = get_max_delta();
    clean_map(_map, Timestamp::now(), max_delta);

    //max_delta is in usec, so first convert to msec
    _timer.reschedule_after_msec((max_delta/1000)/2);
}

// void
// TCPReorderer::set_rtt(int new_rtt)
// {
//     _max_rtt = new_rtt;
// }

// int
// TCPReorderer::static_set_rtt(const String &data, Element *element, void*,
//                              ErrorHandler *errh)
// {
//     TCPReorderer *tr = (TCPReorderer *)element;
//     int new_rtt = tr->_max_rtt;

//     if(!cp_integer(data, &new_rtt))
//         return errh->error("New RTT is not a number");

//     tr->set_rtt(new_rtt);

//     return 0;
// }

void
TCPReorderer::clean_map(FlowNode **map, const Timestamp now, int max_delta)
{
    const Timestamp pkt_timeout = now - Timestamp::make_usec(max_delta);
    Timestamp::seconds_type flow_timeout = now.sec() - REAP_TIMEOUT;

    for (int i = 0; i < NMAP; i++) {
        FlowNode *fn = map[i];
        FlowNode *prev = 0;
        while (fn) {
            // Check if flow is too old and remove if it is
            if (fn->timestamp <= flow_timeout) {
#ifdef CLICK_TCPREORDERER_DEBUG
                click_chatter("CLEANUP FN: flow_num(%u)", fn->flow_num);
#endif
                // Push out all packets
                while (fn->pkt) {
#ifdef CLICK_TCPREORDERER_DEBUG
                    click_chatter("  RELEASE PKT: seq_num(%u)",
                                  static_get_seq_num(fn->pkt));
#endif
                    output(0).push(remove_first_pkt(fn));
                }
                FlowNode *next = fn->next;
                if (prev) {
                    prev->next = next;
                } else {
                    map[i] = next;
                }
                delete fn;
                fn = next;
            } else if (fn->pkt) { // flow not old, check packets
                Packet *curr = fn->tail;
                while(curr && (curr->timestamp_anno() > pkt_timeout)) {
                    curr = curr->prev();
                }

                if (curr) { // There are packets we need to forward
                    while (fn->pkt != curr) {
#ifdef CLICK_TCPREORDERER_DEBUG
                    click_chatter("TIMEOUT PKT: flow_num(%u) seq_num(%u)",
                                  fn->flow_num, static_get_seq_num(fn->pkt));
#endif
                        output(0).push(remove_first_pkt(fn));
                    }
#ifdef CLICK_TCPREORDERER_DEBUG
                    click_chatter("TIMEOUT PKT: flow_num(%u) seq_num(%u)",
                                  fn->flow_num, static_get_seq_num(fn->pkt));
#endif
                    output(0).push(remove_first_pkt(fn)); // remove curr
                }
		fn = fn->next;
            } else {
		fn = fn->next;
	    }
        }
    }
}

void
TCPReorderer::add_flow_node(Packet *pkt)
{
    assert(pkt->has_transport_header());

    FlowNode *fn = new FlowNode;
    fn->flow_num = AGGREGATE_ANNO(pkt);
    fn->seq_num = static_get_seq_num(pkt);
    fn->seq_num+= static_get_data_length(pkt);
    fn->timestamp = Timestamp::now().sec();
    fn->next = _map[fn->flow_num % NMAP];
    fn->pkt = 0;

    _map[fn->flow_num % NMAP] = fn;

#ifdef CLICK_TCPREORDERER_DEBUG
    click_chatter("NEW FLOW: flow_num(%u) pkt_seq_num(%u) seq_num(%u)", 
                  fn->flow_num, static_get_seq_num(pkt), fn->seq_num);
#endif

}

TCPReorderer::FlowNode *
TCPReorderer::get_flow_node(Packet *pkt)
{
    FlowNode *curr = _map[AGGREGATE_ANNO(pkt) % NMAP];
    while(curr) {
        if (curr->flow_num == AGGREGATE_ANNO(pkt))
            return curr;
        curr = curr->next;
    }

    return 0;
}

void
TCPReorderer::insert_pkt_into_flow_node(FlowNode *fn, Packet *pkt)
{
    if (!fn || !pkt)
        return;

    if (!(pkt->timestamp_anno().sec()))
        pkt->timestamp_anno().assign_now();

    pkt->set_next(0);
    pkt->set_prev(0);

    if (!fn->pkt) {
        fn->pkt = pkt;
        fn->tail = pkt;
    } else if (SEQ_LT(static_get_seq_num(pkt), static_get_seq_num(fn->pkt))) {
        pkt->set_next(fn->pkt);
        fn->pkt->set_prev(pkt);
        fn->pkt = pkt;
    } else {
        Packet *curr = fn->pkt->next();
        Packet *prev = fn->pkt;
        while (curr &&
               SEQ_GEQ(static_get_seq_num(pkt), static_get_seq_num(curr))) {
            prev = curr;
            curr = curr->next();
        }
        pkt->set_next(curr);
        pkt->set_prev(prev);
        if (curr)
            curr->set_prev(pkt);
        else // at end of the list
            fn->tail =pkt;
        prev->set_next(pkt);
    }

    fn->timestamp = Timestamp::now().sec();
}

Packet *
TCPReorderer::remove_first_pkt(FlowNode *fn)
{
    if (!fn || !fn->pkt)
        return 0;

    Packet *q = fn->pkt;
    fn->pkt = q->next();

    q->set_prev(0);
    q->set_next(0);

    if (fn->pkt)
        fn->pkt->set_prev(0);
    else // no packets left
        fn->tail = 0;

    fn->seq_num = static_get_seq_num(q) + static_get_data_length(q);
    fn->timestamp = Timestamp::now().sec();
    return q;
}

int
TCPReorderer::get_max_delta()
{
    return _elem_cld->get_max_delta();
}

uint32_t
TCPReorderer::static_get_seq_num(Packet *p)
{
    assert(p->has_transport_header());
    return ntohl(p->tcp_header()->th_seq);
}

uint32_t
TCPReorderer::static_get_data_length(Packet *p)
{
    uint32_t length = p->length() - (p->tcp_header()->th_off * 4);
    if (length == 0 && ((p->tcp_header()->th_flags & TH_SYN) || (p->tcp_header()->th_flags & TH_FIN)))
        length = 1;     

    return length;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(TCPReorderer)
