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

#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

#include <string>
#include <map>
#include <stdexcept>
#include <vector>
#include <fstream>

namespace core {

typedef std::map<std::string, std::string> HashMap;
typedef std::vector<std::string> StringVector;

/** A class for handling simple configuration files.
  * File format is as simple as "key = values". Comment
  * lines begin with '#'.
  * Example:
  * \code
  * # timestamp format
  * timestamp_format = [%H:%M:%S]
  * \endcode*/
class Properties
{
public:
    /** Constructor. Does nothing. */
    Properties();

    /** Set the filename for reading and writing. */
    void set_filename(const std::string &filename) { m_filename = filename; }

    /** Load properties from the file */
    void load();

    /** Load from a stream */
    void load(std::istream &in);

    /** Saves properties to the file.
     * If autosave is true, file is saved when a value is edited. */
    void save();

    /** Set autosave. */
    void set_autosave(bool autosave);

    /** Returns true if autosave is set. */
    bool get_autosave() const { return m_autosave; }

    /** Returns the filename. */
    const std::string &get_filename() const { return m_filename; }

    /** Returns the separator of vector values in the config file. Default value is \c ; */
    const char* get_vector_separator() const { return m_vector_separator; }

    /** Set the vector value separator. */
    void set_vector_separator(const char* separator) { m_vector_separator = separator; }

    /** Returns the value of the key. */
	std::string find_str(const std::string &key, std::string def = "") const;

    /** Find an integer value. */
    int find_int(const std::string &key, int def=0) const;

    /** Find a boolean value. If the string is "true" true is returned, otherwise false is returned. */
    bool find_bool(const std::string &key, bool def) const;

    /** Find a vector. Values are separated by \ref vector_separator */
    StringVector find_vector(const std::string &key) const;

    /** Set a string value. */
    void set(const std::string &key, const std::string &value);

    /** Set a string value. */
    void set(const std::string &key, const char *value);

    /** Set an integer value. */
    void set(const std::string &key, int value);

    /** Set a boolean value. */
    void set(const std::string &key, bool value);

    /** Set a vector value. */
    void set(const std::string &key, const std::vector<std::string> &value);

    /** Check whether the key is set or not.
     * @param key The key to check 
     * @return True if the key is set */
    bool exists(const std::string &key) const { return m_properties.find(key) != m_properties.end(); }

    /** Called when a value is changed. */
    virtual void properties_changed() { }

    /** Returns the map containing all properties and values. */
    HashMap get_properties() { return m_properties; }

    /** Destructor. Saves the file. */
    virtual ~Properties();
private:
    bool m_autosave;
    bool m_changed;
    std::string m_filename;
    HashMap m_properties;
    const char *m_vector_separator;
};

} // namespace core

#endif // _PROPERTIES_H_

