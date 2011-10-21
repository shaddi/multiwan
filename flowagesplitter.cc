// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include <click/error.hh>
#include <click/confparse.hh>
#include <click/packet_anno.hh>
#include "flowagesplitter.hh"
CLICK_DECLS

FlowAgeSplitter::FlowAgeSplitter()
    : _timer(this), _head(0), _tail(0), _curr_thresh_node(0), _curr_thresh_order(0),
      _c_flow(0),_hshFIdFDNode(0)
{
}

FlowAgeSplitter::~FlowAgeSplitter()
{
}

int
FlowAgeSplitter::configure(Vector<String> &, ErrorHandler *)
{
    return 0;
}

int
FlowAgeSplitter::initialize(ErrorHandler *)
{
    // To reduce risk of the list becoming empty if we don't see any packets by
    // the timeout the head and tail element are premanent and do not
    // change. All code will be written to reflect this.

    _tail = new FlowDataNode();
    _head = new FlowDataNode();

    _tail->flowID = -1;
    _tail->orderID = 0;
    _tail->delete_me = false;
    _tail->next = _head;
    _tail->prev = _head;

    _head->flowID = -1;
    _head->orderID = 0;
    _head->delete_me = false;
    _head->next = _tail;
    _head->prev = _tail;

    _curr_thresh_node = _tail;
    _curr_thresh_order = _tail->orderID;

    _c_flow = new int[2];
    for (int i = 0; i < 2; i++)
        _c_flow[i] = 0;

    _hshFIdFDNode = HashMap<uint32_t, FlowDataNode*>();

    _timer.initialize(this);
    _timer.schedule_now();

    return 0;
}

void
FlowAgeSplitter::add_handlers()
{
    add_write_handler("bump_flows", static_bump_flows, 0);
    add_read_handler("get_flow_count", static_get_flow_count, (void *) 0);
}

void
FlowAgeSplitter::cleanup(CleanupStage)
{
    FlowDataNode *curr = _head;
    while (curr != _tail) {
        FlowDataNode *tmp = curr->next;
        delete curr;
        curr = tmp;
    }

    delete[] _c_flow;
}

void
FlowAgeSplitter::push(int, Packet *p)
{
    uint32_t fID = AGGREGATE_ANNO(p);
    FlowDataNode *fdNode = _hshFIdFDNode[fID];
    if (fdNode) {
        fdNode->delete_me = false;
    } else {
#ifdef CLICK_FLOWAGESPLITTER_DEBUG
        click_chatter("[FLOWAGESPLITTER] New flow! %u", fID);
#endif
        fdNode = new FlowDataNode();
        fdNode->flowID = fID;
        fdNode->orderID = _head->next->orderID + 1;
        fdNode->delete_me = false;

        _hshFIdFDNode.insert(fID, fdNode);

        fdNode->next = _head->next;
        fdNode->prev = _head;
        _head->next->prev = fdNode;
        _head->next = fdNode;

        _c_flow[0]++;
    }

    if (fdNode->orderID > _curr_thresh_order)
        output(0).push(p);
    else
        output(1).push(p);
}

void
FlowAgeSplitter::run_timer(Timer *timer)
{
    assert(timer == &_timer);

#ifdef CLICK_FLOWAGESPLITTER_DEBUG
    //    click_chatter("Timer! Time for cleaning.");
    click_chatter("[FLOWAGESPLITTER] Flow counts: %d %d", _c_flow[0], _c_flow[1]);
#endif

    _head->delete_me = false;
    _tail->delete_me = false;
    _curr_thresh_node->delete_me = false; //Not a huge hardship if this is not
                                        //reaped as well

    FlowDataNode *curr = _tail;
    while (curr != _head) {
        FlowDataNode *tmp = curr->prev;

        if (curr->delete_me) { // reap node, haven't seen a new packet in a while.
#ifdef CLICK_FLOWAGESPLITTER_DEBUG
            click_chatter("[FLOWAGESPLITTER] Delete flow! %u", curr->flowID);
#endif
            if (curr->orderID > _curr_thresh_order)
                _c_flow[0]--;
            else
                _c_flow[1]--;

            curr->prev->next = curr->next;
            curr->next->prev = curr->prev;
            delete curr;
        } else {
            curr->delete_me = true;
        }

        curr = tmp;
    }

    _timer.reschedule_after_sec(FLOWAGESPLITTER_CLEANUP_INTERVAL);
}

void
FlowAgeSplitter::bump_flows(unsigned int n)
{
    unsigned int i = 0;
    while ((i < n) && (_curr_thresh_node->prev != _head)) {
        _curr_thresh_node = _curr_thresh_node->prev;

        i++;
    }

#ifdef CLICK_FLOWAGESPLITTER_DEBUG
    click_chatter("[FLOWAGESPLITTER] Told to bump %u flows, bump %u", n, i);
#endif

    _curr_thresh_order = _curr_thresh_node->orderID;

    _c_flow[0] -= i;
    _c_flow[1] += i;
}

int
FlowAgeSplitter::static_bump_flows(const String &data, Element *element, void*,
                                   ErrorHandler *errh)
{
    FlowAgeSplitter *fas = (FlowAgeSplitter *) element;
    int n = 0;

    if (!cp_integer(data, &n))
        return errh->error("Number given to bump_flows is not a number");

    if (n < 0)
        return errh->error("Number for bump_flows should be greater than 0");

    fas->bump_flows((unsigned int)n);

    return 0;
}

String
FlowAgeSplitter::static_get_flow_count(Element *e, void*)
{
    char buffer[256];
    sprintf(buffer, "%d %d", ((FlowAgeSplitter*)e)->_c_flow[0],
            ((FlowAgeSplitter*)e)->_c_flow[1]);

    return String(buffer);
}
CLICK_ENDDECLS
EXPORT_ELEMENT(FlowAgeSplitter)
