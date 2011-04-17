#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <regex>
#include <ext/hash_map>
#include "CompositeObject.h"
using namespace std;

string architecture;
ofstream outputFile;
const string HEAD = "head-";
const string INIT = "init-";
const string CORE = "core-";
const string LIBS = "libs-";
const string DRIVERS = "drivers-";
const string NET = "net-";
const string OBJ = "obj-";

struct eqstr{
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1,s2)==0;
  }
};


void searchFile(string, string);
void parseLine(string path, string line, string dependencyString, int afterDashIndex, ifstream& inputFile);

void getNextCompositeLine(string& line, int lineNumber, string fileName){
	string nextLine = "";
	
	//find index of first character from the back of the string
	int index = line.length() -1;
	while(line[index] == ' ' || line[index] == '\t'){
		index--;
	}

	//if this character is the newline sign
	if( line[index] == '\\'){		
		line.replace(index, 1, " "); //remove the "\"
		char command[300];
		sprintf(command, "sed -n '%d,%dp' %s  > line.txt", lineNumber, lineNumber, fileName.c_str());
		system(command);			
		ifstream lineFile("line.txt");
		
		if(lineFile.is_open() && !lineFile.eof()){
			getline(lineFile, nextLine);
			line = line + " " + nextLine;
			lineFile.close();
			getNextCompositeLine(line, lineNumber + 1, fileName);
		}
	}	

}

void getNextLines(string& files, ifstream& inputFile){
	string nextLine = "";
	//if lines ends with \ then the next line is also included
	int index = files.length() -1;

	while(files[index] == ' ' || files[index] == '\t'){
		index--;
	}

	if( files[index] == '\\'){
		if( inputFile.good() && !inputFile.eof() ){
			files.replace(index, 1, " "); //remove the "\"
			getline(inputFile, nextLine);
			files = files + " " + nextLine;
			getNextLines(files, inputFile);
		}
	}
}

int findLineNumber(string line){
	int colonIndex = line.find(":");
	string lineNumber = line.substr(0, colonIndex);
	return atoi(lineNumber.c_str());
}

bool addToCompositeList(string path, string object, string dependencyString, ifstream& inputFile){
	string compositeObject = object.substr(0, object.length() - 2);
	
	if(compositeObject[0] == '$')
		return true; //will return true so no further proceessing is done

	system(("grep -n ^" + compositeObject + "- " + path + "Makefile  > grepResults.txt").c_str());
	ifstream grepResults("grepResults.txt");

	string line = "";
	bool isCompositeObj = false;

	if(grepResults.is_open()){
		while(grepResults.good() && !grepResults.eof()){
			getline(grepResults, line);
			if(line.length() != 0){
				int lineNumber = findLineNumber(line) + 1;
				int objectIndex = line.find(compositeObject);
				int dashIndex = line.find("-", objectIndex + compositeObject.length());
				getNextCompositeLine(line, lineNumber, path + "/Makefile");
				parseLine(path, line, dependencyString, dashIndex + 1, inputFile);
				isCompositeObj = true;
			}
		}
	}
	grepResults.close();
	return isCompositeObj;
}

void addFileDependencies(string file, string dependencyString){

	system(("grep \"#include\" " + file + " > includeResults.txt").c_str());
	ifstream includeResults("includeResults.txt");
	string line;
	if(includeResults.is_open() ){
		while(!includeResults.eof()){
			getline(includeResults, line);
			int firstBracket = line.find("<");
			int lastBracket = line.find(">");
			if(firstBracket == string::npos && lastBracket == string::npos){
				firstBracket = line.find("\"");
				lastBracket = line.find("\"", firstBracket + 1);
			}
			string includeName = line.substr(firstBracket + 1, lastBracket - firstBracket + 1);
			outputFile<<"\"./"<<includeName<<"\"";
			if(dependencyString.length() != 0)
				outputFile<<" \"("<<dependencyString<<")\"";
				
			outputFile<<endl;
		}
	}
	includeResults.close();
}

