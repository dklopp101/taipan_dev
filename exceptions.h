#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "infostrings.h"
#include "vm.h"

#define ERR_MSG_MAXLEN            500
#define WORKSTACK_ERR_MSG         "\nWorkStack Error: %s"
#define ADDRESS_ERR_MSG           "\nAddress Error: %s"
#define TYPE_MISMATCH_INFOSTR     "\nram[%u] = %s not %s"
#define INVALID_RAM_ADDR_INFOSR   "\nram address %u is invalid, memory-size: %u"
#define EMPTY_STK_INFOSTR         "\nno objects on stack to print."
#define INTERNAL_ERR_MSG_STR      "\nINTERNAL ERROR: %s"

class InvalidRamAddrException : public std::exception
{
private:
    std::string message;

public:

    InvalidRamAddrException(uint32_t invalid_addr, uint32_t memory_size)
    {
        char msg[55];
        sprintf(msg, INVALID_RAM_ADDR_INFOSR, invalid_addr, memory_size);
        message = msg;
    }

    const char*
        what() const noexcept override
    {
        return message.c_str();
    }
};

class TypeMismatchException : public std::exception
{
private:
    std::string message;

public:

    TypeMismatchException(uint8_t given_type, uint8_t actual_type, uint32_t addr)
    {
        char msg[ERR_MSG_MAXLEN];
        sprintf(msg, TYPE_MISMATCH_INFOSTR, addr, datatype_str[actual_type], datatype_str[given_type]);
        message = msg;
    }

    const char*
        what() const noexcept override
    {
        return message.c_str();
    }
};

class InternalErrorException : public std::exception
{
private:
    std::string message;

public:
    InternalErrorException(const char* _msg)
    {
        char msg[ERR_MSG_MAXLEN];
        sprintf(msg, INTERNAL_ERR_MSG_STR, _msg);
        message = msg;
    }

    const char*
        what() const noexcept override
    {
        return message.c_str();
    }
};

class AddressException : public std::exception
{
private:
    std::string message;

public:
    AddressException(const char* _msg)
    {
        char msg[ERR_MSG_MAXLEN];
        sprintf(msg, ADDRESS_ERR_MSG, _msg);
        message = msg;
    }

    const char*
        what() const noexcept override
    {
        return message.c_str();
    }
};

class WorkStackException : public std::exception
{
private:
    std::string message;

public:
    WorkStackException(const char* _msg)
    {
        char msg[ERR_MSG_MAXLEN];
        sprintf(msg, WORKSTACK_ERR_MSG, _msg);
        message = msg;
    }

    const char*
        what() const noexcept override
    {
        return message.c_str();
    }
};


#endif // EXCPETIONS.H