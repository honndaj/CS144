#include <iostream>
#include <stdexcept>

#include "byte_stream.hh"

using namespace std;
ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  if ( is_closed() || available_capacity() == 0 || data.empty() ) {
    return;
  }
  auto len = min( data.size(), available_capacity() );
  if ( len < data.size() ) {
    data = data.substr( 0, len );
  }
  string_deque_.push_back( std::move( data ) );
  if ( string_deque_.size() == 1 ) {
    view_front_ = string_view( string_deque_.front() );
  }
  byte_in_ += len;
  byte_bufferd_ += len;
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
  return capacity_ - byte_bufferd_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return byte_in_;
}

string_view Reader::peek() const
{
  // Your code here.
  if ( string_deque_.empty() ) {
    return {};
  }
  return view_front_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return string_deque_.empty() && is_closed_;
}

bool Reader::has_error() const
{
  // Your code here.
  return has_error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  len = min( len, byte_bufferd_ );

  while ( len > 0 && !string_deque_.empty() ) {
    uint64_t front_size = view_front_.size();
    if ( front_size > len ) {
      view_front_.remove_prefix( len );
      byte_out_ += len;
      byte_bufferd_ -= len;
      return;
    }
    len -= front_size;
    byte_out_ += front_size;
    byte_bufferd_ -= front_size;
    string_deque_.pop_front();
    view_front_ = string_view( string_deque_.front() );
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return byte_bufferd_;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return byte_out_;
}
