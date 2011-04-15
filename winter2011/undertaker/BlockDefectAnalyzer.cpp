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


#include "StringJoiner.h"
#include "BlockDefectAnalyzer.h"
#include "ModelContainer.h"
#include "MakeModelContainer.h"
#include<fstream>

//sarah
unsigned int BlockDefectAnalyzer::dead_blocks = 0;
unsigned int BlockDefectAnalyzer::undead_blocks = 0;
unsigned int BlockDefectAnalyzer::symbolic_blocks = 0;
unsigned int BlockDefectAnalyzer::logic_blocks = 0;
unsigned int BlockDefectAnalyzer::code_blocks = 0;
unsigned int BlockDefectAnalyzer::kconfig_blocks = 0;
unsigned int BlockDefectAnalyzer::missing_blocks = 0;

void BlockDefectAnalyzer::markOk(const std::string &arch) {
    _OKList.push_back(arch);
}

std::string BlockDefectAnalyzer::getBlockPrecondition(const ConfigurationModel *model) const {
    StringJoiner formula;
std::cout<<"getting block precondition"<<std::endl;
    /* Adding block and code constraints extraced from code sat stream */
    formula.push_back(_block);
    formula.push_back(_cs->getCodeConstraints());


	MakeModel* makeModel = MakeModelContainer::lookupModel(ModelContainer::lookupArch(model));
std::string makeConstraints = _cs->getMakeConstraints(makeModel);
	/* Adding make constraints */
if(makeModel)
	formula.push_back(makeConstraints);
   


    if (model) {
        /* Adding kconfig constraints and kconfig missing */
        std::set<std::string> missingSet;
        formula.push_back(_cs->getKconfigConstraints(model, missingSet, makeConstraints));
        formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
    }

    return formula.join("\n&&\n");
}

DeadBlockDefect::DeadBlockDefect(CodeSatStream *cs, const char *block)
    :  BlockDefectAnalyzer(None), _needsCrosscheck(false),
       _arch(NULL) {
    this->_suffix = "dead";
    this->_block = block;
    this->_cs = cs;
}

bool DeadBlockDefect::isDefect(const ConfigurationModel *model) {
    StringJoiner formula;

    if(!_arch)
        _arch = ModelContainer::lookupArch(model);

	MakeModel* makeModel = MakeModelContainer::lookupModel(_arch);

    formula.push_back(_block);
    formula.push_back(_cs->getCodeConstraints());
	std::string codeFormula = formula.join("\n&&\n");
    SatChecker code_constraints(codeFormula);

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
	_formula = codeFormula;
//std::cout<<"returning code defect"<<std::endl;
        return true;
    }


//if this file is not relevent to the arch being examined, then return false without examination. We check for code defects first since they do not depend on the //constraints
	//if(!makeModel->isRelevent( _cs->getFilename())){
	//	return false;
	//}

    //sarah
	std::string makeConstraints = "";
	if(makeModel){	

		makeConstraints = _cs->getMakeConstraints(makeModel);
    		formula.push_back(makeConstraints);
    		std::string makeformula = formula.join("\n&&\n");

    		SatChecker make_constraints(makeformula);

		if(!make_constraints()){
			if(_defectType != Make){
				_formula = makeformula;
				_arch = ModelContainer::lookupArch(model);
			}
//std::cout<<"returning make"<<std::endl;
			_defectType = Make;		
			return true;
    		}
	}


    if (model) {

        std::set<std::string> missingSet;
        formula.push_back(_cs->getKconfigConstraints(model, missingSet, makeConstraints));
        std::string formula_str = formula.join("\n&&\n");
        SatChecker kconfig_constraints(formula_str);


        if (!kconfig_constraints()) {
            if (_defectType != Configuration) {
                // Wasn't already identified as Configuration defect
                // crosscheck doesn't overwrite formula
                _formula = formula_str;
                _arch = ModelContainer::lookupArch(model);
            }
            _defectType = Configuration;
	//std::cout<<"Returning ocnfiguration defect on arch "<<_arch<<std::endl;
            return true;
        } else {
            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
            std::string formula_str = formula.join("\n&&\n");
            SatChecker missing_constraints(formula_str);

            if (!missing_constraints()) {
                if (_defectType != Configuration) {
                    _formula = formula_str;
                    _defectType = Referential;
                }
	//std::cout<<"Returning missing defect on arch "<<ModelContainer::lookupArch(model)<<std::endl;
                return true;
            }
        }
    }
std::cout<<"not a dead defect: "<<false<<std::endl;
    return false;
}

bool DeadBlockDefect::isGlobal() const { return _isGlobal; }
bool DeadBlockDefect::needsCrosscheck() const {
    switch(_defectType) {
    case None:
    case Implementation:
	return false;
    case Configuration:
    case Make:
	//std::cout<<	"returning "<<!ModelContainer::isModelExplicitlySpecified()<<std::endl;
	return !ModelContainer::isModelExplicitlySpecified();
    default:
        // skip crosschecking if we already know that it is global
        return !_isGlobal;
    }
}

void BlockDefectAnalyzer::defectIsGlobal() { _isGlobal = true; }

