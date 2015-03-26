/*****************************************************
             PROJECT  : MALT
             VERSION  : 0.1.0
             DATE     : 03/2015
             AUTHOR   : Valat Sébastien
             LICENSE  : CeCILL-C
*****************************************************/

/********************  HEADERS  *********************/
#include "ELFReader.hpp"
//standard
#include <cstdio>
#include <cassert>
//internals
#include <common/Debug.hpp>

/*******************  NAMESPACE  ********************/
namespace MALT
{

/*******************  FUNCTION  *********************/
ElfReader::ElfReader(const std::string& file)
{
	this->elf = NULL;
	this->fp = NULL;
}

/*******************  FUNCTION  *********************/
ElfReader::~ElfReader(void)
{
}

/*******************  FUNCTION  *********************/
void ElfReader::loadGlobalVariables(ElfGlobalVariableVector& variables)
{
}

/*******************  FUNCTION  *********************/
void convertToJson(htopml::JsonState& json, const ElfGlobalVariable& value)
{
}

/*******************  FUNCTION  *********************/
bool ElfReader::hasLibElf(void)
{
	return false;
}

}
