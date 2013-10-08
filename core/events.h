/* vim:set ts=4 sw=4 sts=4 et cindent: */
/*
 * nanodc - The ncurses DC++ client
 * Copyright © 2005-2006 Markus Lindqvist <nanodc.developer@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contributor(s):
 *  
 */

#ifndef _EVENTS_H_
#define _EVENTS_H_

#include <pthread.h>
#include <list>
#include <vector>
#include <functional>
#include <boost/signals2.hpp>
#include <boost/any.hpp>
#include <utils/instance.h>
#include <utils/mutex.h>
#include <string>
#include <boost/lockfree/queue.hpp>
#include <client/Semaphore.h>

namespace events {

typedef std::vector<boost::any> AnyList;
typedef std::function<void ()> EventFunc;
typedef boost::signals2::signal<void ()> EventSig;
typedef std::map<std::string, EventSig*> EventMap;

/** Event priorities. */
enum Priority {
    FIRST = boost::signals2::at_front,
    DEFAULT = 5,
    LAST = boost::signals2::at_back
};

class StopEvent { };

class Manager:
    public utils::Instance<events::Manager>
{
public:
    Manager();

    /** Create an event. Events can't be emitted before they are created.
     * @throw std::logic_error If the event already exists. */
    void create_event(const std::string &event) throw(std::logic_error);

    /** Add a listener for a specific event.
     * @throw std::logic_error If event is not created. */
    boost::signals2::connection add_listener(const std::string &event,
		const EventFunc &func, Priority priority = DEFAULT)
        throw(std::logic_error);

    /** Emit the event named \c event. */
    void emit(const std::string &event, boost::any a1=boost::any(),
            boost::any a2=boost::any(), boost::any a3=boost::any(),
            boost::any a4=boost::any(), boost::any a5=boost::any(),
            boost::any a6=boost::any(), boost::any a7=boost::any());

    /** Get the nth argument of current event. */
    template <class T>
	T arg(unsigned int n) { return boost::any_cast<T>((*m_args)[n]); }

    /** Return a reference to nth argument. */
    boost::any& arg(unsigned int n) { return (*m_args)[n]; }

    /** Return the number of arguments. */
    unsigned int args() { return m_args->size(); }

    /** Stop handling current event. */
    void stop() { throw StopEvent(); }

    /** Handle events until quit() is called. */
    void main_loop();

    /** Stop the main_loop after the current event is processed. */
    void quit() { m_running = false; }

    ~Manager();
private:
    EventMap m_events;
    //std::vector<std::pair<EventSig*, AnyList> > m_queue;
    //utils::Mutex m_queueMutex;

	struct Callback {
		EventSig* sig;
		AnyList args;
	};

	boost::lockfree::queue<Callback*> tasks;

    AnyList* m_args = nullptr;
    bool m_running = true;
    //pthread_cond_t m_queueCond;
	dcpp::Semaphore s;
};

inline
void stop()
{
    events::Manager::get()->stop();
}

inline
boost::signals2::connection add_listener(const std::string &event, const EventFunc& func)
{
    return events::Manager::get()->add_listener(event, func);
}

inline
boost::signals2::connection add_listener_last(const std::string &event, const EventFunc& func)
{
    return events::Manager::get()->add_listener(event, func, LAST);
}

inline
boost::signals2::connection add_listener_first(const std::string &event, const EventFunc& func)
{
    return events::Manager::get()->add_listener(event, func, FIRST);
}

inline
void emit(const std::string &event, boost::any a1=boost::any(),
        boost::any a2=boost::any(), boost::any a3=boost::any(),
        boost::any a4=boost::any(), boost::any a5=boost::any(),
        boost::any a6=boost::any(), boost::any a7=boost::any())
{
    events::Manager::get()->emit(event, a1, a2, a3, a4, a5, a6, a7);
}

template <class T>
T arg(unsigned int n) {
    return events::Manager::get()->arg<T>(n);
}

inline
boost::any& arg(unsigned int n) {
    return events::Manager::get()->arg(n);
}

inline
unsigned int args() {
    return events::Manager::get()->args();
}

inline
void create(const std::string &event)
{
    events::Manager::get()->create_event(event);
}

}

#endif // _EVENTS_H_

