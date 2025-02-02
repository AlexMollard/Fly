#pragma once

class OutputDebugStringBuf : public std::streambuf
{
protected:
    virtual int_type overflow(int_type c = traits_type::eof()) override;
    virtual std::streamsize xsputn(const char* s, std::streamsize n) override;
};

class OutputDebugStream : public std::ostream
{
public:
    OutputDebugStream();

private:
    OutputDebugStringBuf buf;
};