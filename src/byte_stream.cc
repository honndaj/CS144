#include <stdexcept>
#include <iostream>

#include "byte_stream.hh"

using namespace std;
ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), s_() ,has_error_(false), is_closed_(false), byte_in_(0), byte_out_(0) {}

void Writer::push( string data )
{
  // Your code here.
  if(is_closed()) {
    return ;
  }
  auto len = data.size();
  if(len > capacity_ - s_.size()){
    len = capacity_ - s_.size();
  }
  s_.append(data.substr(0, len));
  byte_in_ += len;
}

void Writer::close()
{
  // Your code here.
  is_closed_ = true;
}

void Writer::set_error()
{
  // Your code here.
  has_error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return is_closed_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - s_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return byte_in_;
}

string_view Reader::peek() const
{
  // Your code here.
  string_view sv(s_);
  return sv;
}

bool Reader::is_finished() const
{
  // Your code here.
  return s_.size() == 0 && is_closed_;
}

bool Reader::has_error() const
{
  // Your code here.
  return has_error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  s_ = s_.substr(len);
  byte_out_ += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return s_.size();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return byte_out_;
}
