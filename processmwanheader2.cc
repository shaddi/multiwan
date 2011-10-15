// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "processmwanheader2.hh"
#include <click/error.hh>
#include <click/confparse.hh>
#include <click/packet_anno.hh>
CLICK_DECLS

ProcessMWanHeader2::ProcessMWanHeader2()
    : _elem_ds(0), _elem_fas(0), _timer(this), _max_paint(0), _update_int(0),
      _cong_deltas(0), _cong_scores(0), _distrib(0), _distrib_total(100),
      _distrib_inc(5), _distrib_min(5), _flowsplit_num(FLOWSPLIT_NUM),
      _flowsplit_threshold(FLOWSPLIT_THRESHOLD)
{
}

ProcessMWanHeader2::~ProcessMWanHeader2()
{
}

int
ProcessMWanHeader2::configure(Vector<String> &conf, ErrorHandler *errh)
{
    Element *ds_element = 0;
    Element *fas_element = 0;
    unsigned int tmp = 0;
    unsigned short tmp2 = 0;
    if (cp_va_kparse(conf, this, errh,
		     "DISTROSWITCH", cpkM, cpElement, &ds_element,
		     "MAX_PAINT", cpkM, cpUnsigned, &_max_paint,
		     "UPDATE_INT", cpkM, cpUnsigned, &_update_int,
		     "DISTRIB_TOTAL", cpkM, cpUnsigned, &_distrib_total,
		     "DISTRIB_INC", cpkM, cpUnsigned, &_distrib_inc,
		     "DISTRIB_MIN", cpkM, cpUnsigned, &_distrib_min,
		     "FLOWAGESPLITTER", cpkN, cpElement, &fas_element,
		     "FLOWSPLIT_NUM", cpkN, cpUnsigned, &tmp,
		     "FLOWSPLIT_THRESHOLD", cpkN, cpUnsigned, &tmp2,
		     cpEnd) < 0)
	return -1;

    DistroSwitch *ds = 0;
    if (ds_element && 
        !(ds = (DistroSwitch *)(ds_element->cast("DistroSwitch"))))
	return errh->error("DISTROSWITCH must be a DistroSwitch element");
    else if (ds)
	_elem_ds = ds;

    FlowAgeSplitter *fas = 0;
    if (fas_element && 
        !(fas = (FlowAgeSplitter *)(fas_element->cast("FlowAgeSplitter"))))
	return errh->error("FLOWAGESPLITTER must be a FlowAgeSplitter element");
    else if (fas)
	_elem_fas = fas;
    else
        _elem_fas = 0;

    if (tmp > 0)
        _flowsplit_num = tmp;
    if (tmp2 > 0)
        _flowsplit_threshold = tmp2;

    _cong_deltas = new unsigned short[_max_paint];
    _cong_scores = new unsigned short[_max_paint];
    _distrib = new uint32_t[_max_paint];

    for (unsigned int i = 0; i < _max_paint; i++) {
        _cong_deltas[i] = 0;
        _cong_scores[i] = 0;
        _distrib[i] = _distrib_total/_max_paint;
    }

#ifdef CLICK_PROCESSMWANHEADER2_DEBUG
    click_chatter("Done with configure");
#endif

    return 0;
}

int
ProcessMWanHeader2::initialize(ErrorHandler *)
{
    _timer.initialize(this);
    _timer.schedule_now();
    return 0;
}

void
ProcessMWanHeader2::add_handlers()
{
}

void
ProcessMWanHeader2::cleanup(CleanupStage)
{
    delete _cong_deltas;
    delete _cong_scores;
    delete _distrib;
}

Packet *
ProcessMWanHeader2::simple_action(Packet *p)
{
#ifdef CLICK_PROCESSMWANHEADER2_DEBUG
    //    click_chatter("A packet!");
#endif

    int paint = PAINT_ANNO(p);
    unsigned short cong_delta = *((unsigned short*) (p->data()+8));

#ifdef CLICK_PROCESSMWANHEADER2_DEBUG
    //    click_chatter("It says the congestion delta is %x for line %d", cong_delta, paint);
#endif

    _cong_deltas[paint] = cong_delta;

    return p;
}

void
ProcessMWanHeader2::run_timer(Timer *timer)
{
    assert(timer == &_timer);

    update_cong_scores();

    update_distribution();

#ifdef CLICK_PROCESSMWANHEADER2_DEBUG
    bool badness = false;
    for (unsigned int i = 0; i < _max_paint; i++)
        if (_distrib[i] < _distrib_min)
            badness = true;
    if (badness)
        click_chatter("[PROCESSMWANHEADER2] Some of distribution is < min!");
#endif

    _elem_ds->set_distribution(_max_paint, _distrib);

    if (_elem_fas)
        bump_flows_if_need();

    _timer.reschedule_after_msec(_update_int);
}

