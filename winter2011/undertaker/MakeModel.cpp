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


#include "MakeModel.h"

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <list>
#include <stack>


MakeModel::MakeModel(std::string name, std::ifstream &in, std::ostream &log) : RsfReader(in, log), _name(name) {
}

std::string MakeModel::getExp(std::string fileName){
	std::string shortfilename = fileName.substr(2);
	for (iterator i = begin(); i != end(); i++){
		if((fileName.compare((*i).first) == 0) || (shortfilename.compare((*i).first) == 0) ){
			if((*i).second.size() == 0)
				return "";			
			else 
				return "( " + (*i).second.front() + " )";			
		}
	}

return "(false)";
}

bool MakeModel::isRelevent(std::string fileName){

	if( (getExp(fileName).compare("(false)") != 0)){
		return true;
	}

	return false;
}


