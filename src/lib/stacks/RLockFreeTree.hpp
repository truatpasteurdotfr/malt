/*****************************************************
             PROJECT  : MATT
             VERSION  : 0.1.0-dev
             DATE     : 02/2015
             AUTHOR   : Valat Sébastien
             LICENSE  : CeCILL-C
*****************************************************/

#ifndef MATT_RLOCK_FREE_TREE_HPP
#define MATT_RLOCK_FREE_TREE_HPP

/********************  HEADERS  *********************/
//internal
#include <portability/Spinlock.hpp>
//current
#include "StackTree.hpp"
#include "BacktraceStack.hpp"
#include <json/JsonState.h>

/*********************  TYPES  **********************/
namespace htopml
{
	class JsonState;
}

/*******************  NAMESPACE  ********************/
namespace MATT
{

/*********************  CLASS  **********************/
/**
 * Define the node for RLockFreeTree.
**/
struct RLockFreeTreeNode
{
	RLockFreeTreeNode(void*callSite);
	/** Pointer to the next element on the same parent. NULL if none (last element). **/
	RLockFreeTreeNode * next;
	/** Pointer to the parent node, NULL if root. **/
	RLockFreeTreeNode * parent;
	/** Pointer to the first child of the current node. NULL if none. **/
	RLockFreeTreeNode * firstChild;
	/** Define the corresponding call site. **/
	void * callSite;
	/** Optional data attached to the current node, NULL if none. **/
	StackTreeStorage data;
	/**
	 * Short integer ID to keep reproductible output for unit test instead of using data address.
	 * It also produce smaller output than exadecimal addresses. 
	 **/
	int dataId;
};

/*******************  FUNCTION  *********************/
void convertToJson(htopml::JsonState & json, const RLockFreeTreeNode & tree);

/*********************  CLASS  **********************/
/**
 * This class provide a basic tree based on nodes with childs and optional data attached to the nodes.
 * The child list is built with a simple linked list. As we do not consider node deletion we can access to all lists
 * without locks. We use locks only to insert new keys after checking their absence.
 * 
 * This tree might be used for the enter-exit mode in place of StackSTLHashMap.
 * 
 * @brief Short tree storage with lock free read access, no deletion and locked insertion. To be used for enter-exit mode.
**/
class RLockFreeTree : public StackTree
{
	public:
		typedef RLockFreeTreeNode * Handler;
	public:
		RLockFreeTree(bool threadSafe = true);
		~RLockFreeTree(void);
		virtual StackTreeHandler enterFunction(StackTreeHandler handler, void* callsite);
		virtual StackTreeHandler exitFunction(StackTreeHandler handler,void  * callsite);
		virtual StackTreeHandler enterThread(void);
		virtual StackTreeHandler setOnRoot(StackTreeHandler handler);
		virtual void exitThread(StackTreeHandler handler);
		Handler getNode(StackTreeHandler handler);
		virtual StackTreeHandler getFromStack(StackTreeHandler handler, const Stack & stack);
		virtual StackTreeHandler getFromStack(StackTreeHandler handler,int skip);
		virtual void copyData(const Stack & stack,const StackTreeStorage & storage);
		virtual bool isEnterExit ( void ) const;
		virtual void toJson ( htopml::JsonState& json, const StackTree& tree ) const;
	public:
		friend void convertToJson(htopml::JsonState & json, const RLockFreeTree & tree);
	protected:
		virtual void * getData(StackTreeHandler handler,int id);
// 		virtual void setData(StackTreeHandler handler,int id, void* data);
		Handler addChild(Handler node, void* callsite);
		Handler findChild(Handler node, void* callsite);
		virtual void insertChild(RLockFreeTreeNode * parent,RLockFreeTreeNode * child);
		void printData(htopml::JsonState & json,const RLockFreeTreeNode * node,int i) const;
		void printData(htopml::JsonState & json,int i) const;
	protected:
		Spinlock lock;
		bool threadSafe;
		RLockFreeTreeNode root;
		int lastDataId;
};

}

#endif //MATT_RLOCK_FREE_TREE_HPP
