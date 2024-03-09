#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t raw_ip_address = next_hop.ipv4_numeric();
  auto entry = arp_cache.find( raw_ip_address );
  if ( entry != arp_cache.end() && entry->second.second < 30000 ) { //有缓存且未超时
    frame_queue.push( std::move( EthernetFrame(
      EthernetHeader( entry->second.first, ethernet_address_, EthernetHeader::TYPE_IPv4 ), serialize( dgram ) ) ) );
  } else { 

    if(timer_4_arp_request_.find(raw_ip_address) == timer_4_arp_request_.end() || timer_4_arp_request_[raw_ip_address] >= 5000) {
      timer_4_arp_request_[raw_ip_address] = 0;
      ARPMessage arp_msg = {};
      arp_msg.sender_ethernet_address = ethernet_address_;
      arp_msg.sender_ip_address = ip_address_.ipv4_numeric();
      arp_msg.target_ip_address = raw_ip_address;
      arp_msg.opcode = 1; // 1代表request
      frame_queue.push( std::move( EthernetFrame(
        EthernetHeader( ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP ), serialize( arp_msg ) ) ) );
    }
    dgram_queue[raw_ip_address].push_back( dgram );
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return nullopt;
  }

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram ret = {};
    if ( parse( ret, frame.payload ) ) {
      return ret;
    } else {
      return nullopt;
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp_recv_msg = {};
    if ( parse( arp_recv_msg, frame.payload ) ) {
      arp_cache[arp_recv_msg.sender_ip_address] = make_pair( arp_recv_msg.sender_ethernet_address, 0 );
      if ( arp_recv_msg.opcode == 1 ) { // 如果收到的是 ARP request
        if(arp_recv_msg.target_ip_address != ip_address_.ipv4_numeric()) {
          return nullopt;
        }
        ARPMessage arp_send_msg = {};
        arp_send_msg.sender_ethernet_address = ethernet_address_;
        arp_send_msg.sender_ip_address = ip_address_.ipv4_numeric();
        arp_send_msg.target_ethernet_address = arp_recv_msg.sender_ethernet_address;
        arp_send_msg.target_ip_address = arp_recv_msg.sender_ip_address;
        arp_send_msg.opcode = 2;
        frame_queue.push( std::move( EthernetFrame(
          EthernetHeader( arp_send_msg.target_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP ),
          serialize( arp_send_msg ) ) ) );
      } else if ( arp_recv_msg.opcode == 2 ) { // 如果收到的是 ARP reply，就可以发送dgram_queue中暂存的dgram
        auto it = dgram_queue.find( arp_recv_msg.sender_ip_address );
        if ( it != dgram_queue.end() ) {
          vector<IPv4Datagram>& dgram_vec = it->second;
          for ( size_t i = 0; i < dgram_vec.size(); i++ ) {
            frame_queue.push( std::move( EthernetFrame(
              EthernetHeader( arp_recv_msg.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_IPv4 ),
              serialize( dgram_vec[i] ) ) ) );
          }
          dgram_queue.erase( it );
        }
      }
    }
  }
  return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for ( auto it = arp_cache.begin(); it != arp_cache.end(); ) {
    auto key = it->second;
    key.second += ms_since_last_tick;
    if ( key.second >= 30000 ) {
      it = arp_cache.erase( it );
    } else {
      it++;
    }
  }
  
  for(auto it = timer_4_arp_request_.begin(); it != timer_4_arp_request_.end(); ) {
    auto key = it->second;
    key += ms_since_last_tick;
    it++;
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( frame_queue.empty() ) {
    return nullopt;
  }

  auto ret = frame_queue.front();
  frame_queue.pop();
  return ret;
}
