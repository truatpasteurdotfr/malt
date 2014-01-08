/********************  HEADERS  *********************/
#include "SimpleCallStackNode.h"

/*******************  FUNCTION  *********************/
SimpleCallStackNode::SimpleCallStackNode(void** callStack, int size)
	:callStack(callStack,size)
{

}

/*******************  FUNCTION  *********************/
std::ostream& operator<<(std::ostream& out, const SimpleCallStackNode& tracer)
{
	out << "[ " << tracer.callStack << " ] " << tracer.info;
	return out;
}

/*******************  FUNCTION  *********************/
void typeToJson(htopml::JsonState& json, std::ostream& stream, const SimpleCallStackNode& value)
{
	json.openStruct();
	json.printField("stack",value.callStack);
	json.printField("infos",value.info);
	json.closeStruct();
}