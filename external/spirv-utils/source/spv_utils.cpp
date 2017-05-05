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

#include <spv_utils.h>
#include <cassert>
#include <sstream>

namespace sut {

static const size_t kSpvIndexMagicNumber = 0;
static const size_t kSpvIndexVersionNumber = 1;
static const size_t kSpvIndexGeneratorNumber = 2;
static const size_t kSpvIndexBound = 3;
static const size_t kSpvIndexSchema = 4;
static const size_t kSpvIndexInstruction = 5;
static const size_t kPendingOpsInitialReserve = 5;
static const uint32_t kMarker = 0xFFFFFFFF;

OpcodeHeader SplitSpvOpCode(uint32_t word) {
  return {static_cast<uint16_t>((0xFFFF0000 & word) >> 16U),
          static_cast<uint16_t>(0x0000FFFF & word)};
}

uint32_t MergeSpvOpCode(const OpcodeHeader &header) {
  return ((static_cast<uint32_t>(header.words_count) << 16U) |
          (static_cast<uint32_t>(header.opcode)));
}

InvalidParameter::InvalidParameter(const std::string &what_arg)
    : std::runtime_error(what_arg) {}

InvalidStream::InvalidStream(const std::string &what_arg)
    : std::runtime_error(what_arg) {}

InvalidOperation::InvalidOperation(const std::string &what_arg)
    : std::logic_error(what_arg) {}

OpcodeStream::OpcodeStream(const void *module_stream, size_t binary_size)
    : module_stream_(), offsets_table_(), original_module_size_(0) {
  if (!module_stream || !binary_size || ((binary_size % 4) != 0) ||
      ((binary_size / 4) < kSpvIndexInstruction)) {
    throw InvalidParameter("Invalid parameter in ctor of OpcodeStream!");
  }

  // The +1 is because we will append a null-terminator to the stream
  module_stream_.reserve((binary_size / 4));
  module_stream_.insert(
      module_stream_.begin(), static_cast<const uint32_t *>(module_stream),
      static_cast<const uint32_t *>(module_stream) + (binary_size / 4));
  original_module_size_ = module_stream_.size();

  // Reserve enough memory for the worst case scenario, where each instruction
  // is one word long
  // The +1 is because we will append a null-terminator to the stream
  offsets_table_.reserve(module_stream_.size() + 1);

  ParseModule();
}

OpcodeStream::OpcodeStream(const std::vector<uint32_t> &module_stream)
    : module_stream_(), offsets_table_(), original_module_size_(0) {
  if (module_stream.size() < kSpvIndexInstruction) {
    throw InvalidParameter(
        "Invalid number of words in the module passed to ctor of "
        "OpcodeStream!");
  }

  module_stream_.reserve(module_stream.size());
  module_stream_.insert(module_stream_.begin(), module_stream.begin(),
                        module_stream.end());
  original_module_size_ = module_stream_.size();

  // Reserve enough memory for the worst case scenario, where each instruction
  // is one word long
  // The +1 is because we will append a null-terminator to the stream
  offsets_table_.reserve(module_stream_.size() + 1);

  ParseModule();
}

OpcodeStream::OpcodeStream(std::vector<uint32_t> &&module_stream)
    : module_stream_(std::move(module_stream)),
      offsets_table_(),
      original_module_size_(0) {
  if (module_stream_.size() < kSpvIndexInstruction) {
    throw InvalidParameter(
        "Invalid number of words in the module passed to ctor of "
        "OpcodeStream!");
  }

  original_module_size_ = module_stream_.size();

  // Reserve enough memory for the worst case scenario, where each instruction
  // is one word long
  // The +1 is because we will append a null-terminator to the stream
  offsets_table_.reserve(module_stream_.size() + 1);

  ParseModule();
}
void OpcodeStream::ParseModule() {
  const size_t words_count = module_stream_.size();

  // Set tokens for theader; these always take the same amount of words
  InsertOffsetInTable(kSpvIndexMagicNumber);
  InsertOffsetInTable(kSpvIndexVersionNumber);
  InsertOffsetInTable(kSpvIndexGeneratorNumber);
  InsertOffsetInTable(kSpvIndexBound);
  InsertOffsetInTable(kSpvIndexSchema);

  size_t word_index = kSpvIndexInstruction;

  // Once the loop is finished, the offsets table will contain one entry
  // per instruction
  while (word_index < words_count) {
    size_t inst_word_count = ParseInstructionWordCount(word_index);

    // Insert offset for the current instruction
    InsertOffsetInTable(word_index);

    // Advance the word index by the size of the instruction
    word_index += inst_word_count;
  }

  // Append end terminator to table
  InsertOffsetInTable(words_count);

  // Append end terminator to original words stream
  InsertWordHeaderInOriginalStream({0U, static_cast<uint16_t>(spv::Op::OpNop)});
}

void OpcodeStream::InsertWordHeaderInOriginalStream(
    const OpcodeHeader &header) {
  module_stream_.push_back(MergeSpvOpCode(header));
}

void OpcodeStream::InsertOffsetInTable(size_t offset) {
  offsets_table_.push_back(OpcodeIterator(offset, module_stream_));
}

size_t OpcodeStream::ParseInstructionWordCount(size_t start_index) {
  // Read the first word of the instruction, which
  // contains the word count
  uint32_t first_word = PeekAt(start_index);

  // Decompose and read the word count
  OpcodeHeader header = SplitSpvOpCode(first_word);

  if (header.words_count < 1U) {
    std::stringstream msg_stream;
    msg_stream << "Word with index " << start_index << " has word count of "
               << header.words_count;
    std::string msg = msg_stream.str();
    throw InvalidStream(msg);
  }

  return static_cast<size_t>(header.words_count);
}

uint32_t OpcodeStream::PeekAt(size_t index) const {
  return module_stream_[index];
}

OpcodeStream OpcodeStream::EmitFilteredStream() const {
  WordsStream new_stream;
  // The new stream will roughly be as large as the original one
  new_stream.reserve(module_stream_.size());

  for (OffsetsList::const_iterator oi = offsets_table_.begin();
       oi != (offsets_table_.end() - 1); oi++) {
    if (oi->insert_before_count() > 0) {
      EmitByType(new_stream, oi->insert_before_offset(),
                 oi->insert_before_count());
    }

    if (!oi->is_removed()) {
      new_stream.insert(new_stream.end(), module_stream_.begin() + oi->offset(),
                        module_stream_.begin() + (oi + 1)->offset());
    } else if (oi->replace_count() > 0) {
      EmitByType(new_stream, oi->replace_offset(), oi->replace_count());
    }

    if (oi->insert_after_count() > 0) {
      EmitByType(new_stream, oi->insert_after_offset(),
                 oi->insert_after_count());
    }
  }

  return OpcodeStream(std::move(new_stream));
}

std::vector<uint32_t> OpcodeStream::GetWordsStream() const {
  return std::vector<uint32_t>(module_stream_.begin(),
                               module_stream_.begin() + original_module_size_);
}

void OpcodeStream::EmitByType(WordsStream &new_stream, size_t start_offset,
                              size_t count) const {
  new_stream.insert(new_stream.end(), module_stream_.begin() + start_offset,
                    module_stream_.begin() + start_offset + count);

  // If there isn't a marker after the last word, it means that there
  // are other parts to output which are appended in the stream of words
  if (*(module_stream_.begin() + start_offset + count) != kMarker) {
    // Go through the makers until you get to the last one
    const uint32_t *current_marker = &module_stream_[start_offset + count];

    while (*current_marker != kMarker) {
      uint32_t next_index = ((0xFFFF0000 & *current_marker) >> 16U);
      uint32_t next_count = (0x0000FFFF & *current_marker);

      // Output
      new_stream.insert(new_stream.end(), module_stream_.begin() + next_index,
                        module_stream_.begin() + next_index + next_count);

      current_marker = &module_stream_[next_index + next_count];
    }
  }
}

OpcodeStream::iterator OpcodeStream::begin() { return offsets_table_.begin(); }

OpcodeStream::iterator OpcodeStream::end() { return offsets_table_.end(); }

OpcodeStream::const_iterator OpcodeStream::end() const {
  return offsets_table_.end();
}

OpcodeStream::const_iterator OpcodeStream::begin() const {
  return offsets_table_.begin();
}

OpcodeStream::const_iterator OpcodeStream::cbegin() const {
  return offsets_table_.cbegin();
}

OpcodeStream::const_iterator OpcodeStream::cend() const {
  return offsets_table_.cend();
}

size_t OpcodeStream::size() const { return offsets_table_.size(); }

OpcodeIterator::OpcodeIterator(size_t offset, std::vector<uint32_t> &words)
    : offset_(offset),
      insert_before_offset_(0),
      insert_before_count_(0),
      insert_after_offset_(0),
      insert_after_count_(0),
      replace_offset_(0),
      replace_count_(0),
      remove_(false),
      words_(words) {}

spv::Op OpcodeIterator::GetOpcode() const {
  uint32_t header_word = words_[offset_];

  return static_cast<spv::Op>(SplitSpvOpCode(header_word).opcode);
}

void OpcodeIterator::InsertBefore(const uint32_t *instructions,
                                  size_t words_count) {
  assert(instructions && words_count);

  // Offset before new words are inserted
  size_t previous_offset = words_.size();

  // +1 is for the end marker
  words_.reserve(previous_offset + words_count + 1);
  words_.insert(words_.end(), instructions, instructions + words_count);

  // Add the end marker
  words_.push_back(kMarker);

  // After the first word of type before has been inserted, always append
  // in the list and write the marker
  if (insert_before_count_ != 0) {
    uint32_t *latest_marker = &words_[words_.size() - 1U];

    // Write the marker
    *latest_marker = ((0xFFFF0000 & (insert_before_offset_ << 16U)) |
                      (0x0000FFFF & insert_before_count_));
  }

  insert_before_offset_ = previous_offset;
  insert_before_count_ = words_count;
}

void OpcodeIterator::InsertAfter(const uint32_t *instructions,
                                 size_t words_count) {
  assert(instructions && words_count);

  // Offset before new words are inserted
  size_t previous_offset = words_.size();

  // +1 is for the end marker
  words_.reserve(previous_offset + words_count + 1);
  words_.insert(words_.end(), instructions, instructions + words_count);

  // Add the end marker
  words_.push_back(kMarker);

  // After the first word of type after has been inserted, always append
  // in the list and write the marker
  if (insert_after_count_ != 0) {
    uint32_t *latest_marker = &words_[words_.size() - 1U];

    // Write the marker
    *latest_marker = ((0xFFFF0000 & (insert_after_offset_ << 16U)) |
                      (0x0000FFFF & insert_after_count_));
  }

  insert_after_offset_ = previous_offset;
  insert_after_count_ = words_count;
}

uint32_t *OpcodeIterator::GetLatestMaker(size_t initial_offset,
                                         size_t initial_count) const {
  uint32_t *current_marker = &words_[initial_offset + initial_count];

  while (*current_marker != kMarker) {
    uint32_t next_index = ((0xFFFF0000 & *current_marker) >> 16U);
    uint32_t next_count = (0x0000FFFF & *current_marker);
    current_marker = &words_[next_index + next_count];
  }

  return current_marker;
}

void OpcodeIterator::Remove() {
  if (remove_) {
    throw InvalidOperation("Called Remove() more than once!");
  }

  remove_ = true;
}

void OpcodeIterator::Replace(const uint32_t *instructions, size_t words_count) {
  assert(instructions && words_count);

  if (replace_count() > 0) {
    throw InvalidOperation("Called Replace() more than once!");
  }

  // Since we are replacing, remove the old instruction
  Remove();

  // Offset before new words are inserted
  size_t previous_offset = words_.size();

  // +1 is for the end marker
  words_.reserve(previous_offset + words_count + 1);
  words_.insert(words_.end(), instructions, instructions + words_count);

  // Add the end marker
  words_.push_back(kMarker);

  replace_offset_ = previous_offset;
  replace_count_ = words_count;
}

uint32_t OpcodeIterator::GetFirstWord() const { return words_[offset_]; }

}  // namespace sut
