#include "workstack.h"

void
WorkStack::
reset()
{
	top       = base;
	obj_count = 0;
	top_addr  = 0;
}

u8
WorkStack::
get_obj_size(u32 addr)
{
	validate_stack_addr(addr);

    switch (type_record[addr]) {
		case UINT32_CODE:
			return UINT_SIZE;
			break;

		case INT32_CODE:
			return INT_SIZE;
			break;

		case UINT8_CODE:
			return UBYTE_SIZE;
			break;

		default: throw 1;
		}
}
// updates obj_count, top & top_addr vars before pushing or popping
// the stack, "preparing" it for the actual data change.
void
WorkStack::
prepare_stack_top(u8 action, u8 push_datatype)
{
    if (action == PUSH) {
		// check if stack is empty because top & top_addr need
		// no changing if it is because they're already at the 
		// correct value for pushing first item.
		if (!obj_count++) return;

        switch (push_datatype) {
			case UINT32_CODE:
			case INT32_CODE:
				top_addr += UINT_SIZE;
				top      += UINT_SIZE;
				break;

			case UINT8_CODE:
				top_addr += UBYTE_SIZE;
				top      += UBYTE_SIZE;
				break;

			default:
				throw RamError::INVALID_DATATYPE;
		}
    }

    else if (action == POP) {
		// check if we're trying to pop an empty stack.
		if (!obj_count)
			throw WorkStackError::EMPTY_STACK_ACCESS;

		switch (type_record[top_addr]) {
			case UINT32_CODE:
				// remove popped values type-record.
				type_record[top_addr] = NOVALUE_CODE;
				for (int i = 1; i < UINT_SIZE; i++)
					type_record[top_addr + i] = NOVALUE_CODE;

				// set u32ptr to top before updating top and top_addr.
				// this will be used by popping function to return popped value.
				uint_ptr = (u32*)top;

				if (obj_count > 1)
				{
					top_addr -= UINT_SIZE;
					top      -= UINT_SIZE;
				}

				break;

			case INT32_CODE:
				// remove popped values type-record.
				type_record[top_addr] = NOVALUE_CODE;
				for (int i = 1; i < INT_SIZE; i++)
					type_record[top_addr + i] = NOVALUE_CODE;

				// set u32ptr to top before updating top and top_addr.
				// this will be used by popping function to return popped value.
				int_ptr = (i32*) top;
				top_addr -= INT_SIZE;
				top -= INT_SIZE;
				break;

			case UINT8_CODE:
				type_record[top_addr] = NOVALUE_CODE;
				ubyte_ptr = top;
				top_addr--;
				top--;
				break;
		}

        obj_count--;
    }

	// check that updated address is valid.
	validate_stack_addr(top_addr);
}

WorkStack::
WorkStack(u32 memory_size, u8* master_ptr) :
    Ram(memory_size, master_ptr), top_addr(0), obj_count(0)
{
    top = base;
}

void
WorkStack:: // print_workstack_addr
print_ws_addr(u32 addr)
{ 
    validate_stack_addr(addr);

    switch (type_record[addr]) {
		case UINT32_CODE:
			printf(UINT_STK_VALUE_INFOSTR, addr, get_uint(addr));
			return;

		case INT32_CODE:
			printf(INT_STK_VALUE_INFOSTR, addr, get_int(addr));
			return;

		case UINT8_CODE:
			printf(BYTE_STK_VALUE_INFOSTR, addr, get_ubyte(addr));
			return;

		default:
			printf("\nstack[%u] = (unknown) bitstring: ", addr);
			print_bits(*(base + addr));
    }
}

void
WorkStack::
print_top()
{
    if (!obj_count)
        throw WorkStackError::EMPTY_STACK_ACCESS;

    switch (type_record[top_addr]) {
		case UINT32_CODE:
			printf(UINT_STKTOP_VALUE_INFOSTR, get_uint(top_addr));
			return;

		case INT32_CODE:
			printf(INT_STKTOP_VALUE_INFOSTR, get_int(top_addr));
			return;

		case UINT8_CODE:
			printf(BYTE_STKTOP_VALUE_INFOSTR, get_ubyte(top_addr));
			return;

		default:
			printf("\nstack[top] = (unknown) bitstring: ");
			print_bits(*(base + top_addr));
    }
}

u8*
WorkStack::
get_top_ptr() { return top; }

u8*
WorkStack::
get_sectop_ptr()
{
    if (obj_count < 2)
		throw WorkStackError::EMPTY_STACK_ACCESS;

    return (top - get_obj_size(top_addr));
}

void
WorkStack::
push_ubyte(u8 value)
{
	// prep the stack then write the value.
	prepare_stack_top(PUSH, UINT8_CODE);
	*top = value;

	// type-record the pushed value.
	type_record[top_addr] = UINT32_CODE;
	for (int i = 1; i < UINT_SIZE; i++)
		type_record[top_addr + i] = INSIDE_UINT8_PADDING_CODE;
}

