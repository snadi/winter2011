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
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "SatChecker.h"
#include "KconfigWhitelist.h"

#include "SatChecker-grammar.t"

bool SatChecker::check(const std::string &sat) throw (SatCheckerError) {
    SatChecker c(sat.c_str());
    try {
        return c();
    } catch (SatCheckerError &e) {
        std::cout << "Syntax Error:" << std::endl;
        std::cout << sat << std::endl;
        std::cout << "End of Syntax Error" << std::endl;
        throw e;
    }
    return false;
}


/* Got the impression from normal lisp implementations */
int SatChecker::stringToSymbol(const std::string &key) {
    KconfigWhitelist *wl = KconfigWhitelist::getInstance();

    /* If whitelisted do always return a new symbol, so these items
       are free variables */

    if (wl->isWhitelisted(key.c_str())) {
        return newSymbol();
    }

    std::map<std::string, int>::iterator it;
    if ((it = symbolTable.find(key)) != symbolTable.end()) {
        return it->second;
    }
    int n = newSymbol();
    symbolTable[key] = n;
    return n;
}

int SatChecker::newSymbol(void) {
    return Picosat::picosat_inc_max_var();
}

void SatChecker::addClause(int *clause) {
    for (int *x = clause; *x; x++)
        Picosat::picosat_add(*x);
    Picosat::picosat_add(0);
}

int SatChecker::notClause(int inner_clause) {
    int this_clause  = newSymbol();

    int clause1[3] = { this_clause,  inner_clause, 0};
    int clause2[3] = {-this_clause, -inner_clause, 0};

    addClause(clause1);
    addClause(clause2);

    return this_clause;
}

int SatChecker::andClause(int A_clause, int B_clause) {
    // This function just does binary ands in contradiction to the
    // andID transform_rec
    // A & B & ..:
    // !3 A 0
    // !3 B 0
    // 3 !A !B 0

    int this_clause = newSymbol();

    int A[] = {-this_clause, A_clause, 0};
    int B[] = {-this_clause, B_clause, 0};
    int last[] = {this_clause, -A_clause, -B_clause, 0};

    addClause(A);
    addClause(B);
    addClause(last);

    return this_clause;
}

int SatChecker::orClause(int A_clause, int B_clause) {
    // This function just does binary ands in contradiction to the
    // andID transform_rec
    // A & B & ..:
    // 3 !A 0
    // 3 !B 0
    // !3 A B 0

    int this_clause = newSymbol();

    int A[] = {this_clause, -A_clause, 0};
    int B[] = {this_clause, -B_clause, 0};
    int last[] = {-this_clause, A_clause, B_clause, 0};

    addClause(A);
    addClause(B);
    addClause(last);

    return this_clause;
}


