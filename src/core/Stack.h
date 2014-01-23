#ifndef ATT_STACK_H
#define ATT_STACK_H

/********************  HEADERS  *********************/
#include <ostream>
#include <stdint.h>
#include "FuncNameDic.hpp"
// #include <json/JsonState.h>

/*******************  FUNCTION  *********************/
typedef uint64_t StackHash;

/*******************  FUNCTION  *********************/
namespace htopml
{
	class JsonState;
};

/*******************  NAMESPACE  ********************/
namespace ATT
{

/*********************  ENUM  ***********************/
enum StackOrder
{
	STACK_ORDER_ASC,
	STACK_ORDER_DESC,
};

/*********************  CLASS  **********************/
class Stack
{
	public:
		Stack(StackOrder order);
		Stack(void ** stack,int size,StackOrder order);
		Stack(const Stack & orig);
		virtual ~Stack(void);
		StackHash hash(void) const;
		static StackHash hash(void ** calls,int size);
		void resolveSymbols(FuncNameDic & dic) const;
		void grow(void);
		bool isValid(void) const;
		int getSize(void) const;
		void set(void ** stack, int size,StackOrder order);
		void set(const Stack & orig);
		void * getCaller(void) const;
		void * getCallee(void) const;
	public:
		friend std::ostream & operator << (std::ostream & out,const Stack & tracer);
		friend void typeToJson(htopml::JsonState & json,std::ostream& stream, const Stack & value);
		friend bool operator == (const Stack & v1,const Stack & v2);
	protected:
		void ** stack;
		int size;
		int memSize;
	private:
		StackOrder order;
};

}

#endif //ATT_STACK_H