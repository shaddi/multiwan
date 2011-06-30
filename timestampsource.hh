#ifndef CLICK_TIMESTAMPSOURCE_HH
#define CLICK_TIMESTAMPSOURCE_HH
#include <click/element.hh>
#include <click/timer.hh>
CLICK_DECLS

//#define CLICK_TIMESTAMPSOURCE_DEBUG

/*
=c

TimestampSource(INTERVAL)

=s local

Creates packet with a timestamp in it

=d

Every INTERVAL seconds it will push out a packet on port 0 with time
now in milliseconds.

Keyword arguments:

=item INTERVAL

Length of time in seconds the element will wait at the minimum to push
out the packet
 */

class TimestampSource : public Element { public:

  TimestampSource();
  ~TimestampSource();

  const char *class_name() const { return "TimestampSource"; }
  const char *port_count() const { return PORTS_0_1; }
  const char *processing() const { return PUSH; }

  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void cleanup(CleanupStage);
  void add_handlers();

  void run_timer(Timer *);
  void set_interval(Timestamp);

private:

  Timer _timer;
  Timestamp _interval;

  static int static_set_interval(const String &, Element *, void *, ErrorHandler *);
};

CLICK_ENDDECLS
#endif

