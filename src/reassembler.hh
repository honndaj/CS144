#pragma once

#include "byte_stream.hh"

#include <string>
#include <map>
#include <iostream>

class Reassembler
{
private:
  void insert_one_substring(std::string& data, Writer& output) // 暂存的子流只会在插入子流时发挥作用
  {
    push_to_ByteStream(output, data);
    uint64_t first_buf_byte;
    // 在从buf里取已收到的子流时，多余的部分是否丢弃？ 现在是按照丢弃来处理
    while(!buf_.empty() && (first_buf_byte = get_first_buf_byte()) <= next_byte_) {//如果缓存不空且第一个缓存的index小于等于需要的下一个字节
      if(first_buf_byte + buf_.begin()->second.size() > next_byte_) {//如果缓存有用
        std::string t = buf_.begin()->second.substr(next_byte_ - first_buf_byte);// 那就把有用部分塞进去
        push_to_ByteStream(output, t);
      } 
      temp_bytes_ -= buf_.begin()->second.size();// 从缓存中移除子流后，需要把缓存的字节数给剪掉
      buf_.erase(buf_.begin());
    }
    check_close(output);
    //std::cout<< is_receive_last_ << " data is inserting: " << data << std::endl;
  }

  void push_to_ByteStream(Writer& output, std::string& data) {
    uint64_t actual_len = std::min(output.available_capacity(), data.size());
    output.push(data.substr(0, actual_len));
    next_byte_ += actual_len;
  }

  uint64_t get_first_buf_byte() {
    uint64_t first_buf_byte = static_cast<uint64_t>(buf_.begin()->first);
    return first_buf_byte;
  }

  void check_close(Writer& output) {
    if(is_receive_last_ && buf_.empty()) {
      output.close();
    }
  }


protected:
  std::map<int, std::string> buf_{};
  uint64_t temp_bytes_ {0};
  uint64_t next_byte_ {0};
  bool is_receive_last_ {false};

public:
  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring, Writer& output );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;
};
