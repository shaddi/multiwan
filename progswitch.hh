// -*- c-basic-offset: 4 -*-
#ifndef CLICK_PROGSWITCH_HH
#define CLICK_PROGSWITCH_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

//#define CLICK_PROGSWITCH_DEBUG

/*
=c

ProgSwitch()

=s

A programmable switch.

=d

Acts as a programmable switch, it's handler can be called and it
passed an array where all packets coming in from port i go to port
ary[i].

Default: Initialized with input port i is mapped to output port i.
 */

class ProgSwitch : public Element {
public:
    ProgSwitch();
    ~ProgSwitch();

    const char *class_name() const { return "ProgSwitch"; }
    const char *port_count() const { return "1-/="; }
    const char *processing() const { return PUSH; }

    enum { CONFIGURE_PHASE = CONFIGURE_PHASE_DEFAULT };
    int configure_phase() const { return CONFIGURE_PHASE; }
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    void push(int, Packet *);
    void set_switch_map(int, const int[]);

private:

    int _total_ports;
    int *_map;

    static int static_set_switch_map(const String&, Element*, void*, ErrorHandler*);
};

CLICK_ENDDECLS
#endif