void
WorkStack::
push_uint(u32 value)
{
	// prep the stack then write the value.
	prepare_stack_top(PUSH, UINT32_CODE);
	*((u32*) top) = value;

	// type-record the pushed value.
	type_record[top_addr] = UINT32_CODE;
    for (int i = 1; i < UINT_SIZE; i++)
        type_record[top_addr + i] = INSIDE_UINT_CODE;
}

void
WorkStack::
push_int(i32 value)
{
	// prep the stack then write the value.
	prepare_stack_top(PUSH, INT32_CODE);
	*((i32*)top) = value;

	// type-record the pushed value.
	type_record[top_addr] = INT32_CODE;
	for (int i = 1; i < INT_SIZE; i++)
		type_record[top_addr + i] = INSIDE_INT_CODE;
}

u32
WorkStack::
pop_uint()
{
	if (type_record[top_addr] != UINT32_CODE)
		throw WorkStackError::INVALID_DATATYPE_ACCESS;

	prepare_stack_top(POP, NONE);
    return *uint_ptr;
}

u8
WorkStack::
pop_ubyte()
{
	if (type_record[top_addr] != UINT8_CODE)
		throw WorkStackError::INVALID_DATATYPE_ACCESS;

	prepare_stack_top(POP, NONE);
	return *uint_ptr;
}

i32
WorkStack::
pop_int()
{
	if (type_record[top_addr] != INT32_CODE)
		throw WorkStackError::INVALID_DATATYPE_ACCESS;

	prepare_stack_top(POP, NONE);
	return *int_ptr;
}

u32
WorkStack::
read_uint(u32 addr)
{
	validate_stack_addr(addr);

    if (!obj_count)
		throw WorkStackError::EMPTY_STACK_ACCESS;

	if (type_record[addr] != UINT32_CODE)
		throw WorkStackError::INVALID_DATATYPE_ACCESS;

    return *((u32*)(base + addr));
}

i32
WorkStack::
read_int(u32 addr)
{
	validate_stack_addr(addr);

	if (!obj_count)
		throw WorkStackError::EMPTY_STACK_ACCESS;

	if (type_record[addr] != INT32_CODE)
		throw WorkStackError::INVALID_DATATYPE_ACCESS;

	return *((i32*)(base + addr));
}

u8
WorkStack::
read_ubyte(u32 addr)
{
	validate_stack_addr(addr);

	if (!obj_count)
		throw WorkStackError::EMPTY_STACK_ACCESS;

	if (type_record[addr] != UINT8_CODE)
		throw WorkStackError::INVALID_DATATYPE_ACCESS;

	return *(base + addr);
}


u32
WorkStack::
read_top_uint()
{
	if (!obj_count)
		throw WorkStackError::INVALID_ACCESS;

	return *((u32*)top);
}

i32
WorkStack::
read_top_int()
{
	if (!obj_count)
		throw WorkStackError::INVALID_ACCESS;

	return *((i32*)top);
}

u8
WorkStack::
read_top_ubyte()
{
	if (!obj_count)
		throw WorkStackError::INVALID_ACCESS;

	return *top;
}

u32
WorkStack::
read_sectop_uint()
{
    if (obj_count < 2)
        throw WorkStackError::EMPTY_STACK_ACCESS;

	return *((u32*)(top - get_obj_size(top_addr)));
}

i32
WorkStack::
read_sectop_int()
{
	if (obj_count < 2)
		throw WorkStackError::EMPTY_STACK_ACCESS;

	return *((i32*)(top - get_obj_size(top_addr)));
}

u8
WorkStack::
read_sectop_ubyte()
{
	if (obj_count < 2)
		throw WorkStackError::EMPTY_STACK_ACCESS;

	return *((i32*)(top - get_obj_size(top_addr)));
}

void
WorkStack::
remove(u32 count = 1)
{
	if (!obj_count)
		throw WorkStackError::EMPTY_STACK_ACCESS;

    for (int i = 0; i < count; i++)
        prepare_stack_top(POP, NOTYPE_CODE);

}

void
WorkStack::
validate_stack_addr(u32 addr)
{
	if ((addr >= memory_size) || addr > top_addr)
		throw RamError::INVALID_ADDR;
}

void workstack_Test()
{
	printf("\nstarting workstack test..\n");
	WorkStack* ws = new WorkStack(100, (u8*) malloc(100));

	u32 z = 0;

	ws->push_uint(666);

	newline();
	try { ws->print_top(); } catch (...) { printf("\nworkstack empty."); }

	ws->push_uint(682);

	newline();
	try { ws->print_top(); }
	catch (...) { printf("\nworkstack empty."); }

	z = ws->pop_uint();

	newline();
	try { ws->print_top(); printf("HERE"); }
	catch (...) { printf("\nworkstack empty."); }

	printf("\n><><>> %u", z);
	delete ws;
	printf("\n\ncompleting workstack test..\n\n");
}

//int main() { workstack_Test(); return 0; }