// -*- c-basic-offset: 4 -*-
#ifndef CLICK_FLARESWITCH_HH
#define CLICK_FLARESWITCH_HH
#include <click/element.hh>
#include <click/glue.hh>
#include <click/hashmap.hh>
#include <click/timer.hh>

struct click_ip;

CLICK_DECLS

#define CLICK_FLARESWITCH_DEBUG

/*
  =c

  FlareSwitch(START_MTBS, TOKEN_RESET_INT)

  =s local

  Load balance switch using FLARE algorithm based on distribution. Default is
  equal distribution between all the output ports.

  Required elements before it: MarkIPHeader, SetTimestamp

  =d

  Sends packets out it's ports based on a distribution given to it using the
  FLARE algorithm.

  Keyword arguments:

  =item MTBS

  The starting minimum time before switching-ability for flowlets (ms)

  =item TOKEN_RESET_INT

  Optional; The interval of how long until the tokens are reset (ms) (Default
  500ms)

*/

#define FLARESWITCH_TOKEN_RESET_DEFAULT_INT 500

class FlareSwitch : public Element {
public:
    FlareSwitch();
    ~FlareSwitch();

    const char *class_name() const { return "FlareSwitch"; }
    const char *port_count() const { return "1/1-"; }
    const char *processing() const { return PUSH; }
  
    enum { CONFIGURE_PHASE = CONFIGURE_PHASE_DEFAULT };
    int configure(Vector<String> &conf, ErrorHandler *errh);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    void push(int, Packet *);
    void run_timer(Timer *);

    void set_distribution(unsigned int, const uint32_t[]);
    void set_mtbs(unsigned int);

private:
    enum {TCP = 6, UDP = 17};

    struct FlowTuple {
        uint8_t protocol;
        struct in_addr ip_src;
        struct in_addr ip_dst;
        uint16_t port_src;
        uint16_t port_dst;

    };

    struct FlowData {
        FlowTuple flow_tuple;
        unsigned short flowID;
        uint64_t last_seen_time;
        int port;
    };

    uint32_t *_distrib;
    int64_t *_tokens;
    int _total_ports;
    int _distrib_sum;
    unsigned int _mtbs;
    unsigned int _token_reset_int;
    HashMap<unsigned short, FlowData*> _hshFIdFD;
    Timer _timer;

    void update_tokens(uint32_t length);
    int max_tokens_port();

    static int static_set_distribution(const String&, Element*, void*,
                                       ErrorHandler*);
    static int static_set_mtbs(const String&, Element*, void*, ErrorHandler*);

    static unsigned short crc16(char *data_p, unsigned short length);

};

CLICK_ENDDECLS
#endif
