/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


// -*- mode: c++ -*-
#ifndef kconfigwhitelist_h__
#define kconfigwhitelist_h__

#include <list>
#include <string>

/**
 * \brief Manages the whitelist of Kconfig Items
 */
struct KconfigWhitelist : protected std::list<std::string> {
    static KconfigWhitelist *getInstance(); //!< accessor for this singleton
    bool empty(); //!< checks if the whitelist is empty
    bool isWhitelisted(const char*) const; //!< checks if the given item is in the whitelist
    void addToWhitelist(const std::string); //!< adds an item to the whitelist

    /**
     * \brief load Kconfig Items from a textfile into the whitelist
     * \param file the filename to load items from
     * \return the number of items that where loaded
     */
    int loadWhitelist(const char *file);
private:
    KconfigWhitelist() : std::list<std::string>() {} //!< private c'tor
};

#endif