void
ProcessMWanHeader2::update_cong_scores()
{
    unsigned short *cong_scores = new unsigned short[_max_paint];
    for (unsigned int i = 0; i < _max_paint; i++)
        cong_scores[i] = static_count_ones(_cong_deltas[i]);

    unsigned short *tmp = _cong_scores;
    _cong_scores = cong_scores;
    delete tmp;
}

// update_cong_scores() needs to be called first.
void
ProcessMWanHeader2::update_distribution()
{
#ifdef CLICK_PROCESSMWANHEADER2_DEBUG
    //    click_chatter("[PROCESSMWANHEADER2] Updating distribution");
#endif

    uint32_t sum = 0;
    int c_not_cong = 0;
    for (unsigned int i = 0; i < _max_paint; i++) {
        sum += _cong_scores[i];
        if (_cong_scores[i] == 0)
            c_not_cong++;
    }
    uint32_t avg = sum/_max_paint;
 
    if (c_not_cong == 0) {
        unsigned int *above_avg = new unsigned int[_max_paint];
        unsigned int *below_avg = new unsigned int[_max_paint];
        int c_above = 0;
        int c_below = 0;
        uint32_t sum = 0;

        for (unsigned int i = 0; i < _max_paint; i++) {
            if (_cong_scores[i] > avg) {
                above_avg[c_above] = i;
                c_above++;
            } else {
                below_avg[c_below] = i;
                c_below++;
                sum += MAX_CONG_SCORE - _cong_scores[i];
            }
        }

        for (int i = 0; i < c_above; i++) {
            uint32_t r = click_random(1, sum);
            uint32_t sum2 = 0;
            int add_to = -1;
            for (int j = 0; j < c_below; j++) {
                sum2 += MAX_CONG_SCORE - _cong_scores[below_avg[j]];
                if (r <= sum2) {
                    add_to = j;
                    break;
                }
            }

            int curr_index = above_avg[i];

            uint32_t distrib_shift = get_distrib_shift(curr_index);

            _distrib[add_to] += distrib_shift;
            _distrib[curr_index] -= distrib_shift;
        }

        delete above_avg;
        delete below_avg;
    } else {
        unsigned int *non_zero_score = new unsigned int[_max_paint];
        unsigned int *zero_score = new unsigned int[_max_paint];
        int c_non_zero = 0;
        int c_zero = 0;

        for (unsigned int i = 0; i < _max_paint; i++) {
            if (_cong_scores[i] > 0) {
                non_zero_score[c_non_zero] = i;
                c_non_zero++;
            } else {
                zero_score[c_zero] = i;
                c_zero++;
            }
        }

        for (int i = 0; i < c_non_zero; i++){
            uint32_t r = click_random(0, c_zero-1);
            int add_to = zero_score[r];

            int curr_index = non_zero_score[i];

            uint32_t distrib_shift = get_distrib_shift(curr_index);

            _distrib[add_to] += distrib_shift;
            _distrib[curr_index] -= distrib_shift;

        }

        delete non_zero_score;
        delete zero_score;
    }
}

/*
  update_cong_scores() needs to be called first. Assumes this is only called
  when there is a FlowAgeSplitter element to call
 */
void
ProcessMWanHeader2::bump_flows_if_need()
{
    uint32_t sum = 0;
    bool bIsCongested = true;
    for (unsigned int i = 0; i < _max_paint; i++) {
        sum += _cong_scores[i];
        if (_cong_scores[i] == 0)
            bIsCongested = false;
    }

    if (bIsCongested && (sum/_max_paint >= _flowsplit_threshold)) {
#ifdef CLICK_PROCESSMWANHEADER2_DEBUG
        click_chatter("[PROCESSMWANHEADER2] Bumping %u flows.", _flowsplit_num);
#endif
        _elem_fas->bump_flows(_flowsplit_num);
    }

}

uint32_t
ProcessMWanHeader2::get_distrib_shift(int curr_index)
{
    uint32_t shift = 0;
    uint32_t distrib = _distrib[curr_index];

    if (distrib == _distrib_min)
        shift = 0;
    else if (distrib < (_distrib_min + _distrib_inc))
        shift = distrib - _distrib_min;
    else
        shift = _distrib_inc;

    return shift;
}

unsigned short
ProcessMWanHeader2::static_count_ones(unsigned short num)
{
    unsigned short count = 0;
    for(int i = 0; i < MAX_CONG_SCORE; i++) {
        if (num & (1 << i))
            count++;
    }

    return count;
}

bool
ProcessMWanHeader2::static_ary_equals(int size, int *a, int *b)
{
    for (int i = 0; i < size; i++)
        if (a[i] != b[i])
            return false;

    return true;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ProcessMWanHeader2)
