// -*- c-basic-offset: 4 -*-
#ifndef CLICK_ADDMWANHEADER_HH
#define CLICK_ADDMWANHEADER_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

//#define CLICK_ADDMWANHEADER_DEBUG

/*
  =c

  AddMWanHeader(DEFAULT_BW)

  =s local

  Adds a multiwan header to the front of the packet

  =d

  Adds a multiwan header to the front of the packet. The header is as follows:

  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                 Timestamp in microseconds                   |
  |                                                             |
  |-------------------------------------------------------------|
  |          Bandwidth of this line in Bytes per second         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Keyword arguemtsn:

  =item DEFAULT_BW

  The default bandwidth value in B/s.
*/

class AddMWanHeader : public Element {
public:
    AddMWanHeader();
    ~AddMWanHeader();

    const char *class_name() const { return "AddMWanHeader"; }
    const char *port_count() const { return PORTS_1_1; }
    const char *processing() const { return AGNOSTIC; }
  
    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();
    void cleanup(CleanupStage);

    Packet *simple_action(Packet*);

private:
    uint32_t _default_BW;

    uint32_t get_bandwidth();
};

CLICK_ENDDECLS
#endif
