/********************  HEADERS  *********************/
#include <cassert>
#include <dlfcn.h>
#include <cstdlib>
#include <execinfo.h>
#include <cstdio>
#include "FuncNameDic.h"

/*******************  FUNCTION  *********************/
FuncNameDic::FuncNameDic(void )
{

}

/*******************  FUNCTION  *********************/
FuncNameDic::~FuncNameDic(void )
{
// 	for (FuncNameDicMap::const_iterator it = nameMap.begin() ; it != nameMap.end() ; ++it)
// 		free((void*)it->second);
}

/*******************  FUNCTION  *********************/
const char* FuncNameDic::getName(void* callSite)
{
	//errors
	assert(callSite != NULL);

	//search in map
	FuncNameDicMap::const_iterator it = this->nameMap.find(callSite);

	//if not found, build new entry
	if (it == nameMap.end())
	{
		return setupNewEntry(callSite);
	} else {
		return it->second;
	}
}

/*******************  FUNCTION  *********************/
const char* FuncNameDic::setupNewEntry(void* callSite)
{
	//errors
	assert(callSite != NULL);

	//use backtrace_symbol to extract name and line
	char ** tmp = backtrace_symbols(&callSite,1);
	//TODO move to assume
	assert(tmp != NULL);
	assert(tmp[0] != NULL);

	nameMap[callSite] = tmp[0];

	return tmp[0];
}

/*******************  FUNCTION  *********************/
std::ostream& operator<<(std::ostream& out, const FuncNameDic& dic)
{
	for (FuncNameDicMap::const_iterator it = dic.nameMap.begin() ; it != dic.nameMap.end() ; ++it)
		out << it->first << " " << it->second << std::endl;
	return out;
}

/*******************  FUNCTION  *********************/
void typeToJson(htopml::JsonState& json, std::ostream& stream, const FuncNameDic& value)
{
	json.openStruct();

	for (FuncNameDicMap::const_iterator it = value.nameMap.begin() ; it != value.nameMap.end() ; ++it)
	{
		char buffer[64];
		sprintf(buffer,"%p",it->first);
		json.printField(buffer,it->second);
	}

	json.closeStruct();
}