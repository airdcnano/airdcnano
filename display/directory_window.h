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

#ifndef _DIRECTORYWINDOW_H_
#define _DIRECTORYWINDOW_H_

#include <string>
#include <stdexcept>
#include <vector>
#include <utils/utils.h>
#include <utils/mutex.h>
#include <display/listview.h>
#include <display/window.h>
#include <display/item.h>
#include <display/directory.h>

namespace display {

/** The default width of directory view. */
const unsigned int DIRVIEW_DEFAULTSIZE = 25;
/** The minimum size of directory view. */
const unsigned int DIRVIEW_MINSIZE = 5;
/** The minimum size of file list view. */
const unsigned int FILEVIEW_MINSIZE = 5;

/** A directory tree window. */
class DirectoryWindow:
    public display::Window
{
public:
    enum View { DIRECTORIES, FILES, DIRSANDFILES };

    /** Default constructor. */
    DirectoryWindow(display::Type aType, const std::string& aID);

    virtual void handle(int key);

    /** Redraw the window. */
    virtual void redraw();

    virtual void resize();

    virtual void create_list() = 0;

    void open_item();

    void set_current(Directory *directory);

    /** Change the width of the directory view. The new width is currentWidth+width */
    void change_dirview_width(int width);

    /** If m_viewType is DIRSANDFILES, changes focus between directory list and file list. */
    void change_focus();

    /** Destructor. */
    virtual ~DirectoryWindow();

    Directory *m_root;
    Directory *m_current;
	virtual void complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_, bool& appendSpace_) {}
protected:
    /** The width of the directory view (if m_viewType is DIRSANDFILES) */
    unsigned int m_dirViewWidth;
    /** Whether to show directory list, file list or both on the screen */
    View m_viewType;
    /** Used when m_viewType is DIRSANDFILES. Specifies which
      * window receives the presses. */
    View m_selectedView;
    display::ListView *m_dirView;
    display::ListView *m_fileView;
};

} // namespace display

#endif // _DIRECTORYWINDOW_H_
