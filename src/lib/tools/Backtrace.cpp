/*****************************************************
             PROJECT  : MALT
             VERSION  : 0.1.0
             DATE     : 03/2015
             AUTHOR   : Valat Sébastien
             LICENSE  : CeCILL-C
*****************************************************/

/********************  HEADERS  *********************/
#include <common/Options.hpp>
#include "Backtrace.hpp"
#include "BacktraceGlibc.hpp"
#include "BacktraceLibunwind.hpp"

/*******************  NAMESPACE  ********************/
namespace MALT
{

/*******************  FUNCTION  *********************/
int Backtrace::backtrace ( void** buffer, int maxDepth )
{
	if (getOptions().stackLibunwind)
		return BacktraceLibunwind::backtrace(buffer,maxDepth);
	else
		return BacktraceGlibc::backtrace(buffer,maxDepth);
}

}
