// -*- c-basic-offset: 4 -*-
#include <click/config.h>
#include "processmwanheader3.hh"
#include <click/error.hh>
#include <click/confparse.hh>
#include <click/packet_anno.hh>
CLICK_DECLS

ProcessMWanHeader3::ProcessMWanHeader3()
    : _elem_fs(0), _elem_fas(0), _timer(this), _max_paint(0), _update_int(0),
      _cong_deltas(0), _cong_seq_nums(0), _cong_seq_nums_last_update(0),
      _cong_scores(0), _distrib(0), _distrib_total(100), _distrib_inc(5),
      _distrib_min(5), _flowsplit_num(FLOWSPLIT_NUM),
      _flowsplit_threshold(FLOWSPLIT_THRESHOLD), _c_uniform_cong(0),
      _c_cong_mask(0), _mtbs(-1)
{
}

ProcessMWanHeader3::~ProcessMWanHeader3()
{
}

int
ProcessMWanHeader3::configure(Vector<String> &conf, ErrorHandler *errh)
{
    Element *fs_element = 0;
    Element *fas_element = 0;
    unsigned int tmp = 0;
    unsigned short tmp2 = 0;
    if (cp_va_kparse(conf, this, errh,
		     "FLARESWITCH", cpkM, cpElement, &fs_element,
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

    FlareSwitch *fs = 0;
    if (fs_element && 
        !(fs = (FlareSwitch *)(fs_element->cast("FlareSwitch"))))
	return errh->error("FLARESWITCH must be a FlareSwitch element");
    else if (fs)
	_elem_fs = fs;

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
    _cong_seq_nums = new unsigned short[_max_paint];
    _cong_seq_nums_last_update = new unsigned short[_max_paint];
    _cong_scores = new unsigned short[_max_paint];
    _distrib = new uint32_t[_max_paint];

    for (unsigned int i = 0; i < _max_paint; i++) {
        _cong_deltas[i] = 0;
        _cong_seq_nums[i] = 0;
        _cong_seq_nums_last_update[i] = 0;
        _cong_scores[i] = 0;
        _distrib[i] = _distrib_total/_max_paint;
    }

#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
    click_chatter("Done with configure");
#endif

    return 0;
}

int
ProcessMWanHeader3::initialize(ErrorHandler *)
{
    _timer.initialize(this);
    _timer.schedule_now();
    return 0;
}

void
ProcessMWanHeader3::add_handlers()
{
}

void
ProcessMWanHeader3::cleanup(CleanupStage)
{
    delete[] _cong_deltas;
    delete[] _cong_seq_nums;
    delete[] _cong_seq_nums_last_update;
    delete[] _cong_scores;
    delete[] _distrib;
}

Packet *
ProcessMWanHeader3::simple_action(Packet *p)
{
#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
    //    click_chatter("A packet!");
#endif

    int paint = PAINT_ANNO(p);
    unsigned short cong_delta = *((unsigned short*) (p->data()+8));
    unsigned short cong_seq_num = *((unsigned short*) (p->data()+8+2));

#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
    //    click_chatter("It says the congestion delta is %x for line %d", cong_delta, paint);
    //    click_chatter("[PROCESSMWANHEADER3] paint %d seq_num %u ", paint, cong_seq_num);
#endif

    _cong_deltas[paint] = cong_delta;
    _cong_seq_nums[paint] = cong_seq_num;
    _mtbs = *((unsigned short*) (p->data()+(8+2+2)));

    return p;
}

void
ProcessMWanHeader3::run_timer(Timer *timer)
{
    assert(timer == &_timer);

    bool update = true;

    // for (int i = 0; i < _max_paint; i++) {
    //     if (_elem_ds->get_pkt_count(i) < MAX_CONG_SCORE)
    //         update = false;
    //     _elem_ds->reset_pkt_count(i);
    // }

#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
    // click_chatter("[PROCESSMWANHEADER3] UPDATE %s", update ?
    //               "YES -------------------------------------" :
    //               " NO |||||||||||||||||||||||||||||||||||||");
#endif

    if (update) {
        update_cong_scores();

        update_distribution();

        _elem_fs->set_distribution(_max_paint, _distrib);
        if (_mtbs < -1) // This is a cheat, since default is set by FlareSwitch
            _elem_fs->set_mtbs(_mtbs);

        if (_elem_fas)
            bump_flows_if_need();
    }

#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
        bool badness = false;
        char buffer[256];
        char buffer2[256];
        sprintf(buffer, "");
        sprintf(buffer2, "");
        for (unsigned int i = 0; i < _max_paint; i++) {
            if ((_distrib[i] < _distrib_min) || (_distrib[i] > _distrib_total))
                badness = true;

            char tmp[100];
            sprintf(tmp, "%s", buffer);
            char tmp2[100];
            sprintf(tmp2, "%s", buffer2);

            sprintf(buffer, "%s %u", tmp, _cong_scores[i]);
            sprintf(buffer2, "%s %u", tmp2, _distrib[i]);
        }

        // click_chatter("[PROCESSMWANHEADER3] cong scores:  %s", buffer);
        click_chatter("[PROCESSMWANHEADER3] distribution: %s", buffer2);

        if (badness)
            click_chatter("[PROCESSMWANHEADER3] Some of distribution is < min!");
#endif

    _timer.reschedule_after_msec(_update_int);
}

void
ProcessMWanHeader3::update_cong_scores()
{
#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
    char buffer[256];
    sprintf(buffer, "");
    char buffer2[256];
    sprintf(buffer2, "");
#endif

    uint32_t min = MAX_CONG_SCORE;
    for (unsigned int i = 0; i < _max_paint; i++) {
        uint32_t tmp = _cong_seq_nums[i] - _cong_seq_nums_last_update[i];
        _cong_seq_nums_last_update[i] = _cong_seq_nums[i];
        if (tmp < min)
            min = tmp;
#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
        char tmp2[256];
        sprintf(tmp2, "%s", buffer2);
        sprintf(buffer2, "%s %u", tmp2, tmp);
#endif
    }

    unsigned short *cong_scores = new unsigned short[_max_paint];
    for (unsigned int i = 0; i < _max_paint; i++) {
        cong_scores[i] = static_count_ones(_cong_deltas[i], min);
#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
        char tmp[256];
        sprintf(tmp, "%s", buffer);
        sprintf(buffer, "%s %x:%u", tmp, _cong_deltas[i], cong_scores[i]);
#endif
    }

#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
    click_chatter("[PROCESSMWANHEADER3] congestion (cMask %u [%s]): %s",
                  min, buffer2, buffer);
#endif

    unsigned short *tmp = _cong_scores;
    _cong_scores = cong_scores;
    _c_cong_mask = min;
    delete[] tmp;
}

// update_cong_scores() needs to be called first.
void
ProcessMWanHeader3::update_distribution()
{
#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
    //    click_chatter("[PROCESSMWANHEADER3] Updating distribution");
#endif

    uint32_t sum = 0;
    int c_not_cong = 0;
    bool b_scores_equal = true;
    uint32_t prev_score = _cong_scores[0];
    for (unsigned int i = 0; i < _max_paint; i++) {
        sum += _cong_scores[i];
        if (_cong_scores[i] == 0)
            c_not_cong++;
        if (prev_score != _cong_scores[i])
            b_scores_equal = false;
    }
    uint32_t avg = sum/_max_paint;
 
    // if (b_scores_equal)
    //     _c_uniform_cong++;
    // else
    //     _c_uniform_cong = 0;

    //    if (_c_uniform_cong >= 2) {
    if (b_scores_equal){// && (_c_cong_mask > 0)) {
        //        _c_uniform_cong = 0;

        uint32_t min = _distrib_total;
        unsigned int min_i = -1;
        uint32_t max = 0;
        unsigned int max_i = -1;
        for (unsigned int i = 0; i < _max_paint; i++) {
            if (min > _distrib[i]) {
                min = _distrib[i];
                min_i = i;
            }
            if (max < _distrib[i]) {
                max = _distrib[i];
                max_i = i;
            }
        }

        uint32_t distrib_shift = get_distrib_shift(max_i);

        _distrib[min_i] += distrib_shift;
        _distrib[max_i] -= distrib_shift;

    } else if (c_not_cong == 0) {
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

#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
            if (add_to < 0)
                click_chatter("[PROCESSMWANHEADER3] FREAK OUT! (add_t < 0) =====================================");
            // else
            //     click_chatter("[PROCESSMWANHEADER3] add_t = %d line %d", add_to,
            //                   below_avg[add_to]);
#endif
            int curr_index = above_avg[i];
            int recv_traffic_index = below_avg[add_to];

             uint32_t distrib_shift = get_distrib_shift(curr_index);

            _distrib[recv_traffic_index] += distrib_shift;
            _distrib[curr_index] -= distrib_shift;
        }

        delete[] above_avg;
        delete[] below_avg;
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

        delete[] non_zero_score;
        delete[] zero_score;
    }
}

/*
  update_cong_scores() needs to be called first. Assumes this is only called
  when there is a FlowAgeSplitter element to call
 */
void
ProcessMWanHeader3::bump_flows_if_need()
{
    uint32_t sum = 0;
    bool bIsCongested = true;
    for (unsigned int i = 0; i < _max_paint; i++) {
        sum += _cong_scores[i];
        if (_cong_scores[i] == 0)
            bIsCongested = false;
    }

    if (bIsCongested && (sum/_max_paint >= _flowsplit_threshold)) {
#ifdef CLICK_PROCESSMWANHEADER3_DEBUG
        click_chatter("[PROCESSMWANHEADER3] Bumping %u flows.", _flowsplit_num);
#endif
        _elem_fas->bump_flows(_flowsplit_num);
    }

}

uint32_t
ProcessMWanHeader3::get_distrib_shift(int curr_index)
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
ProcessMWanHeader3::static_count_ones(unsigned short num, uint32_t c_mask)
{
    unsigned short count = 0;
    for(unsigned int i = 0; i < MAX_CONG_SCORE; i++) {
        if ((num & (1 << i)) && (i >= (MAX_CONG_SCORE - c_mask)))
            count++;
    }

    return count;
}

bool
ProcessMWanHeader3::static_ary_equals(int size, int *a, int *b)
{
    for (int i = 0; i < size; i++)
        if (a[i] != b[i])
            return false;

    return true;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ProcessMWanHeader3)
