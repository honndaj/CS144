#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <map>

class TCPSender
{
  Wrap32 isn_;
  uint64_t next_seqno_need_to_send_ { 0 }; // 下一个要发送的seqno（封装完未发送）。包括SYN和FIN
  uint64_t next_seqno_need_to_wrap_ { 0 }; //下一个要封装到message里的seqno, 比next_seqno_need_to_send_大
  uint64_t ackno_ { 0 };                   //收到的最大ackno
  uint64_t initial_RTO_ms_;
  uint64_t now_RTO_ms_;
  uint64_t remain_ms_;
  bool is_sending_one_seq_ { false }; // 是否正在发送一字节的测试msg，防止连续发送不同的测试msg
  uint64_t one_seqno_ { 0 }; // 和is_sending_one_sqe_配套使用，记录一个字节的序列号，用来确定是否收到测试字节（因为也有可能收到之前发送的数据包）
  uint64_t num_consecutive_retransmissions_ { 0 };
  uint16_t rwnd_ { 1 };          // SYN和FIN占用窗口 注意这里一开始是1
  uint64_t seq_in_flight_ { 0 }; // 发出去的序号数量
  bool already_send_SYN_ { false };
  bool already_send_FIN_ { false };
  bool is_retransmission_ { false };

  std::map<uint64_t, TCPSenderMessage> track_segment_ {};

  void make_msg( std::string& payload, bool is_SYN, bool is_FIN );
  void read_payload( size_t payload_len, Reader& outbound_stream, std::string& payload );

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
