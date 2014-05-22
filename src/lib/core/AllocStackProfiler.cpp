/*****************************************************
             PROJECT  : MATT
             VERSION  : 0.1.0-dev
             DATE     : 01/2014
             AUTHOR   : Valat Sébastien
             LICENSE  : CeCILL-C
*****************************************************/

/********************  HEADERS  *********************/
//standard
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
//extension GNU
#include <execinfo.h>
//from htopml
#include <json/ConvertToJson.h>
//internal potability
#include <portability/OS.hpp>
//internal commons
#include <common/Debug.hpp>
#include <common/Helpers.hpp>
#include <common/CodeTiming.hpp>
#include <common/SimpleAllocator.hpp>
#include <common/FormattedMessage.hpp>
//locals
#include "AllocStackProfiler.hpp"
#include "LocalAllocStackProfiler.hpp"

/********************  MACROS  **********************/
#define MATT_SKIP_DEPTH 3

/*******************  NAMESPACE  ********************/
namespace MATT
{

/*******************  FUNCTION  *********************/
AllocStackProfiler::AllocStackProfiler(const Options & options,StackMode mode,bool threadSafe)
	:requestedMem(options.timeProfilePoints,options.timeProfileLinear),largestStack(STACK_ORDER_DESC)
{
	this->mode = mode;
	this->threadSafe = threadSafe;
	this->options = options;
	this->largestStackSize = 0;
	
	//init internal alloc
	if (gblInternaAlloc == NULL)
		gblInternaAlloc = new SimpleAllocator(true);
	
	//init tref to convert ticks in sec
	gettimeofday(&trefSec,NULL);
	trefTicks = getticks();
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::onMalloc(void* ptr, size_t size,Stack * userStack)
{
	onAllocEvent(ptr,size,userStack);
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::onCalloc(void* ptr, size_t nmemb, size_t size,Stack * userStack)
{
	onAllocEvent(ptr,size * nmemb,userStack);
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::onFree(void* ptr,Stack * userStack)
{
	if (ptr != NULL)
		onFreeEvent(ptr,userStack);
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::onPrepareRealloc(void* oldPtr,Stack * userStack)
{
	//nothing to unregister, skip
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::onRealloc(void* oldPtr, void* ptr, size_t newSize,Stack * userStack)
{
	MATT_OPTIONAL_CRITICAL(lock,threadSafe)
		//to avoid to search it 2 times
		MMCallStackNode callStackNode;
		size_t oldSize = 0;
		
		//free part
		if (oldPtr != NULL)
			oldSize = onFreeEvent(oldPtr,userStack,&callStackNode,false);
		
		//alloc part
		if (newSize > 0)
			onAllocEvent(ptr,newSize,userStack,&callStackNode,false);
		
		//register size jump
		if (options.distrReallocJump)
		{
			ReallocJump jump = {oldSize,newSize};
			ReallocJumpMap::iterator it = reallocJumpMap.find(jump);
			if (it == reallocJumpMap.end())
				reallocJumpMap[jump] = 1;
			else
				it->second++;
		}
	MATT_END_CRITICAL
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::onAllocEvent(void* ptr, size_t size,Stack* userStack,MMCallStackNode * callStackNode,bool doLock)
{
	//locals
	MMCallStackNode localCallStackNode;
	if (callStackNode == NULL)
		callStackNode = &localCallStackNode;
	
	MATT_OPTIONAL_CRITICAL(lock,threadSafe && doLock)
		//update mem usage
		if (options.timeProfileEnabled)
		{
			segments.onDeltaEvent(1);
			requestedMem.onDeltaEvent(size);
			if (virtualMem.isNextPoint())
			{
				OSMemUsage mem = OS::getMemoryUsage();
				virtualMem.onUpdateValue(mem.virtualMemory - gblInternaAlloc->getTotalMemory());
				physicalMem.onUpdateValue(mem.physicalMemory - gblInternaAlloc->getTotalMemory());
			}
		}
	
		if (options.stackProfileEnabled)
		{
			//search if not provided
			if (!callStackNode->valid())
				*callStackNode = getStackNode(userStack);
			
			//count events
			CODE_TIMING("updateInfoAlloc",callStackNode->infos->onAllocEvent(size));
		}

		//register for segment history tracking
		if (ptr != NULL)
			CODE_TIMING("segTracerAdd",segTracker.add(ptr,size,*callStackNode));
		
		//update size map
		if (options.distrAllocSize)
		{
			AllocSizeDistrMap::iterator it = sizeMap.find(size);
			if (it == sizeMap.end())
				sizeMap[size] = 1;
			else
				it->second++;
		}
	
		//update intern mem usage
		if (options.timeProfileEnabled)
			internalMem.onUpdateValue(gblInternaAlloc->getInuseMemory());
	MATT_END_CRITICAL
}

/*******************  FUNCTION  *********************/
size_t AllocStackProfiler::onFreeEvent(void* ptr, MATT::Stack* userStack, MMCallStackNode* callStackNode, bool doLock)
{
	//locals
	size_t size = 0;
	MMCallStackNode localCallStackNode;
	if (callStackNode == NULL)
		callStackNode = &localCallStackNode;

	MATT_OPTIONAL_CRITICAL(lock,threadSafe && doLock)
		//update memory usage
		if (options.timeProfileEnabled && virtualMem.isNextPoint())
		{
			OSMemUsage mem = OS::getMemoryUsage();
			virtualMem.onUpdateValue(mem.virtualMemory - gblInternaAlloc->getTotalMemory());
			physicalMem.onUpdateValue(mem.physicalMemory - gblInternaAlloc->getTotalMemory());
		}

		//search segment info to link with previous history
		SegmentInfo * segInfo = NULL;
		if (options.timeProfileEnabled || options.stackProfileEnabled)
			CODE_TIMING("segTracerGet",segInfo = segTracker.get(ptr));
		
		//check unknown
		if (segInfo == NULL)
		{
			//fprintf(stderr,"Caution, get unknown free segment : %p, ingore it.\n",ptr);
			return 0;
		}
			
		//update mem usage
		size = segInfo->size;
		ticks lifetime = segInfo->getLifetime();
		if (options.timeProfileEnabled)
			requestedMem.onDeltaEvent(-size);
		
		if (options.stackProfileEnabled)
		{
			//search call stack info if not provided
			if (!callStackNode->valid())
				*callStackNode = getStackNode(userStack);
			
			//count events
			CODE_TIMING("updateInfoFree",callStackNode->infos->onFreeEvent(size));
			
			//update alive (TODO, need to move this into a new function on StackNodeInfo)
			segInfo->callStack.infos->onFreeLinkedMemory(size,lifetime);
		}
		
		//remove tracking info
		CODE_TIMING("segTracerRemove",segTracker.remove(ptr));
		
		//update intern mem usage
		if (options.timeProfileEnabled)
		{
			segments.onDeltaEvent(-1);
			internalMem.onUpdateValue(gblInternaAlloc->getInuseMemory());
		}
	MATT_END_CRITICAL
	
	return size;
}

/*******************  FUNCTION  *********************/
MMCallStackNode AllocStackProfiler::getStackNode(Stack* userStack)
{
	MMStackMap::Node * node;
	CODE_TIMING("searchInfo",node = &stackTracer.getNode(*userStack));
	MMCallStackNode res(node->first.stack,&node->second);
	return res;
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::resolvePerThreadSymbols()
{
	for (LocalAllocStackProfilerList::const_iterator it = perThreadProfiler.begin() ; it != perThreadProfiler.end() ; ++it)			
		(*it)->resolveSymbols(symbolResolver);
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::onExit(void )
{
	MATT_OPTIONAL_CRITICAL(lock,threadSafe)
		//resolve symbols
		if (options.stackResolve)
			CODE_TIMING("resolveSymbols",
				this->symbolResolver.loadProcMap();
				this->resolvePerThreadSymbols();
				this->stackTracer.resolveSymbols(symbolResolver);
				this->symbolResolver.resolveNames()
			);
	
		//open output file
		//TODO manage errors
		std::ofstream out;
		
		//config
		if (options.outputDumpConfig)
		{
			options.dumpConfig(FormattedMessage(options.outputName).arg(OS::getExeName()).arg(Helpers::getFileId()).arg("ini").toString().c_str());
		}
		
		//lua
		if (options.outputLua)
		{
			out.open(FormattedMessage(options.outputName).arg(OS::getExeName()).arg(OS::getPID()).arg("lua").toString().c_str());
			CODE_TIMING("outputLua",htopml::convertToLua(out,*this,options.outputIndent));
			out.close();
		}

		//json
		if (options.outputJson)
		{
			out.open(FormattedMessage(options.outputName).arg(OS::getExeName()).arg(OS::getPID()).arg("json").toString().c_str());
			CODE_TIMING("outputJson",htopml::convertToJson(out,*this,options.outputIndent));
			out.close();
		}

		//valgrind out
		if (options.outputCallgrind)
		{
			fprintf(stderr,"Prepare valgrind output...\n");
			ValgrindOutput vout;
			
			for (StackSTLHashMap<CallStackInfo>::const_iterator itMap = stackTracer.begin() ; itMap != stackTracer.end() ; ++itMap)
				vout.pushStackInfo(*(itMap->first.stack),itMap->second,symbolResolver);
			
			//stackTracer.fillValgrindOut(vout,symbolResolver);
			CODE_TIMING("outputCallgrind",vout.writeAsCallgrind(FormattedMessage(options.outputName).arg(OS::getExeName()).arg(OS::getPID()).arg("callgrind").toString(),symbolResolver));
		}

		//print timings
		#ifdef MATT_ENABLE_CODE_TIMING
		CodeTiming::printAll();
		gblInternaAlloc->printState();
		#endif //MATT_ENABLE_CODE_TIMING
	MATT_END_CRITICAL
}

/*******************  FUNCTION  *********************/
void convertToJson(htopml::JsonState& json, const AllocStackProfiler& value)
{
	json.openStruct();

	if (value.options.stackProfileEnabled)
		json.printField("stackInfo",value.stackTracer);
	
	if (value.options.stackResolve)
		json.printField("sites",value.symbolResolver);

	if (value.options.timeProfileEnabled)
	{
		json.printField("requestedMem",value.requestedMem);
		json.printField("physicalMem",value.physicalMem);
		json.printField("virtualMem",value.virtualMem);
		json.printField("internalMem",value.internalMem);
		json.printField("segments",value.segments);
	}
	
	if (value.options.maxStackEnabled)
	{
		json.openFieldArray("threads");
		for (LocalAllocStackProfilerList::const_iterator it = value.perThreadProfiler.begin() ; it != value.perThreadProfiler.end() ; ++it)			
			json.printValue(**it);
		json.closeFieldArray("threads");
// 		json.openFieldStruct("maxStack");
// 		json.printField("size",value.largestStackSize);
// 		json.printField("stack",value.largestStack);
// 		json.printField("mem",value.largestStackMem);
// 		json.printField("total",value.largestStackMem.getTotalSize());
// 		json.closeFieldStruct("maxStack");
	}
	
	if (value.options.distrAllocSize)
	{
		json.openFieldStruct("sizeMap");
		for (AllocSizeDistrMap::const_iterator it = value.sizeMap.begin() ; it != value.sizeMap.end() ; ++it)			
		{
			std::stringstream out;
			out << it->first;
			json.printField(out.str().c_str(),it->second);
		}
		json.closeFieldStruct("sizeMap");
	}
	
	if (value.options.distrReallocJump)
	{
		json.openFieldArray("reallocJump");
		for (ReallocJumpMap::const_iterator it = value.reallocJumpMap.begin() ; it != value.reallocJumpMap.end() ; ++it)			
		{
			json.printListSeparator();
			json.openStruct();
			json.printField("oldSize",it->first.oldSize);
			json.printField("newSize",it->first.newSize);
			json.printField("count",it->second);
			json.closeStruct();
		}
		json.closeFieldArray("reallocJump");
	}
	
	json.printField("leaks",value.segTracker);
	CODE_TIMING("ticksPerSecond",json.printField("ticksPerSecond",value.ticksPerSecond()));
	json.closeStruct();
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::onLargerStackSize(const StackSizeTracker& stackSizes, const Stack& stack)
{
	//current is smaller
	if (stackSizes.getSize() < this->largestStackSize)
		return;
	
	//save
	MATT_OPTIONAL_CRITICAL(largestStackLock,threadSafe)
		this->largestStackSize = stackSizes.getSize();
		this->largestStackMem = stackSizes;
		this->largestStack = stack;
		//ATT_DEBUG("update largestStack");
	MATT_END_CRITICAL;
}

/*******************  FUNCTION  *********************/
const Options* AllocStackProfiler::getOptions(void) const
{
	return &options;
}

/*******************  FUNCTION  *********************/
void AllocStackProfiler::registerPerThreadProfiler(LocalAllocStackProfiler* profiler)
{
	//errors
	MATT_ASSERT(profiler != NULL);
	
	//insert in list
	MATT_OPTIONAL_CRITICAL(lock,threadSafe)
		this->perThreadProfiler.push_back(profiler);
	MATT_END_CRITICAL;
}

/*******************  FUNCTION  *********************/
ticks AllocStackProfiler::ticksPerSecond(void) const
{
	timeval tSec;
	ticks tTicks;
	ticks res;
	
	//read
	tTicks = getticks();
	gettimeofday(&tSec,NULL);
	
	//compute delta and store
	timeval delta;
	timersub(&tSec,&trefSec,&delta);
	
	//if too chost, sleep a little and return
	if (delta.tv_sec == 0 && delta.tv_usec < 200000)
	{
		fprintf(stderr,"MATT : Using usleep to get better ticks <-> seconds conversion !\n");
		usleep(200000);
		res = this->ticksPerSecond();
	} else {
		res = (double)(tTicks-trefTicks)/((double)delta.tv_sec + (double)delta.tv_usec/(double)1000000.0);
	}
	
	return res;
}

}