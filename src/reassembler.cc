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
  if(first_index == next_byte_) {// data�պ���Ҫ��
    insert_one_substring(data, output);
  }
  else if(first_index < next_byte_) {// data������ǰ��һ����
    if(first_index + len > next_byte_){
      std::string t = data.substr(next_byte_ - first_index);//ע�ⲻ�ܺϲ���һ�У��������÷���ֵ����ʱ������
      insert_one_substring(t, output);
    }
  }
  else {
    if(next_byte_ + output.available_capacity() <= first_index) {
      return ;
    } 
    auto it = buf_.find(first_index);
    if(it != buf_.end()) {// exist��ѡ����
      if(it->second.size() < len) {
        buf_[first_index] = data;
        temp_bytes_ += len - buf_[first_index].size();
      }
    }
    else{// not exist��ֱ�ӷ�
      buf_[first_index] = data;
      temp_bytes_ += len;
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return {temp_bytes_};
}