void addFiles(string path, string files, string dependencyString, ifstream& inputFile){

    	string token; // Have a buffer string
	getNextLines(files, inputFile);
	stringstream ss(files); // Insert the string into a stream
	while (ss >> token && (token.compare("\\") != 0)){        
		if(token[token.length() - 1] == '/'){
			searchFile(path + token, dependencyString);
		}else {
			//if it is not a directory, then check if it is a complex object first
			if(!addCompositeObject(path, token, dependencyString, inputFile)){
				//if it is not a complex object and ends with .o then simply replace .o with .c
				//and output its dependencyString
				if(token[token.length() - 1] == 'o'){
					token[token.length() -1] = 'c'; //replace .o with .c
					outputFile<<"\""<<path + token<<"\"";
					if(dependencyString.length() != 0)
						outputFile<<" \"("<<dependencyString<<")\"";
				
					outputFile<<endl;
				
					//addFileDependencies(path + token, dependencyString);
				}
			}
		}
	}
}

int findKeyword(string line){

	int index = string::npos;

	//look for the keywords, and make sure they are at the beginning of the line
	if( ((index = line.find(HEAD)) != string::npos) && (index == 0)  ){	
		index = index + HEAD.length();
	}else if((index = line.find(INIT)) != string::npos && (index == 0)){
		index = index + INIT.length();
	}else if((index = line.find(CORE)) != string::npos && (index == 0)){
		index = index + CORE.length();
	}else if((index = line.find(LIBS)) != string::npos && (index == 0)){
		index = index + LIBS.length();
	}else if((index = line.find(DRIVERS)) != string::npos && (index == 0)){
		index = index + DRIVERS.length();
	}else if((index = line.find(NET)) != string::npos && (index == 0)){
		index = index + NET.length();
	}else if((index = line.find(OBJ)) != string::npos && (index == 0)){
		index = index + OBJ.length();
	}

	return index;
}

void parseLine(string path, string line, string dependencyString, int afterDashIndex, ifstream& inputFile){
	int delimiterPos = -1;
	string dependency = "";
	string part1 = "";
	string part2 = "";
			
	if((delimiterPos = line.find(":=")) == string::npos){
		//if := not found, check for +=. If that's not found, skip this line
		if((delimiterPos = line.find("+=")) == string::npos){
			return;
		}
	}

	part1 = line.substr(afterDashIndex, delimiterPos - afterDashIndex);	
	part2 = line.substr(delimiterPos + 2);
	
	//if there is a config dependency (i.e. its not -y or -m)
	if(!((part1.compare("y") == 0) || (part1.compare("m") == 0) || (part1.compare("objs") == 0))){
		int configIndex = -1;
		if((configIndex = part1.find("CONFIG_")) != string::npos){
			int bracketIndex = part1.find(")");
			dependency = part1.substr(configIndex, bracketIndex - configIndex);
		}
	}

	if(dependency.length() != 0 && dependencyString.length() != 0){
		//there is a current dependency and a parent dependency
		addFiles(path, part2, dependencyString + " && " + dependency, inputFile);
	}else if(dependency.length() != 0){
		//found a current dependency but no parent dependencies
		addFiles(path, part2, dependency, inputFile);
	}else if(dependencyString.length() != 0){
		//found no current dependency but there is a parent dependency
		addFiles(path, part2, dependencyString, inputFile);
	}else{
		//no parent dependencies & no current dependencies
		addFiles(path, part2, "", inputFile);
		
	}
}

void searchFile(string path, string dependencyString){

	ifstream inputFile((path + "Makefile").c_str());

	if(inputFile.fail()){
		inputFile.open((path + "KBuild").c_str());
	}	

	string line;
	int index = -1;
	
	

	if (inputFile.is_open()){

		while(inputFile.good())	{
			index = -1;
			getline(inputFile,line);

			if((line.length() != 0) && (line[0] != '#')){
				index = findKeyword(line);
			}

			if(index != -1){
				parseLine(path, line, dependencyString, index, inputFile);
			}else{
				boost::regex regexp("([^-])+(-)([y|objs])(\\s)*([:=|+=])(.*)");
				if(boost::regex_match( line, regexp )){
					addToCompositeList(line);
				}
				
			}
		}
	}

	inputFile.close();
}


void parseMakeFile(string path){
searchFile(path.c_str(), "");
}

int main(int argc, char ** argv){

	architecture = argv[1];
	//architecture is passed in as an argument
	cout<<"looking at arch"<<architecture<<endl;
  	outputFile.open("models/testx86.makemodel");

	
	//parse base Make File
	parseMakeFile("./");

	//parse architecture make file
	parseMakeFile("./arch/" + architecture +"/");
	outputFile.close();

return 0;
}
