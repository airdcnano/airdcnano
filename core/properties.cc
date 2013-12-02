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

#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <core/properties.h>
#include <utils/utils.h>

#include <client/stdinc.h>
#include <client/StringTokenizer.h>

namespace core {

Properties::Properties():
    m_autosave(true),
    m_changed(false),
    m_vector_separator(";")
{

}

void Properties::load(std::istream &in)
{
    std::string key, value;
    key.reserve(20);

    m_properties.clear();
    for(std::string line; std::getline(in, line);) {
		auto comment = line.find('#');
		if (comment != std::string::npos) {
			auto space = line.find_first_not_of(" ", comment);
			if (space == std::string::npos) {
				continue;
			}
		}

        // key = value
		auto keyend = line.find_first_of(" =");
        key = line.substr(0, keyend);

		auto valuestart = line.find_first_not_of(" =", keyend);
        if(valuestart == std::string::npos)
            continue;

        value = line.substr(valuestart);
        m_properties[key] = value;
    }
}

void Properties::load()
{
    std::ifstream in;
    in.open(m_filename.c_str());
    if(in.good())
        load(in);
    in.close();
}

void Properties::save()
{
    std::string key, oldfilename;
    oldfilename = m_filename + ".old";
    std::rename(m_filename.c_str(), oldfilename.c_str());

    std::ofstream newfile(m_filename.c_str());
    std::ifstream oldfile(oldfilename.c_str());
    if(!newfile || !oldfile)
        return;

	auto temp = m_properties;
    for(std::string line; std::getline(oldfile, line);) {
		auto comment = line.find('#');
		auto space = line.find_last_not_of(" \t\n");

        if(comment < space || line.size() == 0) {
            newfile << line << std::endl;
            continue;
        }

        // key = value
		auto keyend = line.find_first_of(" =");
        key = line.substr(0, keyend);

		auto valuestart = line.find_first_not_of(" =", keyend);
        if(valuestart == std::string::npos)
            continue;

        newfile << key << " = " << m_properties[key] << std::endl; 
        temp.erase(key);
    }
    oldfile.close();

	for (const auto& i: temp) {
        newfile << i.first << " = " << i.second 
                << std::endl << std::endl;
    }

    newfile.close();
    std::remove(oldfilename.c_str());
}

std::string Properties::find_str(const std::string &key, std::string def)
    const
{
    auto it = m_properties.find(key);
    return (it == m_properties.end() ? def : it->second);
}

int Properties::find_int(const std::string &key, int def)
    const
{
    if(!exists(key))
        return def;
	return utils::to<int>(find_str(key));
}

bool Properties::find_bool(const std::string &key, bool def)
    const
{
    if(!exists(key))
        return def;
	return find_str(key) == "true" ? true : false;
}

std::vector<std::string> Properties::find_vector(const std::string &key)
    const
{
	return dcpp::StringTokenizer<std::string>(find_str(key), m_vector_separator).getTokens();
}

void Properties::set(const std::string &key, const std::string &value)
{
    m_properties[key] = value;
    if(m_autosave)
        save();
    else
        m_changed = true;
    properties_changed();
}

void Properties::set(const std::string &key, const char *value)
{
    set(key, std::string(value));
}

void Properties::set(const std::string &key, int value)
{
    set(key, utils::to_string(value));
}

void Properties::set(const std::string &key, bool value)
{
    set(key, std::string(value ? "true" : "false"));
}

void Properties::set(const std::string &key, const std::vector<std::string> &value)
{
    std::ostringstream oss;
    std::copy(value.begin(), value.end(),
        std::ostream_iterator<std::string>(oss, m_vector_separator));
    set(key, oss.str());
}

void Properties::set_autosave(bool autosave)
{
    m_autosave = autosave;
    if(m_changed) {
        save();
        m_changed = false;
    }
}

Properties::~Properties()
{
    if(m_changed)
        save();
}

} // namespace core

