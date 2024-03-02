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
      = message.seqno.unwrap( zero_point_, ackno_ ) + message.SYN; // ���������index��ΪɶSYNҲ���Դ����ݰ���
    if ( !message.payload.empty()
         && ab_first_index != 0 ) { // ab_first_index!=0 ��һ���쳣���ݣ�seqno=0�ĵ�SYN������1������segment��
      reassembler.insert(
        ab_first_index - 1, message.payload, false, inbound_stream ); // �ֽ����Ĺرղ�ͨ��reassemler��ʵ��
    }
    ackno_ = inbound_stream.bytes_pushed() + 1; // plus 1 due to the SYN
    if ( message.FIN ) {
      last_byte_ = ab_first_index - 1 + message.payload.size();
      is_receive_FIN_ = true;
    }
  }
  if ( last_byte_ == inbound_stream.bytes_pushed() && is_receive_FIN_ ) { // �������ȫ���ֽڣ������յ���FIN
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
