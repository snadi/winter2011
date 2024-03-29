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


#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <utility>
#include <fstream>

#include "ModelContainer.h"
#include "KconfigWhitelist.h"

ConfigurationModel* ModelContainer::loadModels(std::string model) {
    ModelContainer *f = getInstance();
    int found_models = 0;
    typedef std::list<std::string> FilenameContainer;
    FilenameContainer filenames;
    ConfigurationModel *ret = 0;

    const boost::regex model_regex("^([[:alnum:]]+)\\.model$", boost::regex::perl);

    if (! boost::filesystem::exists(model)){
        std::cerr << "E: model '" << model << "' doesn't exist (neither directory nor file)" << std::endl;
        return 0;
    }

    if (! boost::filesystem::is_directory(model)) {
        /* A model file was specified, so load exactly this one */
        boost::match_results<const char*> what;
        std::string model_name = std::string(basename(model.c_str()));

        /* Strip the .model suffix if possible */
        if (boost::regex_search(model_name.c_str(), what, model_regex)) {
            model_name = std::string(what[1]);
        }
        ret = f->registerModelFile(model, model_name);
        std::cout << "I: loaded rsf model for " << model_name
                  << std::endl;

        return ret;
    }


    for (boost::filesystem::directory_iterator dir(model);
         dir != boost::filesystem::directory_iterator();
         ++dir) {
        filenames.push_back(dir->path().filename());
    }
    filenames.sort();

    for (FilenameContainer::iterator filename = filenames.begin();
         filename != filenames.end(); filename++) {
        boost::match_results<const char*> what;

        if (boost::regex_search(filename->c_str(), what, model_regex)) {
            std::string found_arch = what[1];
            ModelContainer::iterator a = f->find(found_arch);

            if (a == f->end()) {
                ConfigurationModel *a = f->registerModelFile(model + "/" + filename->c_str(), found_arch);
                /* overwrite the return value */
                if (a) ret = a;
                found_models++;

                std::cout << "I: loaded rsf model for " << found_arch
                          << std::endl;
            }
        }
    }

    if (found_models > 0) {
        std::cout << "I: found " << found_models << " rsf models" << std::endl;
        return ret;
    } else {
        std::cerr << "E: could not find any models" << std::endl;
        return 0;
    }
}

ConfigurationModel *ModelContainer::registerModelFile(std::string filename, std::string arch) {
    ConfigurationModel *db;
    /* Was already loaded */
    if ((db = lookupModel(arch.c_str()))) {
        std::cout << "I: A model for " << arch << " was already loaded" << std::endl;
        return db;
    }

    std::ifstream rsf_file(filename.c_str());
    static std::ofstream devnull("/dev/null");

    if (!rsf_file.good()) {
        std::cerr << "could not open file for reading: "
                  << filename << std::endl;
        return NULL;
    }
    db = new ConfigurationModel(arch, rsf_file, devnull);

    this->insert(std::make_pair(arch,db));

    return db;
};

ConfigurationModel *ModelContainer::lookupModel(const char *arch)  {
    ModelContainer *f = getInstance();
    // first step: look if we have it in our models list;
    ModelContainer::iterator a = f->find(arch);
    if (a != f->end()) {
        // we've found it in our map, so return it
        return a->second;
    } else {
        // No model was found
        return NULL;
    }
}

//sarah
ConfigurationModel* ModelContainer::lookupRelatedModel(std::string fileName)  {
	ModelContainer *f = getInstance();

	if(fileName[0]=='.' && fileName[1] == '/'){
		fileName  = fileName.substr(2);
	}

	int index = fileName.find("arch");

	if((index != std::string::npos) && (index < fileName.length())){
		fileName = fileName.substr(index + 5);
		
		index = fileName.find('/');
		if(index != std::string::npos){
			fileName.erase(index);
		}
	}else{
		return NULL;
	}	
	

	if(ModelContainer::isArchName(fileName)){
 	   	// first step: look if we have it in our models list;
	    	ModelContainer::iterator a = f->find(fileName);
		if (a != f->end()) {
        		// we've found it in our map, so return it
		//std::cout<<"Returning "<<a->second->getName()<<std::endl;
	        	return a->second;
	 	} else {
        		// No model was found
		        return NULL;
    		
		}
	}else{
		return NULL;
	}
}

bool ModelContainer::isArchName(std::string archName){


	if(archName.compare("alpha") == 0 || archName.compare("arm") == 0 || archName.compare("avr32") == 0 || archName.compare("blackfin") == 0 || 
		archName.compare("cris") == 0 || archName.compare("frv") == 0 || archName.compare("h8300") == 0 || archName.compare("ia64") == 0 || 
		archName.compare("m32r") == 0 || archName.compare("m68k") == 0 || archName.compare("m68knommu") == 0 || archName.compare("microblaze") == 0 || 			archName.compare("mips") == 0 || archName.compare("mn10300") == 0 || archName.compare("parisc") == 0 | archName.compare("powerpc") == 0 ||
 		archName.compare("s390") == 0 || archName.compare("score") == 0 || archName.compare("sh") == 0 ||  archName.compare("sparc") == 0 || 
		archName.compare("tile") == 0 || archName.compare("um")  == 0 || archName.compare("x86") == 0 || archName.compare("xtensa") == 0 )
			return true;
	else
			return false;
	
}

const char *ModelContainer::lookupArch(const ConfigurationModel *model) {
    ModelContainer *f = getInstance();
    ModelContainer::iterator i;
    for (i = f->begin(); i != f->end(); i++) {
        if ((*i).second == model)
            return (*i).first.c_str();
    }
    return NULL;
}

ConfigurationModel *ModelContainer::lookupMainModel() {
    ModelContainer *f = getInstance();
    return ModelContainer::lookupModel(f->main_model.c_str());
}

void ModelContainer::setModelExplicitlySpecified(bool value){
    ModelContainer *f = getInstance();
	f->modelExplicitlySpecified = value;
}

bool ModelContainer::isModelExplicitlySpecified(){
    ModelContainer *f = getInstance();
return f->modelExplicitlySpecified;
}

void ModelContainer::setMainModel(std::string main_model) {
    ModelContainer *f = getInstance();
    if (!ModelContainer::lookupModel(main_model.c_str())) {
        std::cerr << "E: Could not specify main model " << main_model << ", because no such model is loaded" << std::endl;
        return;
    }
    std::cout << "I: Using " << main_model << " as primary model" << std::endl;
    f->main_model = main_model;
}

const char *ModelContainer::getMainModel() {
    ModelContainer *f = getInstance();
    return f->main_model.c_str();
}


ModelContainer *ModelContainer::getInstance() {
    static ModelContainer *instance;
    if (!instance) {
        instance = new ModelContainer();
    }
    return instance;
}

ModelContainer::~ModelContainer() {
    ModelContainer::iterator i;
    for (i = begin(); i != end(); i++)
        free((*i).second);
}
