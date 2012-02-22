/*
Programmer: Roman Chertov
 * Copyright (c) 2012 The Aerospace Corporation

 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "ewmacounter.hh"
#include <click/args.hh>
#include <click/straccum.hh>
#include <click/sync.hh>
#include <click/glue.hh>
#include <click/error.hh>
CLICK_DECLS

EWMACounter::EWMACounter()
{
    reset();
}

EWMACounter::~EWMACounter()
{
}

void EWMACounter::reset()
{
    // no need to use a lock as the handler is invoked in a synced manner (i.e. simple action can't be called)
    _count = 0;
    _byte_count = 0;
    _prev_count = 0;
    _prev_byte_count = 0;
    _count_ewma.clear();
    _byte_ewma.clear();
    _first_pkt = true;
    _first_timer = true;
}

int EWMACounter::configure(Vector<String> &, ErrorHandler *)
{
    return 0;
}

int EWMACounter::initialize(ErrorHandler *)
{
    reset();
    _timer.assign(timer_call, this);
    _timer.initialize(this);
    _timer.schedule_after_sec(1);
    return 0;
}

Packet* EWMACounter::simple_action(Packet *p)
{
    _lock.acquire();
    if (_first_pkt)
        _first_pkt = false;
    _count++;
    _byte_count += p->length();
    _lock.release();
    return p;
}

String EWMACounter::emacounter_read_count_handler(Element *e, void *thunk)
{
    EWMACounter *c = (EWMACounter *)e;
    return String(thunk ? c->byte_count() : c->count());
}

String EWMACounter::emacounter_read_rate_handler(Element *e, void *thunk)
{
    EWMACounter *c = (EWMACounter *)e;
    return !thunk ? c->_count_ewma.unparse() : c->_byte_ewma.unparse();
}

void EWMACounter::timer_call(Timer *, void *data)
{
    EWMACounter *c = (EWMACounter *)data;

    c->_lock.acquire();
    //click_chatter("%d %d", c->_count - c->_prev_count, c->_byte_count - c->_prev_byte_count);
    if (c->_first_timer && !c->_first_pkt) // we want 1 second to have elapsed after we saw the very first packet
    {
        c->_count_ewma.assign_unscaled(c->_count - c->_prev_count);
        c->_byte_ewma.assign_unscaled(c->_byte_count - c->_prev_byte_count);
        c->_first_timer = false;
        //click_chatter("%s", c->_count_ewma.unparse().c_str());
    }
    else
    {
        c->_count_ewma.update(c->_count - c->_prev_count);
        c->_byte_ewma.update(c->_byte_count - c->_prev_byte_count);
    }
    c->_prev_count = c->_count;
    c->_prev_byte_count = c->_byte_count;
    c->_timer.schedule_after_sec(1);
    c->_lock.release();
}

static int emacounter_reset_write_handler(const String &, Element *e, void *, ErrorHandler *)
{
    EWMACounter *c = (EWMACounter *)e;
    c->reset();
    return 0;
}

void EWMACounter::add_handlers()
{
  add_read_handler("count", emacounter_read_count_handler, 0);
  add_read_handler("byte_count", emacounter_read_count_handler, 1);
  add_read_handler("rate", emacounter_read_rate_handler, 0);
  add_read_handler("byte_rate", emacounter_read_rate_handler, 1);
  add_write_handler("reset", emacounter_reset_write_handler, 0, Handler::BUTTON);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(EWMACounter)
ELEMENT_MT_SAFE(EWMACounter)
