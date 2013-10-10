/* 
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define APPNAME "AirDC++n"
#define VERSIONSTRING "2.60"
#define DCVERSIONSTRING "0.830"

#define BETAVER

#ifdef _WIN64
# define CONFIGURATION_TYPE "x86-64"
#else
# define CONFIGURATION_TYPE "x86-32"
#endif

namespace dcpp {
	extern const std::string shortVersionString;
	extern const std::string fullVersionString;
	int getBuildNumber();
	string getBuildNumberStr();
}

#define BUILD_NUMBER_STR getBuildNumberStr()
#define BUILD_NUMBER getBuildNumber()

#ifdef BETAVER
#define VERSION_URL "http://builds.airdcpp.net/version/version.xml"
#else
#define VERSION_URL "http://version.airdcpp.net/version.xml"
#endif

#ifdef NDEBUG
# define INST_NAME "{AIRDC-AEE8350A-B49A-4753-AB4B-E55479A48351}"
#else
# define INST_NAME "{AIRDC-AEE8350A-B49A-4753-AB4B-E55479A48350}"
#endif

/* Update the .rc file as well... */