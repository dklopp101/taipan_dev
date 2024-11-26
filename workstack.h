#ifndef WORKSTACK_H
#define WORKSTACK_H

#include "ram.h"
#include "vm.h"

// used by prepare_stack_top() in workstack.cpp
// 
// prepare_stack_top() updates obj_count, top and top_addr
// vars in preparation for pushing or popping, these macro'd
// integers' are used to tell the function which of those two
// actions are going to be performed and what prep action
// needs to take place by prepare_stack_top() before the actual data change.
#define PUSH 50
#define POP  60

enum class WorkStackError
{
	GENERAL_ERR = 1,
	INVALID_DATATYPE_ACCESS,
	EMPTY_STACK_ACCESS,
	INVALID_ACCESS
};

struct WorkStack : public Ram
{
    u8* top;
    u32 obj_count;
    u32 top_addr;

    WorkStack(u32 memory_size, u8* master_ptr);

	u8 get_obj_size(u32 addr);
	void prepare_stack_top(u8 action, u8 push_datatype);

    void print_ws_addr(u32 addr);
    void print_top();

    u8* get_top_ptr();
    u8* get_sectop_ptr();

    void push_ubyte(u8 value);
    void push_uint(u32 value);
	void push_int(i32 value);

    u32 pop_uint();
    u8 pop_ubyte();
    i32 pop_int();

    u32 read_uint(u32 addr);
    i32 read_int(u32 addr);
	u8 read_ubyte(u32 addr);

	u32 read_top_uint();
    u8 read_top_ubyte();
    i32 read_top_int();

	u32 read_sectop_uint();
    i32 read_sectop_int();
	u8  read_sectop_ubyte();

    void remove(u32 count);
	void validate_stack_addr(u32 addr);
	void reset();
};

void workstack_Test();

//int main();

#endif // WORKSTACK_H