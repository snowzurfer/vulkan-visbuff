/*
  MIT License

  Copyright (c) 2017 Alberto Taiuti

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef SPV_UTILS_H_DSEVTT7Q
#define SPV_UTILS_H_DSEVTT7Q

#include <cstdint>
#include <functional>
#include <memory>
#include <spirv/1.1/spirv.hpp11>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace sut {

struct OpcodeHeader final {
  uint16_t words_count;
  uint16_t opcode;
};  // struct OpCodeHeader

// Split a SPIR-V header word into the words count and the opcode and return
// this as an OpcodeHeader object
OpcodeHeader SplitSpvOpCode(uint32_t word);
// Merge the word count and opcode of an instruction, in form of an OpcodeHeader
// object, into a word
uint32_t MergeSpvOpCode(const OpcodeHeader &header);

class InvalidParameter final : public std::runtime_error {
 public:
  explicit InvalidParameter(const std::string &what_arg);
};  // class InvalidParameter

class InvalidStream final : public std::runtime_error {
 public:
  explicit InvalidStream(const std::string &what_arg);
};  // class InvalidStream

class InvalidOperation final : public std::logic_error {
 public:
  explicit InvalidOperation(const std::string &what_arg);
};  // class InvalidOperation

class OpcodeIterator final {
 public:
  // Ctor
  // Construct an iterator given the offset of the instruction which this
  // iterator refers to and the stream of words this instruction is contained in
  explicit OpcodeIterator(size_t offset, std::vector<uint32_t> &words);

  // Get the opcode from the first word of the instruction
  spv::Op GetOpcode() const;

  // Get the offset in words for this instruction from the beginning of the
  // stream
  size_t offset() const { return offset_; }

  // Get the first word of this instruction
  uint32_t GetFirstWord() const;

  // Insert instructions stream in LIFO order
  void InsertBefore(const uint32_t *instructions, size_t words_count);
  // Insert instructions stream in LIFO order
  void InsertAfter(const uint32_t *instructions, size_t words_count);
  void Remove();
  void Replace(const uint32_t *instructions, size_t words_count);

 private:
  bool is_removed() const { return remove_; }
  size_t insert_before_offset() const { return insert_before_offset_; }
  size_t insert_before_count() const { return insert_before_count_; }
  size_t insert_after_offset() const { return insert_after_offset_; }
  size_t insert_after_count() const { return insert_after_count_; }
  size_t replace_offset() const { return replace_offset_; }
  size_t replace_count() const { return replace_count_; }

  // Make the class a friend so that it can access the accessor methods
  friend class OpcodeStream;

  // Data relative to this instruction
  size_t offset_;

  // List of words to be used to otput the filtered stream
  size_t insert_before_offset_;
  size_t insert_before_count_;
  size_t insert_after_offset_;
  size_t insert_after_count_;
  size_t replace_offset_;
  size_t replace_count_;
  bool remove_;

  uint32_t *GetLatestMaker(size_t initial_offset, size_t initial_count) const;

  std::vector<uint32_t> &words_;

};  // class Opcode

class OpcodeStream final {
 public:
  typedef std::vector<OpcodeIterator>::iterator iterator;
  typedef std::vector<OpcodeIterator>::const_iterator const_iterator;

 public:
  explicit OpcodeStream(const void *module_stream, size_t binary_size);
  explicit OpcodeStream(const std::vector<uint32_t> &module_stream);
  explicit OpcodeStream(std::vector<uint32_t> &&module_stream);

  // Standard iterators which can be used to access the instructions
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator cbegin() const;
  const_iterator end() const;
  const_iterator cend() const;

  // Number of instructions in the stream
  size_t size() const;

  // Apply pending operations and emit filtered stream into a new object
  //
  // This OpcodeStream does not get modified but it still retains the
  // operations applied to it, so calling EmitFilteredStream() a second time
  // will produce the same filtered stream
  OpcodeStream EmitFilteredStream() const;

  // Get the raw words stream, unfiltered and non-modified
  std::vector<uint32_t> GetWordsStream() const;

 private:
  typedef std::vector<uint32_t> WordsStream;
  typedef std::vector<OpcodeIterator> OffsetsList;

  // Stream of words representing the module as it has been modified
  WordsStream module_stream_;

  size_t original_module_size_;

  // One entry per instruction, with entries coming only from the original
  // module, i.e. without the filtering
  OffsetsList offsets_table_;

  void InsertOffsetInTable(size_t offset);
  void InsertWordHeaderInOriginalStream(const struct OpcodeHeader &header);

  // Parse the module stream into an offset table; called by the ctor
  void ParseModule();

  // Return the word count of a given instruction starting at start_index
  size_t ParseInstructionWordCount(size_t start_index);

  // Return the word at a given index in the module stream
  uint32_t PeekAt(size_t index) const;

  // Emit the words for a given type of operation; the different type
  // depends only on where the words are read from, so passing the start offset
  // and count is sufficient to distinguish
  void EmitByType(WordsStream &new_stream, size_t start_offset,
                  size_t count) const;

};  // class OpcodeStream

}  // namespace sut

#endif
