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
  uint64_t last_byte_ { 0 };      // ͨ����FIN��־��segment��������һ���ֽڣ����ж��Ƿ����
  bool is_connecting_ { false };  // �Ƿ�������������
  bool is_connected_ { false };   // �Ƿ��Ѿ����ӹ�
  bool is_receive_FIN_ { false }; // �Ƿ���յ���FIN

public:
  /*
   * The TCPReceiver receives TCPSenderMessages, inserting their payload into the Reassembler
   * at the correct stream index.
   */
  void receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream );

  /* The TCPReceiver sends TCPReceiverMessages back to the TCPSender. */
  TCPReceiverMessage send( const Writer& inbound_stream ) const;
};
