#include "pch.h"

#include "OutputDebugStream.h"

std::streambuf::int_type OutputDebugStringBuf::overflow(int_type c)
{
	if (c != traits_type::eof())
	{
		char buf[2] = { static_cast<char>(c), '\0' };
		OutputDebugStringA(buf);
	}
	return c;
}

std::streamsize OutputDebugStringBuf::xsputn(const char* s, std::streamsize n)
{
	std::string str(s, n);
	OutputDebugStringA(str.c_str());
	return n;
}

OutputDebugStream::OutputDebugStream()
      : std::ostream(&buf)
{
}