int SatChecker::transform_bool_rec(iter_t const& input) {
    iter_t root_node = input;
 beginning_of_function:
    iter_t iter = root_node->children.begin();


    if (root_node->value.id() == bool_grammar::symbolID) {
        iter_t inner_node = root_node->children.begin();
        std::string value (inner_node->value.begin(), inner_node->value.end());
        _debug_parser("- " + value, false);
        return stringToSymbol(value);
    } else if (root_node->value.id() == bool_grammar::not_symbolID) {
        iter_t inner_node = root_node->children.begin()->children.begin();
        _debug_parser("- not");
        int inner_clause = transform_bool_rec(inner_node);
        _debug_parser();

        return notClause(inner_clause);
    } else if (root_node->value.id() == bool_grammar::andID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }

        int i = 0, end_clause[root_node->children.size() + 2];

        int this_clause = newSymbol();
        _debug_parser("- and");
        // A & B & ..:
        // !3 A 0
        // !3 B 0
        // ..
        // 3 !A !B .. 0
        end_clause[i++] = this_clause;

        for (; iter != root_node->children.end(); ++iter) {
            int child_clause = transform_bool_rec(iter);
            int child[3] = {-this_clause, child_clause, 0};
            end_clause[i++] = -child_clause;

            addClause(child);
        }
        end_clause[i++] = 0;

        addClause(end_clause);

        _debug_parser();
        return this_clause;
    } else if (root_node->value.id() == bool_grammar::orID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        int i = 0, end_clause[root_node->children.size() + 2];

        int this_clause  = newSymbol();
        end_clause[i++] = -this_clause;
        // A | B
        // 3 !A 0
        // 3 !B 0
        // !3 A B 0
        _debug_parser("- or");

        for (; iter != root_node->children.end(); ++iter) {
            int child_clause = transform_bool_rec(iter);
            int child[3] = {this_clause, -child_clause, 0};
            end_clause[i++] = child_clause;

            addClause(child);
        }
        end_clause[i++] = 0;

        addClause(end_clause);
        _debug_parser();

        return this_clause;
    } else if (root_node->value.id() == bool_grammar::impliesID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        // A -> B
        // 3 A 0
        // 3 !B 0
        // !3 !A B 0
        // A  ->  B  ->  C
        // (A -> B)  ->  C
        //    A      ->  C
        //          A
        _debug_parser("- ->");
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != root_node->children.end(); iter++) {
            int B_clause = transform_bool_rec(iter);
            int this_clause = newSymbol();
            int c1[] = { this_clause, A_clause, 0};
            int c2[] = { this_clause, -B_clause, 0};
            int c3[] = { -this_clause, -A_clause, B_clause, 0};
            addClause(c1);
            addClause(c2);
            addClause(c3);
            A_clause = this_clause;
        }
        _debug_parser();
        return A_clause;
    } else if (root_node->value.id() == bool_grammar::iffID) {
        iter_t iter = root_node->children.begin();
        /* Skip or rule if there is only one child */
        if ((iter+1) == root_node->children.end()) {
            return transform_bool_rec(iter);
        }
        // A <-> B
        // 3 !A  !B 0
        // 3 A B 0
        // !3 !A B 0
        // !3 A !B 0
        // A  <->  B  <->  C
        // (A <-> B)  <->  C
        //    A      <->  C
        //          A
        _debug_parser("- <->");
        int A_clause = transform_bool_rec(iter);
        iter ++;
        for (; iter != root_node->children.end(); iter++) {
            int B_clause = transform_bool_rec(iter);
            int this_clause = newSymbol();
            int c1[] = { this_clause,  -A_clause, -B_clause, 0};
            int c2[] = { this_clause,   A_clause,  B_clause, 0};
            int c3[] = { -this_clause, -A_clause,  B_clause, 0};
            int c4[] = { -this_clause,  A_clause, -B_clause, 0};
            addClause(c1);
            addClause(c2);
            addClause(c3);
            addClause(c4);
            A_clause = this_clause;
        }
        _debug_parser();
        return A_clause;
    } else if (root_node->value.id() == bool_grammar::comparatorID) {
        /* Skip and rule if there is only one child */
        if (root_node->children.size() == 1) {
            return transform_bool_rec(iter);
        }
        int this_clause  = newSymbol();
        _debug_parser("- __COMP__N", false);
        return this_clause;

    } else {
        /* Just a Container node, we simply go inside and try again. */
        root_node = root_node->children.begin();
        goto beginning_of_function;
    }
}

void SatChecker::fillSatChecker(std::string expression) throw (SatCheckerError) {
    static bool_grammar e;
//std::cout<<"CHECKING EXPRESSION : "<<expression<<std::endl;
    tree_parse_info<> info = pt_parse(expression.c_str(), e,
                                      space_p | ch_p("\n") | ch_p("\r"));

    if (info.full) {
        fillSatChecker(info);
    } else {
       // Enable this line to get the position where the parser dies
     //   std::cout << std::string(expression.begin(), expression.begin()
       //                          + info.length) << std::endl;
        
        Picosat::picosat_reset();
//sarah
        throw SatCheckerError("SatChecker: Couldn't parse: " + expression + " at line: " + std::string(expression.begin(), expression.begin()
                                 + info.length) );
//        throw SatCheckerError("");
    }
}

void SatChecker::fillSatChecker(tree_parse_info<>& info) {
    iter_t expression = info.trees.begin()->children.begin();
    int top_clause = transform_bool_rec(expression);
    /* This adds the last clause */
    Picosat::picosat_assume(top_clause);
}

