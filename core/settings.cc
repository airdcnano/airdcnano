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

#include <core/settings.h>
#include <utils/utils.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>

namespace core {

void Settings::read(const std::string &filename)
{
    this->set_filename(filename);
    this->load();

    if(!this->exists("save_on_edit"))
        this->set("save_on_edit", true);

    this->set_autosave(this->find_bool("save_on_edit", false));
}

void Settings::properties_changed()
{
    m_listeners();
}

} // namespace core

