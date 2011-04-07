#ifndef make_model_h__
#define make_model_h__

#include <string>
#include <map>
#include <deque>
#include <set>
#include <list>

#include "RsfReader.h"

class MakeModel : public RsfReader {
public:
  
    	MakeModel(std::string name, std::ifstream &in, std::ostream &log);

	std::string getExp(std::string filename);
	std::string getName() const { return _name; }
	bool isRelevent(std::string filename);
private:
    std::string _name;
};

#endif
