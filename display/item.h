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

#ifndef _ITEM_H_
#define _ITEM_H_

#include <string>

namespace display {

class Item
{
public:
    /** Get the value of the item */
    virtual std::string get_value() = 0;

    /** For more info about the item call this.
     * @param line The number of the line you want (1-4) */
    virtual std::string get_line(unsigned int line) { return std::string(); }

    /** Used with std::sort. */
    virtual bool compare(const Item &item) const { return false; }

    /** Returns true if the name of the item matches \c name. */
    bool operator ==(const std::string &name) { return m_name == name; }

    /** Set the items name. */
    void set_name(const std::string &name) { m_name = name; }

    /** Get the items name. */
    std::string get_name() const { return m_name; }

    /** Destructor. */
    virtual ~Item() { }
protected:
    std::string m_name; //!< Unique name of this item
};

} // namespace display

#endif // _ITEM_H_