SatChecker::SatChecker(const char *sat, int debug)
  : debug_flags(debug), _sat(std::string(sat)) { }

SatChecker::SatChecker(const std::string sat, int debug)
  : debug_flags(debug), _sat(std::string(sat)) { }

SatChecker::~SatChecker() { }

bool SatChecker::operator()() throw (SatCheckerError) {
    /* Clear the debug parser, if we are called multiple times */
    debug_parser.clear();
    debug_parser_indent = 0;

    Picosat::picosat_init();
    // try to enable as many features as possible
    Picosat::picosat_set_global_default_phase(1);

    fillSatChecker(_sat);

    int res = Picosat::picosat_sat(-1);

    if (res == PICOSAT_SATISFIABLE) {
        /* Let's get the assigment out of picosat, because we have to
           reset the sat solver afterwards */
        std::map<std::string, int>::const_iterator it;
        for (it = symbolTable.begin(); it != symbolTable.end(); ++it) {
            bool selected = Picosat::picosat_deref(it->second) == 1;
            assignmentTable.insert(std::make_pair(it->first, selected));
        }
    }

    Picosat::picosat_reset();

    if (res == PICOSAT_UNSATISFIABLE)
        return false;
    return true;
}

std::string SatChecker::pprint() {
    if (debug_parser.size() == 0) {
        int old_debug_flags = debug_flags;
        debug_flags |= DEBUG_PARSER;
        (*this)();
        debug_flags = old_debug_flags;
    }
    return _sat + "\n\n" + debug_parser + "\n";
}

int SatChecker::formatConfigItems(AssignmentMap solution, std::ostream &out, const MissingSet &missingSet) {
    typedef std::map<std::string, state> SelectionType;
    SelectionType selection, other_variables;

    for (AssignmentMap::iterator it = solution.begin(); it != solution.end(); it++) {
        static const boost::regex item_regexp("^CONFIG_(.*)$", boost::regex::perl);
        static const boost::regex module_regexp("^CONFIG_(.*)_MODULE$", boost::regex::perl);
        static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);
        static const boost::regex choice_regexp("^CONFIG_CHOICE_.*$", boost::regex::perl);
        const std::string &name = (*it).first;
        const bool &valid = (*it).second;
        boost::match_results<std::string::const_iterator> what;

        if (valid && boost::regex_match(name, what, module_regexp)) {
            const std::string &name = what[1];
            std::string fullname = "CONFIG_" + name;
            if (missingSet.find(fullname) != missingSet.end()) {
                std::cout << "Ignoring 'missing' item " << fullname << std::endl;
                other_variables[fullname] = valid ? yes : no;
            } else
                selection[name] = module;
        } else if (boost::regex_match(name, what, choice_regexp)) {
            other_variables[what[0]] = valid ? yes : no;
            continue;
        } else if (boost::regex_match(name, what, item_regexp)) {
            const std::string &name = what[1];
            if (missingSet.find(what[0]) != missingSet.end()) {
                std::cout << "Ignoring 'missing' item " << what[0] << std::endl;

                other_variables[what[0]] = valid ? yes : no;
                continue;
            }

            // ignore entries if already set (e.g., by the module variant).
            if (selection.find(name) == selection.end())
                selection[name] = valid ? yes : no;
        } else if (boost::regex_match(name, block_regexp))
            // ignore block variables
            continue;
        else
            other_variables[name] = valid ? yes : no;
    }
    for (SelectionType::iterator s = selection.begin(); s != selection.end(); s++) {
        const std::string &item = (*s).first;
        const int &state = (*s).second;
        out << "CONFIG_" << item << "=";
        if (state == no)
            out << "n";
        else if (state == module)
            out << "m";
        else if (state == yes)
            out << "y";
        else
            assert(false);
        out << std::endl;
    }
    for (SelectionType::iterator s = other_variables.begin(); s != other_variables.end(); s++) {
        const std::string &item = (*s).first;
        const int &state = (*s).second;
        out << "# " << item << "=";
        if (state == no)
            out << "n";
        else if (state == yes)
            out << "y";
        else
            assert(false);
        out << std::endl;
    }
    return selection.size();
}
