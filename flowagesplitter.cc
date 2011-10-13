// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include <click/error.hh>
#include <click/confparse.hh>
#include <click/packet_anno.hh>
#include "flowagesplitter.hh"
CLICK_DECLS

FlowAgeSplitter::FlowAgeSplitter()
    : _timer(this), _head(0), _tail(0), _curr_thresh_node(0), _curr_thresh_order(0),
      _hshFIdFDNode(0)
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

    _hshFIdFDNode = HashMap<uint32_t, FlowDataNode*>();

    _timer.initialize(this);
    _timer.schedule_now();

    return 0;
}

void
FlowAgeSplitter::add_handlers()
{
    add_write_handler("bump_flows", static_bump_flows, 0);
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
        click_chatter("New flow! %u", fID);
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
    click_chatter("Timer! Time for cleaning.");
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
            click_chatter("Delete flow! %u", curr->flowID);
#endif
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

    _curr_thresh_order = _curr_thresh_node->orderID;
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
CLICK_ENDDECLS
EXPORT_ELEMENT(FlowAgeSplitter)
