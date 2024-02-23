#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  auto len = data.size();
  if(is_last_substring) {
    is_receive_last_ = true;
  }
  if(output.is_closed()) {
    return ;
  }
  if(first_index + len <= next_byte_){
    check_close(output);
  }
  if(first_index == next_byte_) {// data刚好是要的
    insert_one_substring(data, output);
  }
  else if(first_index < next_byte_) {// data冗余了前面一部分
    if(first_index + len > next_byte_){
      std::string t = data.substr(next_byte_ - first_index);//注意不能合并成一行，不能引用非左值（临时变量）
      insert_one_substring(t, output);
    }
  }
  else {// 加入缓存
    if(next_byte_ + output.available_capacity() <= first_index) {// 超过当前窗口就不要了
      return ;
    } 
    if(buf_.empty()) {
      buf_[first_index] = data;
      temp_bytes_ += data.size();
      return ;
    }
    //若buf非空
    auto it = buf_.upper_bound(first_index);
    uint64_t it_index;
    std::string it_data;
    if(it != buf_.begin()) {// 有前一块
      it--;
      //处理first_index的前一块
      it_index = static_cast<uint64_t> (it->first);
      it_data = it->second;

      if(it_index + it_data.size() >= first_index) {
        if(first_index + len < it_index + it_data.size()) {// 如果被覆盖了
          return ;
        }
        data = it_data.substr(0, first_index - it_index) + data;
        first_index = it_index;
        temp_bytes_ -= it_data.size();
        it = buf_.erase(it);
      }
      else {
        it++;
      }
    }

    it_index = static_cast<uint64_t> (it->first);
    it_data = it->second;
    for(; it != buf_.end() && it_index < first_index + len;) {// 若有重叠就继续循环， 直到变成一块
      it_index = static_cast<uint64_t> (it->first);
      it_data = it->second;

      if(it_index + it_data.size() <= first_index + len) { //被覆盖
        temp_bytes_ -= it_data.size();
        it = buf_.erase(it);
        continue;
      }
      //合并块
      uint64_t overlap_len = first_index + len - it_index;
      data += it_data.substr(overlap_len);
      temp_bytes_ -= it_data.size();
      buf_.erase(it);
    }
    buf_[first_index] = data;
    temp_bytes_ += data.size();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return {temp_bytes_};
}