#ifndef CLICK_EMACOUNTER_HH
#define CLICK_EMACOUNTER_HH
#include <click/element.hh>
#include <click/ewma.hh>
#include <click/atomic.hh>
#include <click/timer.hh>
CLICK_DECLS

/*
 * =c
 * EWMACounter([IGNORE])
 * =s counters
 * measures historical packet count using 64bit counters if available and tracks
 * the exponential moving averages for the packet and byte rates
 * =d
 *
 * Passes packets unchanged from its input to its
 * output, maintaining statistics information about
 * packet count and packet rate using an exponential moving average.
 *
 * =h count read-only
 * Returns the number of packets that have passed through since the last reset.
 *
 * =h byte_count read-only
 * Returns the number of packets that have passed through since the last reset.
 *
 * =h rate read-only
 * Returns packet arrival exponential moving average rate.
 *
 * =h byte_rate read-only
 * Returns packet arrival exponential moving average rate in bytes per second.  64bit architectures
 * are less susceptible to overlfow
 *
 * =h reset write-only
 * Resets the count and rate to zero.
 */

class EWMACounter : public Element { public:
#if HAVE_INT64_TYPES
    typedef uint64_t ucounter_t;
    typedef int64_t counter_t;
#else
    typedef int32_t ucounter_t;
    typedef int32_t counter_t;
#endif

    EWMACounter();
    ~EWMACounter();

    const char *class_name() const              { return "EWMACounter"; }
    const char *port_count() const              { return PORTS_1_1; }
    int         configure(Vector<String> &, ErrorHandler *);
    ucounter_t  count() const                      { return _count; }
    ucounter_t  byte_count() const                 { return _byte_count; }
    void        reset();
    int         initialize(ErrorHandler *);
    void        add_handlers();
    Packet*     simple_action(Packet *);


    static void   timer_call(Timer *, void *);
    static String emacounter_read_rate_handler(Element *e, void *thunk);
    static String emacounter_read_count_handler(Element *e, void *thunk);

  private:
    ucounter_t     _count;
    ucounter_t     _byte_count;
    ucounter_t     _prev_count;
    ucounter_t     _prev_byte_count;
    Spinlock       _lock;
    Timer          _timer;
    bool           _first_pkt;
    bool           _first_timer;

#if HAVE_INT64_TYPES
    DirectEWMAX<FixedEWMAXParameters<2, 10, uint64_t, int64_t> > _count_ewma;
    DirectEWMAX<FixedEWMAXParameters<2, 10, uint64_t, int64_t> > _byte_ewma;
#else
    DirectEWMAX<FixedEWMAXParameters<2, 10, uint32_t, int32_t> > _count_ewma;
    DirectEWMAX<FixedEWMAXParameters<2, 10, uint32_t, int32_t> > _byte_ewma;
#endif
};

CLICK_ENDDECLS
#endif
