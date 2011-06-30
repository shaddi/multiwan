#include <click/config.h>
#include "timestampsource.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>
CLICK_DECLS

TimestampSource::TimestampSource()
  : _timer(this), _interval(0, Timestamp::subsec_per_sec / 2)
{
}

TimestampSource::~TimestampSource()
{
}

int
TimestampSource::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (cp_va_kparse(conf, this, errh,
		   "INTERVAL", cpkP, cpTimestamp, &_interval,
		   cpEnd) < 0)
    return -1;

  return 0;
}

int
TimestampSource::initialize(ErrorHandler *)
{
  _timer.initialize(this);
  _timer.schedule_now();
  return 0;
}

void
TimestampSource::cleanup(CleanupStage)
{
}

void
TimestampSource::add_handlers()
{
  add_write_handler("set_interval_ms", static_set_interval, 0);
}

void
TimestampSource::run_timer(Timer *timer)
{
  assert(timer == &_timer);
  
  //create packet to send
  Timestamp now = Timestamp::now();
  uint64_t now_ms = now.sec() * ((uint64_t)(1000));
  now_ms += now.msec();
#ifdef CLICK_TIMESTAMPSOURCE_DEBUG
  click_chatter("SEND timestamp(%llu)", now_ms);
#endif
  Packet *p = Packet::make(&now_ms, sizeof(now_ms));
  output(0).push(p);

  _timer.reschedule_after(_interval);
}

int
TimestampSource::static_set_interval(const String &s, Element *e, void*, 
				     ErrorHandler *errh)
{
  TimestampSource *ts = (TimestampSource *) e;

  int new_interval;
  if(!cp_integer(s, &new_interval))
    return errh->error("New interval is not a number");

  ts->set_interval(Timestamp::make_msec(new_interval));

  return 0;
}

void
TimestampSource::set_interval(Timestamp interval)
{
  _interval = interval;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(TimestampSource)
