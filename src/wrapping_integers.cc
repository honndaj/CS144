#include "wrapping_integers.hh"

using namespace std;

const uint64_t P = 1LL << 32;

uint64_t abs( uint64_t a, uint64_t b )
{
  return a > b ? a - b : b - a;
}

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32( uint32_t( ( zero_point.raw_value_ + n ) % P ) );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t delta, k, a, b;
  delta = raw_value_ < zero_point.raw_value_ ? P + raw_value_ : raw_value_;
  delta -= zero_point.raw_value_;
  k = ( checkpoint - delta ) / P;
  a = k * P + delta;
  b = ( k + 1 ) * P + delta;
  return abs( a, checkpoint ) < abs( b, checkpoint ) ? a : b;
}
