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

#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#include <utils/stacktrace.h>
#include <stdexcept>
#include <typeinfo>
#include <string>

namespace utils {

/** Simple singleton class. */
template<typename T1>
class Instance {
public:
    /** Constructor. */
    Instance() { }

    /** Explicitly create the unique instance of the singleton.
     * @throw std::logic_error If the instance has already been created. */
    static T1* create() {
        if(m_instance) {
            std::string name = typeid(T1).name();
            #if USE_STACKTRACE
            name = cow::StackTrace::demangle(name);
            #endif
            throw std::logic_error(std::string("Instance of ") + name + " already created");
        }

        m_instance = new T1();
        return m_instance;
    }

    /** Get the instance of the object. Creates the instance if
     * it's not created yet.
     * @throw std::logic_error If the instance is not created. */
    static T1* get() {
        if(!m_instance) {
            m_instance = new T1();
        }
        return m_instance;
    }

    /** Check if the instance is created.
     * @return true if the instance is created */
    static bool is_created() { return m_instance; }

    /** Destroy the instance of the object. */
    static void destroy() {
        delete m_instance;
        m_instance = 0;
    }

    /** Destructor. */
    virtual ~Instance() { }
protected:
    static T1* m_instance; //!< Stores the unique singleton instance
private:
    Instance(const Instance&);
    Instance& operator=(const Instance&);

};

/** Define the unique instance. */
template<class T1> T1* Instance<T1>::m_instance = 0;

} // namespace utils

#endif // _INSTANCE_H_

