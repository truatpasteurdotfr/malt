/*****************************************************
             PROJECT  : MATT
             VERSION  : 0.1.0-dev
             DATE     : 02/2015
             AUTHOR   : Valat Sébastien
             LICENSE  : CeCILL-C
*****************************************************/

/********************  HEADERS  *********************/
#include <analysis/ProcessLevelAnalysis.hpp>
#include <analysis/ThreadLevelAnalysis.hpp>
#include <hooks/EnterExitFunctionHooks.hpp>
#include <hooks/MallocHooksFake.hpp>
#include <common/Options.hpp>
#include <common/NoFreeAllocator.hpp>
#include <common/SimpleAllocator.hpp>
#include <portability/OS.hpp>

/*******************  FUNCTION  *********************/
namespace MATT
{

/********************  GLOBALS  *********************/
static ProcessLevelAnalysis * gblMatt = NULL;
static __thread ThreadLevelAnalysis * tlsMatt = NULL;
static bool gblIsInInit = false;
static MallocHooksFake gblMallocHooksFake;

/*******************  FUNCTION  *********************/
void optionsInit(void)
{
	//load options
	Options & options = initGlobalOptions();
	const char * configFile = OS::getEnv("MATT_CONFIG");
	if (configFile != NULL)
		options.loadFromFile(configFile);
	const char * envOptions = OS::getEnv("MATT_OPTIONS");
	if (envOptions != NULL)
		options.loadFromString(envOptions);
}

/*******************  FUNCTION  *********************/
void doGlobalInit(void)
{
	gblIsInInit = true;
	if (gblInternaAlloc == NULL)
		gblInternaAlloc = new SimpleAllocator();
	doNoFreeAllocatorInit();
	optionsInit();
	gblMatt = new ProcessLevelAnalysis();
	gblIsInInit = false;
}

/*******************  FUNCTION  *********************/
ThreadHooks * threadHookInit(void)
{
	if (gblIsInInit)
		return NULL;
	if (gblMatt == NULL)
		doGlobalInit();
	return gblMatt;
}

/*******************  FUNCTION  *********************/
MallocHooks * mallocHookInit(void)
{
	if (gblIsInInit)
		return &gblMallocHooksFake;
	ThreadLevelAnalysis * tls = tlsMatt;
	if (gblMatt == NULL)
		doGlobalInit();
	if (tls == NULL)
	{
		tls = tlsMatt = gblMatt->getNewThreadLevelAnalysis();
	}
	return tls;
}

/*******************  FUNCTION  *********************/
MmapHooks * mmapHookInit(void)
{
	if (gblIsInInit)
		return NULL;
	if (gblMatt == NULL)
		doGlobalInit();
	return gblMatt;
}

/*******************  FUNCTION  *********************/
ExitHooks * exitHookInit(void)
{
	if (gblIsInInit)
		return NULL;
	if (gblMatt == NULL)
		doGlobalInit();
	return gblMatt;
}

/*******************  FUNCTION  *********************/
EnterExitFunctionHooks * enterExitFunctionHookInit(void)
{
	if (gblIsInInit)
		return NULL;
	ThreadLevelAnalysis * tls = tlsMatt;
	if (gblMatt == NULL)
		doGlobalInit();
	if (tls == NULL)
	{
		tls = tlsMatt = gblMatt->getNewThreadLevelAnalysis();
	}
	return tls;
}

}
