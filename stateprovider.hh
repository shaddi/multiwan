// -*- c-basic-offset: 4 -*-
#ifndef CLICK_STATEPROVIDER_HH
#define CLICK_STATEPROVIDER_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

class StateProvider { public:
    StateProvider() {}
  ~StateProvider() {}

    double get_state() { return 0.0; };
};

CLICK_ENDDECLS
#endif
