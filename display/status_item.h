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

#include <boost/signals2.hpp>

namespace display {

/** Represents a single field in a status bar. */
class StatusItem : boost::noncopyable
{
public:
    /** Constructor. */
	StatusItem(const std::string& aID);

    /** Update the content of the item. */
    virtual void update() = 0;

    /** Get the content of the item. */
	const std::string& get_text() const;

    /** Get the name of the item.  */
    const std::string& get_name() const;

    /** Destructor. */
    virtual ~StatusItem() { }

	void callAsync(std::function<void()> aF);
protected:
    bool m_visible = true; //!< Is this item visible

    const std::string m_id; //!< The name of this status item
    std::string m_text; //!< The content of this item

private:
	void handleAsync();
	boost::signals2::scoped_connection asyncConn;
};

class ProgressUpdater : public StatusItem {
public:
	ProgressUpdater(const std::string& aID);
	void update() { }
	void updateStatus(double percent);

	~ProgressUpdater();
private:
	bool visible = false;
};

} // namespace display

#endif // _STATUSITEM_H_
