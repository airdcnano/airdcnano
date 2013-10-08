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

#ifndef _LISTVIEW_H_
#define _LISTVIEW_H_

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <utils/utils.h>
#include <utils/mutex.h>
#include <display/window.h>
#include <display/item.h>

namespace display {

class Column {
public:
    friend class ListView;

    Column(const std::string &name, int minWidth, int preferredWidth, int maxWidth):
        m_name(name), m_rows(), m_minWidth(minWidth),
        m_preferredWidth(preferredWidth),
        m_maxWidth(maxWidth), m_realWidth(0), m_hidden(false)
         { }

    /** Constructs a hidden column. */
    Column(const std::string &name):m_name(name),
        m_minWidth(0), m_preferredWidth(0), m_maxWidth(0),
        m_realWidth(0), m_hidden(true) {  }

    void set_name(const std::string &name) { m_name = name; }
    std::string get_name() const { return m_name; }
    bool is_hidden() const { return m_hidden; }
    void set_real_width(int width) { m_realWidth = width; }
    int get_preferred_width() const { return m_preferredWidth; }
    int get_min_width() const { return m_minWidth; }
    int get_max_width() const { return m_maxWidth; }
    int get_real_width() const { return m_realWidth; }
    void reset_width() { m_realWidth = m_minWidth; }

    void insert_row() { m_rows.push_back(""); }

    int find_row(const std::string &content) {
        auto it = std::find(m_rows.begin(), m_rows.end(), content);
        return std::distance(m_rows.begin(), it);
    }

    void delete_row(int row) { m_rows.erase(m_rows.begin()+row); }

    void set_text(int row, const std::string &text) throw(std::out_of_range) { m_rows.at(row) = text; }
    std::string get_text(int row) { return m_rows.at(row); }

    void clear() { m_rows.clear(); }

    /** Used to calculate the total width of all columns. */
    static int calc_width(int width, Column *column) {
        return width + column->m_realWidth;
    }
private:
    std::string m_name;
    std::vector<std::string> m_rows;
    int m_minWidth;
    int m_preferredWidth;
    int m_maxWidth;
    int m_realWidth;
    bool m_hidden;
};

/** A list window. */
class ListView:
    public display::Window
{
public:
    /** Default constructor. Creates the window with correct size. */
	ListView(display::Type aType = display::TYPE_UNKNOWN, const std::string& aID = "");

    /** Insert a new column. The ListView class is responsible for
     * deleting Column's allocated on the heap.*/
    void insert_column(Column *c) { m_columns.push_back(c); }

    /** Insert a new row.
     * @return The index of the new row. */
    int insert_row();

    /** Find a row by its content. Returns -1 if row is not found. */
    int find_row(int column, const std::string &content) throw(std::out_of_range) {
        int row = m_columns.at(column)->find_row(content);
        return row >= m_rowCount ? -1 : row;
    }

    /** Delete row where \c column contains \c text. */
    void delete_row(int column, const std::string &text);

    void delete_all();

    /** Returns the index of the selected row. */
    int get_selected_row() { return m_currentItem; }

    /** @throw std::out_of_range if column or row is invalid. */
    void set_text(int column, int row, const std::string &text) throw(std::out_of_range);

    void set_text(int column, int row, const char *text) throw(std::out_of_range)
        { set_text(column, row, std::string(text)); }

    void set_text(int column, int row, bool value) throw(std::out_of_range)
        { set_text(column, row, value ? "[x]" : "[ ]"); }

    void set_text(int column, int row, int i) throw(std::out_of_range)
        { set_text(column, row, utils::to_string(i)); }

    std::string get_text(int column, int row) throw(std::out_of_range)
        { return m_columns.at(column)->get_text(row); }

    /** Returns the number of rows in the window. */
    unsigned int get_size() { return m_rowCount; }

    /** Redraw the window. */
    virtual void redraw();

    virtual void resize();

    virtual void handle(wint_t key);

    virtual std::string get_infobox_line(unsigned int n) { return std::string(); }

    /** Get the number of currently selected item.
     * @return index or -1 if nothing is selected */
    int get_current() const { return m_currentItem; }

    /** Scroll the list up or down. */
    void scroll_list(int items);

    /** Destructor. */
    virtual ~ListView();
    typedef std::vector<Column*> Columns;

	void setInsertMode(bool enable);
protected:
	virtual utils::Mutex* getLock() { return nullptr; }
    int m_rowCount;
    Columns m_columns;
    //utils::Mutex m_itemLock;
    int m_currentItem;
    unsigned int m_infoboxHeight;
};

} // namespace display

#endif // _LISTVIEW_H_
