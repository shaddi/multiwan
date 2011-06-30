// -*- c-basic-offset: 4 -*-
#ifndef CLICK_CONNECTIONSTATE_HH
#define CLICK_CONNECTIONSTATE_HH

typedef struct ConnectionState {
  uint32_t id;
  double loss_rate;
  double rtt;

  ConnectionState(uint32_t a, double b, double c) {
    id = a;
    loss_rate = b;
    rtt = c;
  }
};

#endif
