#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  route_table_.push_back( Route_table_entry( route_prefix, prefix_length, next_hop, interface_num ) );
  if ( route_prefix == 0 && prefix_length == 0 ) {
    default_entry_ = route_table_.size() - 1;
  }
}

void Router::route()
{
  for ( size_t j = 0; j < interfaces_.size(); ++j ) {
    optional<InternetDatagram> op_dgram;
    while ( ( op_dgram = interface( j ).maybe_receive() )
              .has_value() ) { // 这也是个坑，要用引用，不然dgram不会被读取，但是这是为什么?
      InternetDatagram dgram = op_dgram.value();
      size_t max_match_len = 0, match_index = 0;

      if ( dgram.header.ttl <= 1 ) {
        continue;
      }

      for ( size_t i = 0; i < route_table_.size(); i++ ) {
        Route_table_entry entry = route_table_[i];
        if ( entry.route_prefix == 0 && entry.prefix_length == 0 ) {
          continue;
        }
        size_t match_len = 0;
        uint32_t t = dgram.header.dst ^ entry.route_prefix;
        while ( match_len < entry.prefix_length && ( t & ( 1 << ( 31 - match_len ) ) ) == 0 ) {
          match_len++;
        }
        if ( match_len != entry.prefix_length ) {
          match_len = 0;
        }
        if ( match_len > max_match_len ) {
          max_match_len = match_len;
          match_index = i;
        }
      }
      if ( max_match_len == 0 ) {
        max_match_len = 32;
        match_index = default_entry_;
      }
      dgram.header.ttl--;
      dgram.header.compute_checksum(); // 修改校验和没注意到，看别人代码才知道的
      if (
        max_match_len
        && dgram.header
             .ttl ) { // 这里一开始没注意，把从某接口进来帧给发回该接口了，应该发给路由表项对应的接口，debug了好久
        interfaces_[route_table_[match_index].interface_num].send_datagram(
          dgram,
          route_table_[match_index].next_hop.has_value() ? route_table_[match_index].next_hop.value()
                                                         : ( Address::from_ipv4_numeric( dgram.header.dst ) ) );
        // cerr << (route_table_[match_index].next_hop.has_value() ? route_table_[match_index].next_hop.value().ip()
        // : (Address::from_ipv4_numeric(dgram.header.dst)).ip())<<endl; cerr << dgram.header.ttl<< endl;
      }
    }
  }
}
