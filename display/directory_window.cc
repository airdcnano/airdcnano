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

#include <algorithm>
#include <functional>
#include <display/directory_window.h>
#include <input/manager.h>
#include <utils/lock.h>
#include <core/log.h>
#include <utils/algorithm.h>

namespace display {

DirectoryWindow::DirectoryWindow(display::Type aType, const std::string& aID) :
    m_root(0),
    m_current(0),
    m_dirViewWidth(0),
    m_viewType(DIRSANDFILES),
    m_selectedView(DIRECTORIES),
	m_dirView(new display::ListView()),
	m_fileView(new display::ListView()),
	Window(aID, aType, false)
{
    m_bindings['\t'] = std::bind(&DirectoryWindow::change_focus, this);
    m_bindings[KEY_LEFT] = std::bind(&DirectoryWindow::change_dirview_width, this, -1);
    m_bindings[KEY_RIGHT] = std::bind(&DirectoryWindow::change_dirview_width, this, 1);
    m_bindings['\n'] = std::bind(&DirectoryWindow::open_item, this);

    m_dirViewWidth = std::max(DIRVIEW_DEFAULTSIZE, get_width()/3);
    m_dirView->insert_column(new display::Column("Name", m_dirViewWidth, m_dirViewWidth, m_dirViewWidth));
    m_dirView->set_draw_title(false);

    m_fileView->insert_column(new display::Column("Name", 10, 20, 30));
    m_fileView->set_draw_title(false);
    resize();
}

void DirectoryWindow::change_dirview_width(int width)
{
    if(m_viewType == DIRSANDFILES &&
       m_dirViewWidth+width > DIRVIEW_MINSIZE &&
       m_dirViewWidth+width < get_width()-FILEVIEW_MINSIZE)
    {
        m_dirViewWidth += width;
        resize();
//        m_windowupdated(this);
    }
}

void DirectoryWindow::open_item()
{
    
}

void DirectoryWindow::change_focus()
{
    if(m_viewType == DIRSANDFILES) {
        if(m_selectedView == DIRECTORIES) {
            m_selectedView = FILES;
        }
        else {
            m_selectedView = DIRECTORIES;
        }
    }
}

void DirectoryWindow::handle(int key)
{
    if(m_bindings.find(key) != m_bindings.end()) {
        m_bindings[key]();
        return;
    }

    if(key == '\t') {
        /* change focus */
        if(m_viewType == DIRSANDFILES) {
            if(m_selectedView == DIRECTORIES) {
                m_selectedView = FILES;
            }
            else {
                m_selectedView = DIRECTORIES;
            }
        }
    }
    else if(m_selectedView == DIRECTORIES) {
        m_dirView->handle(key);
    }
    else if(m_selectedView == FILES) {
        m_fileView->handle(key);
    }
    else {
        throw std::logic_error("DirectoryWindow::handle");
    }
}

void DirectoryWindow::set_current(Directory *directory)
{
    m_current = directory;
//    m_windowupdated(this);
}

void DirectoryWindow::resize()
{
    Window::resize();
    switch(m_viewType) {
        case DIRECTORIES:
            m_dirView->CursesWindow::resize(0, 1, get_width(), get_height());
            break;
        case FILES:
            m_fileView->CursesWindow::resize(0, 1, get_width(), get_height());
            break;
        case DIRSANDFILES:
            try {
                m_dirView->CursesWindow::resize(0, 1, m_dirViewWidth, get_height());
                m_fileView->CursesWindow::resize(m_dirViewWidth+1, 1, get_width()-m_dirViewWidth-1, get_height());
            } catch(std::exception &e) {
                m_dirViewWidth = std::max(DIRVIEW_DEFAULTSIZE, get_width()/3);
                m_dirView->CursesWindow::resize(0, 1, m_dirViewWidth, get_height());
                m_fileView->CursesWindow::resize(m_dirViewWidth+1, 1, get_width()-m_dirViewWidth-1, get_height());
            }
            break;
        default:
            throw std::logic_error("DirectoryWindow::resize");
    }
}

void DirectoryWindow::redraw()
{
    CursesWindow::clear();
    switch(m_viewType) {
        case DIRECTORIES:
            m_dirView->redraw();
            break;
        case FILES:
            m_fileView->redraw();
            break;
        case DIRSANDFILES:
            m_dirView->redraw();
            m_fileView->redraw();
            break;
        default:
            throw std::logic_error("DirectoryWindow::redraw");
    }
}

DirectoryWindow::~DirectoryWindow()
{
    delete m_root;
    delete m_dirView;
    delete m_fileView;
}

} // namespace display
