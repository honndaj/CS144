#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  if(output.available_capacity() + next_byte_ <= first_index) {
    return ;
  }

  if(first_index < next_byte_) {
    if(first_index + data.size() <= next_byte_) {
      return ;
    }else {
      data = data.substr(next_byte_ - first_index);
      first_index = next_byte_;
    }
  }
  // 现在first_index大于等于next_byte
  data = data.substr(0, output.available_capacity());
  //改变buf_大小并插入data
  uint64_t expect_len = max(buf_.size(), first_index + data.size() - next_byte_);
  buf_.resize(expect_len, std::make_pair(' ', false));
  for(uint64_t i = 0; i < data.size(); i++ ) {
    if(buf_[first_index - next_byte_ + i].second == 0) temp_bytes_++;
    buf_[first_index - next_byte_ + i] = std::make_pair(data[i], true);
  }
  // 缓存交付
  if(!buf_.empty() && buf_[0].second) {
    std::string t = {};
    while(!buf_.empty() && buf_[0].second) {
      t += buf_[0].first;
      buf_.pop_front();
      next_byte_++;
      temp_bytes_--;
    }
    output.push(t);
  }
  if(is_last_substring) {
    is_receive_last_ = true;
  }
  if(is_receive_last_ && buf_.empty()) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return {temp_bytes_};
}