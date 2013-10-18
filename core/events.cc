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

#include <utils/utils.h>
#include <core/log.h>
#include <core/events.h>

namespace events {

Manager::Manager():
    m_running(true),
	tasks(1024)
{
    //pthread_cond_init(&m_queueCond, NULL);
}

void Manager::main_loop()
{
	std::unique_ptr<Callback> callback;
    do {
        /* wait until there's events to process */
		s.wait();
		if (tasks.pop(callback)) {
			m_args = &callback->args;

			/* stopping the current event works
			* by throwing StopEvent exception */
			try {
				(*callback->sig)();
			} catch (StopEvent &e) {
				/* do nothing.. */
			}
		}
    } while(m_running);
}

void Manager::create_event(const std::string &event)
    throw(std::logic_error)
{
    if(m_events.find(event) != m_events.end())
        throw std::logic_error("Event already exists");

    m_events[event] = new EventSig();
}

boost::signals2::connection
Manager::add_listener(const std::string &event, const EventFunc &func, Priority priority) noexcept {
    if(m_events.find(event) == m_events.end()) {
        m_events[event] = new EventSig();
    }

    return m_events[event]->connect(priority, func);
}

void Manager::emit(const std::string &event, boost::any a1,
        boost::any a2, boost::any a3,
        boost::any a4, boost::any a5,
        boost::any a6, boost::any a7)
{
    if(m_events.find(event) == m_events.end())
        return;

    AnyList args;
    if(!a1.empty())
        args.push_back(a1);
    if(!a2.empty())
        args.push_back(a2);
    if(!a3.empty())
        args.push_back(a3);
    if(!a4.empty())
        args.push_back(a4);
    if(!a5.empty())
        args.push_back(a5);
    if(!a6.empty())
        args.push_back(a6);
    if(!a7.empty())
        args.push_back(a7);

	tasks.push(new Callback({ m_events[event], move(args) }));
	s.signal();
}

Manager::~Manager()
{
    //pthread_cond_destroy(&m_queueCond);
}

} // namespace events

