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

#ifndef _STATUSITEM_H_
#define _STATUSITEM_H_

#include <string>
#include <utils/mutex.h>

namespace display {

/** Represents a single field in a status bar. */
class StatusItem
{
public:
    /** Constructor. */
    StatusItem(): m_visible(true) { }

    /** Update the content of the item. */
    virtual void update() = 0;

    /** Get the content of the item. */
    virtual std::string get_text() const;

    /** Get the name of the item.  */
    virtual std::string get_name() const;

    /** Destructor. */
    virtual ~StatusItem() { }

protected:
    virtual void set_text(std::string name);
    virtual void set_name(std::string name);

    bool m_visible; //!< Is this item visible

private:
    std::string m_name; //!< The name of this status item
    std::string m_text; //!< The content of this item
    mutable utils::Mutex m_mutex; 
};

class ProgressUpdater : public StatusItem {
public:
	void update() { }
	void updateStatus(double percent);

	~ProgressUpdater();
private:
	bool visible = false;
};

} // namespace display

#endif // _STATUSITEM_H_
