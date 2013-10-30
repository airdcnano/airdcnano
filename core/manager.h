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

#ifndef _COREMANAGER_H_
#define _COREMANAGER_H_

#include <utils/instance.h>
#include <string>

namespace core {

/** Starts the program. */
class Manager:
    public utils::Instance<core::Manager>
{
public:

    /** Constructor. Does nothing. */
    Manager();

    /** Starts the nanodc. Starts screen drawing and input subsystems.
     * @return 0 on clean shutdown, nonzero error code otherwise.*/
    int run();

    /** Calls shutdown(). */
    ~Manager();

	bool isInitialized() const { return initialized; }
private:
    /** Creates the user interface. */
    void init_ui();

    /** Initialize the DC++ core */
    void start_client();


    /** Clean up everything. */
    void shutdown();

	bool initialized = false;
};

} // namespace core

#endif // _COREMANAGER_H_

