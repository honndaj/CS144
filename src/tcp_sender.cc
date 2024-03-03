#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>
#include <cassert>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms ), now_RTO_ms_( initial_RTO_ms ), remain_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return seq_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return num_consecutive_retransmissions_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  TCPSenderMessage ret;
  if(is_retransmission_) {
    is_retransmission_ = false;
    ret = track_segment_.begin()->second;
  }
  else {
    if(track_segment_.find(next_seqno_need_to_send_) == track_segment_.end()) {
      return nullopt;
    }
    ret = track_segment_[next_seqno_need_to_send_];
    next_seqno_need_to_send_+= ret.sequence_length();
  }
  return ret;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  if( already_send_FIN_ ) {
    return ;
  }
  if(outbound_stream.is_finished()) { // 这个分支怎么给他合并掉 TODO
    if(rwnd_ - seq_in_flight_ == 0) {
      return ;
    }
    track_segment_[next_seqno_need_to_wrap_] = TCPSenderMessage(Wrap32::wrap(next_seqno_need_to_wrap_, isn_), !already_send_SYN_, {}, true);
    seq_in_flight_ += 1 + !already_send_SYN_;
    next_seqno_need_to_wrap_ += 1 + !already_send_SYN_;
    if(already_send_SYN_ == 0) {
      already_send_SYN_ = 1;
    }
    already_send_FIN_ = true;
    return ;
  }

  size_t len = min(static_cast<size_t>(outbound_stream.bytes_buffered()), TCPConfig::MAX_PAYLOAD_SIZE), rwnd = rwnd_ == 0 ? 1 : rwnd_;
  if(!already_send_SYN_) {
    if(1 + len > rwnd - seq_in_flight_) {
      len = rwnd - seq_in_flight_ - 1;
    }
  }
  else {
    len = min(len, static_cast<size_t>(rwnd - seq_in_flight_));
  }

  string payload = {};
  while(outbound_stream.bytes_buffered() && payload.size() < len) {
    auto view = outbound_stream.peek();
    view = view.substr(0, min(len - payload.size(), view.size()));
    payload += view;
    outbound_stream.pop(view.size());
  }

  if(!already_send_SYN_ || !payload.empty() ) {
    bool is_FIN = false;
    if(outbound_stream.is_finished()) {
      if(len + !already_send_SYN_ < rwnd_) {
        is_FIN = true;
      }
    }
    auto msg = TCPSenderMessage(Wrap32::wrap(next_seqno_need_to_wrap_, isn_), !already_send_SYN_, std::move(payload), is_FIN);
    uint64_t msg_len = msg.sequence_length();
    track_segment_[next_seqno_need_to_wrap_] = std::move(msg);
    already_send_SYN_ = true;
    seq_in_flight_ += msg_len; //注意这个时候就加了num_in_flight而不是真正发送后才加
    next_seqno_need_to_wrap_ += msg_len;
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  return TCPSenderMessage(Wrap32::wrap(next_seqno_need_to_send_, isn_), false, {}, false);
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  uint64_t ab_ackno = 0;
  rwnd_ = msg.window_size;
  if(msg.ackno.has_value()) {
    ab_ackno = msg.ackno.value().unwrap(isn_, next_seqno_need_to_send_);
  }
  else {
    return ;
  }
  if(ab_ackno > next_seqno_need_to_send_) {
    return ;
  }
  if(ab_ackno > ackno_) {
    ackno_ = ab_ackno;
    now_RTO_ms_ = initial_RTO_ms_;
    remain_ms_ = initial_RTO_ms_;
    num_consecutive_retransmissions_ = 0;
  }
  for(auto it = track_segment_.begin(); it != track_segment_.end(); ) {
    if(it->first + it->second.sequence_length() <= ab_ackno) {
      seq_in_flight_ -= it->second.sequence_length();
      it = track_segment_.erase(it);
    }else {
      it++;
    }
  }
}

void TCPSender::tick( const size_t ms_since_last_tick ) // 很搞啊，重传的maybe_sent不能放这里，是由测试样例调用的
{
  // Your code here.
  if( ms_since_last_tick >= remain_ms_ ) {
    is_retransmission_ = true;
    if(rwnd_) {// 窗口非零别忘了，我没注意到，pdf里直接告诉了
      num_consecutive_retransmissions_ ++;
      now_RTO_ms_ *= 2;
    }
    remain_ms_ = now_RTO_ms_;
  }else {
    remain_ms_ -= ms_since_last_tick;
  }
}
