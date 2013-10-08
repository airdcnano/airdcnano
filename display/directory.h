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

#ifndef _DISPLAYDIRECTORY_H_
#define _DISPLAYDIRECTORY_H_

namespace display
{

class Directory {
public:
    Directory(Directory* parent): m_parent(parent) { }

    std::vector<Directory*> &get_children() { return m_children; }
    std::vector<Item*> &get_items() { return m_items; }
    void append_child(Item *item) { m_items.push_back(item); }

    Directory *get_parent() const { return m_parent; }

    std::string get_name() const { return m_name; }
    void set_name(const std::string &name) { m_name = name; }

    ~Directory() {
        std::for_each(m_children.begin(), m_children.end(), utils::delete_functor<Directory>());
        std::for_each(m_items.begin(), m_items.end(), utils::delete_functor<Item>());
    }
private:
    std::string m_name; //! Name of this directory
    Directory *m_parent; //! Parent directory. NULL if this is root directory.
    std::vector<Directory*> m_children;
    std::vector<Item*> m_items;
};

} // namespace display

#endif // _DISPLAYDIRECTORY_H_

