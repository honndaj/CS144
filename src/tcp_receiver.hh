#pragma once

#include "reassembler.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"

class TCPReceiver
{
protected:
  Wrap32 zero_point_ { 0 };
  uint64_t ackno_ { 0 };
  uint64_t last_byte_ { 0 };      // 通过带FIN标志的segment计算出最后一个字节，来判断是否结束
  bool is_connecting_ { false };  // 是否正处于连接中
  bool is_connected_ { false };   // 是否已经连接过
  bool is_receive_FIN_ { false }; // 是否接收到过FIN

public:
  /*
   * The TCPReceiver receives TCPSenderMessages, inserting their payload into the Reassembler
   * at the correct stream index.
   */
  void receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream );

  /* The TCPReceiver sends TCPReceiverMessages back to the TCPSender. */
  TCPReceiverMessage send( const Writer& inbound_stream ) const;
};
