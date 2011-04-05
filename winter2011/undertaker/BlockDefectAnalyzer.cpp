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

    /* Adding block and code constraints extraced from code sat stream */
    formula.push_back(_block);
    formula.push_back(_cs->getCodeConstraints());

	/* Adding make constraints */
	formula.push_back(_cs->getMakeConstraints());
   


    if (model) {
        /* Adding kconfig constraints and kconfig missing */
        std::set<std::string> missingSet;
        formula.push_back(_cs->getKconfigConstraints(model, missingSet));
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

    formula.push_back(_block);


    formula.push_back(_cs->getCodeConstraints());
    SatChecker code_constraints(_formula = formula.join("\n&&\n"));

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
        return true;
    }

    //sarah
    formula.push_back(_cs->getMakeConstraints());
    std::string makeformula = formula.join("\n&&\n");
    SatChecker make_constraints(makeformula);

    if(!make_constraints()){
	_defectType = Make;
	_isGlobal = true;
	_formula = formula_str;
	return true;
    }

    if (model) {
        std::set<std::string> missingSet;
        formula.push_back(_cs->getKconfigConstraints(model, missingSet));

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
            _isGlobal = true;
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

bool DeadBlockDefect::isGlobal() const { return _isGlobal; }
bool DeadBlockDefect::needsCrosscheck() const {
    switch(_defectType) {
    case None:
    case Implementation:
    case Configuration:
        return false;
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

    if(_arch && (!_isGlobal || _defectType == Configuration))
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
    StringJoiner formula;
    const char *parent = _cs->getParent(_block);

    // no parent -> impossible to be undead
    if (!parent)
        return false;

    if (!_arch)
        _arch = ModelContainer::lookupArch(model);

    formula.push_back("( " + std::string(parent) + " && ! " + std::string(_block) + " )");
    formula.push_back(_cs->getCodeConstraints());
    SatChecker code_constraints(_formula = formula.join("\n&&\n"));

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
        return true;
    }

	//sarah
    formula.push_back(_cs->getMakeConstraints());
    std::string makeformula = formula.join("\n&&\n");
    SatChecker make_constraints(makeformula);

    if(!make_constraints()){
	_defectType = Make;
	_isGlobal = true;
	_formula = formula_str;
	return true;
    }


    if (model) {
        std::set<std::string> missingSet;
        formula.push_back(_cs->getKconfigConstraints(model, missingSet));

        std::string formula_str = formula.join("\n&&\n");
        SatChecker kconfig_constraints(formula_str);

        if (!kconfig_constraints()) {
            if (_defectType != Configuration) {
                // Wasn't already identified as Configuration defect
                // crosscheck doesn't overwrite formula
                _formula = formula_str;
                _arch = ModelContainer::lookupArch(model);
            }
	    _formula = formula_str;
            _defectType = Configuration;
            _isGlobal = true;
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
