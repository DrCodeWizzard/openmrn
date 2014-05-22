/** \copyright
 * Copyright (c) 2014, Balazs Racz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \file GlobalEventHandlerImpl.hxx
 *
 * Implementation headers shared between components of the global event
 * handler's various flows. An end-user will typically not need to include this
 * header.
 *
 * @author Balazs Racz
 * @date 20 April 2014
 */

#ifndef _NMRANET_EVENTSERVICEIMPL_HXX_
#define _NMRANET_EVENTSERVICEIMPL_HXX_

#include <memory>
#include <vector>

#include "nmranet/EventService.hxx"
#include "nmranet/NMRAnetEventRegistry.hxx"

namespace nmranet
{

class IncomingEventFlow;
class GlobalIdentifyFlow;
class EventHandler;

struct EventHandlerCall
{
    EventReport *rep;
    EventHandler *handler;
    EventHandlerFunction fn;
    void reset(EventReport *rep, EventHandler *handler,
               EventHandlerFunction fn)
    {
        this->rep = rep;
        this->handler = handler;
        this->fn = fn;
    }
};

class EventCallerFlow : public StateFlow<Buffer<EventHandlerCall>, QList<4>>
{
public:
    EventCallerFlow(Service *service)
        : StateFlow<Buffer<EventHandlerCall>, QList<4>>(service) {};

private:
    virtual Action entry() OVERRIDE;
    Action call_done();

    BarrierNotifiable n_;
};

class EventService::Impl
{
public:
    Impl(EventService *service);
    ~Impl();

    /* The implementation of the event registry. */
    std::unique_ptr<EventRegistry> registry;

    /** Flows that we own. There will be a few entries for each interface
     * registered. */
    std::vector<std::unique_ptr<StateFlowWithQueue>> ownedFlows_;

    /** This flow will serialize calls to NMRAnetEventHandler objects. All such
     * calls need to be sent to this flow. */
    EventCallerFlow callerFlow_;

    enum
    {
        // These address/mask should match all the messages carrying an event
        // id.
        MTI_VALUE_EVENT = Defs::MTI_EVENT_MASK,
        MTI_MASK_EVENT = Defs::MTI_EVENT_MASK,
        MTI_VALUE_GLOBAL = Defs::MTI_EVENTS_IDENTIFY_GLOBAL,
        MTI_MASK_GLOBAL = 0xffff,
        MTI_VALUE_ADDRESSED_ALL = Defs::MTI_EVENTS_IDENTIFY_ADDRESSED,
        MTI_MASK_ADDRESSED_ALL = 0xffff,
    };
};

/** Flow to receive incoming messages of event protocol, and dispatch them to
 * the global event handler. This flow runs on the executor of the event
 * service (and not necessarily the interface). Its main job is to iterate
 * through the matching event handler and call each of them for that report. */
class EventIteratorFlow : public IncomingMessageStateFlow
{
public:
    EventIteratorFlow(If *interface, EventService *event_service,
                    unsigned mti_value, unsigned mti_mask);
    ~EventIteratorFlow();

protected:
    Action entry() OVERRIDE;
    Action iterate_next();

private:
    EventService *eventService_;

    // Statically allocated structure for calling the event handlers from the
    // main event queue.
    EventReport eventReport_;

    /** Iterator for generating the event handlers from the registry. */
    EventIterator* iterator_;

    BarrierNotifiable n_;
    EventHandlerFunction fn_;

    If *interface_;
};

} // namespace nmranet

#endif // _NMRANET_GLOBAL_EVENT_HANDLER_IMPL_