const std::string BlockDefectAnalyzer::defectTypeToString() const {
    switch(_defectType) {
    case None: return "";  // Nothing to write
    case Implementation:
//sarah
	code_blocks++;
        return "code";
    case Configuration:
//sarah
kconfig_blocks++;
        return "kconfig";
    case Referential:
//sarah
	missing_blocks++;
        return  "missing";
	case Make:
	return "make";
    default:
        assert(false);
    }
    return "";
}


bool DeadBlockDefect::writeReportToFile() const {
    StringJoiner fname_joiner;

//sarah
std::string filename_sar = _cs->getFilename();
if(filename_sar [0]=='.' && filename_sar [1] == '/'){
filename_sar  = filename_sar .substr(2);
}
int position = filename_sar.find( "/" ); // find first space

   // 
   while ( position != std::string::npos ) 
   {
      filename_sar.replace( position, 1, "." );
      position = filename_sar.find( "/", position + 1 );
   } 
filename_sar = "output/" + filename_sar ;

    fname_joiner.push_back(filename_sar);
    fname_joiner.push_back(_block);
    if(_arch && !_isGlobal)
        fname_joiner.push_back(_arch);

    switch(_defectType) {
    case None: return false;  // Nothing to write
    default:
        fname_joiner.push_back(defectTypeToString());
    }

    if (_isGlobal)
        fname_joiner.push_back("globally");

    fname_joiner.push_back(_suffix);

//sarah
fname_joiner.push_back("M");

//sarah
if(std::string("dead").compare(_suffix) == 0){
dead_blocks++;
}else if(std::string("undead").compare(_suffix) == 0){
undead_blocks++;
}
//end sarah

    const std::string filename = fname_joiner.join(".");
    const std::string &contents = _formula;
//std::cout<<"THE FORMULA: "<<_formula<<std::endl;

    std::ofstream out(filename.c_str());

    if (!out.good()) {
        std::cerr << "failed to open " << filename << " for writing " << std::endl;
        return false;
    } else {
        std::cout << "I: creating " << filename << std::endl;
//sarah
std::string position = _cs->getLine(_block);
if(position.find_last_of("symbolic") != std::string::npos ){
symbolic_blocks++;
}else if(position.find_last_of("logic") != std::string::npos){
logic_blocks++;
}
//endsarah
//changed _cs->getline(_block) to position
        out << "#" << _block << ":" << position << std::endl;
        out << SatChecker::pprinter(contents.c_str());
        out.close();
    }

    return true;
}

UndeadBlockDefect::UndeadBlockDefect(CodeSatStream *cs, const char *block)
    : DeadBlockDefect(cs, block) { this->_suffix = "undead"; }

bool UndeadBlockDefect::isDefect(const ConfigurationModel *model) {
std::cout<<"in undead defect "<<std::endl;
    StringJoiner formula;
    const char *parent = _cs->getParent(_block);

    // no parent -> impossible to be undead
    if (!parent)
        return false;

//std::cout<<"checking arch with model: "<<std::endl;
    if (!_arch)
        _arch = ModelContainer::lookupArch(model);

//std::cout<<"got architecture"<<std::endl;
	MakeModel* makeModel = MakeModelContainer::lookupModel(_arch);
//std::cout<<"after makemodel"<<std::endl;


    formula.push_back("( " + std::string(parent) + " && ! " + std::string(_block) + " )");
    formula.push_back(_cs->getCodeConstraints());
    std::string codeFormula = formula.join("\n&&\n");
    SatChecker code_constraints(codeFormula);

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
		_formula = codeFormula;
        return true;
    }

//if this file is not relevent to the arch being examined, then return false without examination
	//if(!makeModel->isRelevent( _cs->getFilename()))
	//	return false;

	//sarah
	std::string makeConstraints = "";
	if(makeModel){
		makeConstraints = _cs->getMakeConstraints(makeModel);
    		formula.push_back(makeConstraints);
    		std::string makeformula = formula.join("\n&&\n");
    		SatChecker make_constraints(makeformula);

		if(!make_constraints()){
			if(_defectType != Make){
				_formula = makeformula;
				_arch = ModelContainer::lookupArch(model);
			}
			_defectType = Make;			
			return true;
    		}
	}



    if (model) {
        std::set<std::string> missingSet;
        formula.push_back(_cs->getKconfigConstraints(model, missingSet, makeConstraints));
        std::string formula_str = formula.join("\n&&\n");
        SatChecker kconfig_constraints(formula_str);

        if (!kconfig_constraints()) {
            if (_defectType != Configuration) {
                // Wasn't already identified as Configuration defect
                // crosscheck doesn't overwrite formula
                _formula = formula_str;
                _arch = ModelContainer::lookupArch(model);
            }
            _defectType = Configuration;
            return true;
        } else {
            formula.push_back(ConfigurationModel::getMissingItemsConstraints(missingSet));
            std::string formula_str = formula.join("\n&&\n");
            SatChecker missing_constraints(formula_str);

            if (!missing_constraints()) {
                if (_defectType != Configuration) {
                    _formula = formula_str;
                    _defectType = Referential;
                }

                return true;
            }
        }
    }
    return false;
}
