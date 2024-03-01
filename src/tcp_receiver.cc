#include "tcp_receiver.hh"
#include <cassert>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if ( message.SYN ) {
    zero_point_ = message.seqno;
    is_connecting_ = true;
    is_connected_ = true;
  }
  if ( is_connecting_ ) {
    uint64_t ab_first_index
      = message.seqno.unwrap( zero_point_, ackno_ ) + message.SYN; // 计算出绝对index（为啥SYN也可以带数据啊？
    if ( !message.payload.empty()
         && ab_first_index != 0 ) { // ab_first_index!=0 防一手异常数据（seqno=0的但SYN不等于1的数据segment）
      reassembler.insert(
        ab_first_index - 1, message.payload, false, inbound_stream ); // 字节流的关闭不通过reassemler来实现
    }
    ackno_ = inbound_stream.bytes_pushed() + 1; // plus 1 due to the SYN
    if ( message.FIN ) {
      last_byte_ = ab_first_index - 1 + message.payload.size();
      is_receive_FIN_ = true;
    }
  }
  if ( last_byte_ == inbound_stream.bytes_pushed() && is_receive_FIN_ ) { // 如果读完全部字节，并且收到了FIN
    is_connecting_ = false;
    ackno_++;
    inbound_stream.close();
  }
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  std::optional<Wrap32> ackno = std::nullopt;
  uint16_t rwnd = UINT16_MAX;
  if ( is_connected_ ) { //
    ackno.emplace( Wrap32::wrap( ackno_, zero_point_ ) );
  }
  rwnd = uint16_t(
    min( uint64_t( rwnd ),
         inbound_stream.available_capacity() ) ); // note the difference of the type(window_size , capacity)
  return TCPReceiverMessage( ackno, rwnd );
}
