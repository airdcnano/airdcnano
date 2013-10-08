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

#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <pthread.h>

namespace utils {

/** A wrapper for <code>pthread_mutex_t</code> */
class Mutex {
public:
    /** Initializes the mutex. */
    Mutex() {
        pthread_mutex_init(&m_lock, 0);
    }
    /** Locks the mutex. */
    void lock() {
        pthread_mutex_lock(&m_lock);
    }
    /** Releases the mutex. */
    void unlock() {
        pthread_mutex_unlock(&m_lock);
    }

    /** Get the pthread mutex handle. */
    pthread_mutex_t *get_mutex() {
        return &m_lock;
    }

    /** Deletes the mutex. */
    ~Mutex() {
        pthread_mutex_destroy(&m_lock);
    }
private:
    pthread_mutex_t m_lock;
};

} // namespace utils

#endif // _MUTEX_H_
