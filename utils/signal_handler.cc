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

#include <stdexcept>
#include <utils/signal_handler.h>

#ifndef __SIGRTMIN
#define __SIGRTMIN 32
#endif

namespace utils {

std::map<uint32_t, SignalHandler::SigHandler> SignalHandler::m_handlers;

void SignalHandler::ignore(uint32_t sig)
{
    if(sig < 0 || sig >= __SIGRTMIN)
        throw std::logic_error("SignalHandler::ignore(): Invalid signal");

    signal(sig, SIG_IGN);
    m_handlers.erase(sig);
}

void SignalHandler::add_handler(uint32_t sig, SigHandler handler)
{
    if(sig < 0 || sig >= __SIGRTMIN)
        throw std::logic_error("SignalHandler::handle(): Invalid signal");

    /* handle all signals in handle() and
     * call the right signal handler from there */
    signal(sig, &SignalHandler::handle);

    m_handlers[sig] = handler;
}

void SignalHandler::handle(int signal)
{
    if(m_handlers.find(signal) != m_handlers.end())
        m_handlers[signal](signal);
}

const std::string SignalHandler::to_string(uint32_t sig)
{
    switch(sig) {
        case SIGHUP:
            return "Hangup detected";
        case SIGINT:
            return "Interrupt from keyboard";
        case SIGQUIT:
            return "Quit signal";
        case SIGILL:
            return "Illegal instruction";
        case SIGABRT:
            return "Abort signal";
        case SIGFPE:
            return "Floating point exception";
        case SIGKILL:
            return "Kill signal";
        case SIGSEGV:
            return "Invalid memory reference";
        case SIGPIPE:
            return "Broken pipe";
        case SIGALRM:
            return "Timer signal";
        case SIGTERM:
            return "Termination signal";
        case SIGBUS:
            return "Bus error";
        default:
            return "Other signal";
    }
}

} // namespace utils
