#include "Register.h"

#include "Assert.h"

Register Register::forLocal(uint32_t offset)
{
    ASSERT(offset, "Invalid register offset");
    return Register { static_cast<int32_t>(offset) };
}

Register Register::forParameter(uint32_t offset)
{
    //ASSERT(offset, "Invalid register offset");
    return Register { -static_cast<int32_t>(offset) };
}

Register Register::invalid()
{
    return Register { s_invalidOffset };
}

bool Register::operator==(Register other)
{
    return m_offset == other.m_offset;
}

Register::Register(int32_t offset)
    : m_offset(offset)
{
}
