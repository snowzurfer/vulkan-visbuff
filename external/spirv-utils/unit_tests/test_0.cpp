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
#include <array>
#include <catch.hpp>
#include <cstdint>
#include <fstream>
#include <iostream>

#define STR_EXPAND(str) #str
#define STR(str) STR_EXPAND(str)

TEST_CASE("spv utils is tested with correct spir-v binary",
          "[spv-utils-correct-spvbin]") {
  // Read spv binary module from a file
  std::ifstream spv_file(STR(SPV_ASSETS_FOLDER) "/test.frag.spv",
                         std::ios::binary | std::ios::ate | std::ios::in);
  REQUIRE(spv_file.is_open() == true);
  std::streampos size = spv_file.tellg();
  CHECK(size > 0);

  spv_file.seekg(0, std::ios::beg);
  char *data = new char[size];
  spv_file.read(data, size);
  spv_file.close();

  SECTION("Passing a null ptr for the data throws an exception") {
    REQUIRE_THROWS_AS(sut::OpcodeStream(nullptr, size), sut::InvalidParameter);
  }
  SECTION("Passing size zero for the size throws an exception") {
    REQUIRE_THROWS_AS(sut::OpcodeStream(data, 0), sut::InvalidParameter);
  }
  SECTION("Passing size zero for the size and nullptr throws an exception") {
    REQUIRE_THROWS_AS(sut::OpcodeStream(nullptr, 0), sut::InvalidParameter);
  }
  SECTION("Passing correct data ptr and size creates the object") {
    REQUIRE_NOTHROW(sut::OpcodeStream(data, size));

    sut::OpcodeHeader header_0 = {1U, static_cast<uint16_t>(spv::Op::OpNop)};
    sut::OpcodeHeader header_1 = {4U, static_cast<uint16_t>(spv::Op::OpNop)};

    uint32_t instruction_0 = MergeSpvOpCode(header_0);
    uint32_t instruction_1 = MergeSpvOpCode(header_1);

    std::array<uint32_t, 4U> longer_instruction = {instruction_1, 0xDEADBEEF,
                                                   0xDEADBEEF, 0xDEADBEEF};
    std::array<uint32_t, 4U> longer_instruction_2 = {instruction_1, 0x1EADBEEF,
                                                     0x1EADBEEF, 0x1EADBEEF};

    SECTION(
        "Inserting before, after and removing produces an output of the right "
        "size") {
      sut::OpcodeStream stream(data, size);
      for (auto &i : stream) {
        if (i.GetOpcode() == spv::Op::OpCapability) {
          i.InsertBefore(longer_instruction.data(), longer_instruction.size());
          i.InsertAfter(&instruction_0, 1U);
          i.InsertAfter(longer_instruction.data(), longer_instruction.size());
          i.InsertAfter(longer_instruction.data(), longer_instruction.size());
          i.InsertBefore(longer_instruction_2.data(),
                         longer_instruction_2.size());
          i.Remove();
        }
      }

      sut::OpcodeStream new_stream = stream.EmitFilteredStream();
      std::vector<uint32_t> new_module = new_stream.GetWordsStream();

      // -1 is due to removing the instruction OpCapability which is 2 words
      // long
      // and adding one instruction one word long.
      REQUIRE(new_module.size() ==
              ((size / 4) + (longer_instruction.size() * 3) +
               longer_instruction_2.size() - 1));
    }

    SECTION("Replacing produces an output of the right size") {
      sut::OpcodeStream stream(data, size);
      for (auto &i : stream) {
        if (i.GetOpcode() == spv::Op::OpCapability) {
          i.Replace(longer_instruction.data(), longer_instruction.size());
        }
      }

      sut::OpcodeStream new_stream = stream.EmitFilteredStream();
      std::vector<uint32_t> new_module = new_stream.GetWordsStream();

      // -2 is due to removing the instruction OpCapability which is 2 words
      // long
      REQUIRE(new_module.size() ==
              ((size / 4) + longer_instruction.size() - 2));
    }

    SECTION("Inserting does not alter the size of the original raw module") {
      sut::OpcodeStream stream(data, size);
      for (auto &i : stream) {
        if (i.GetOpcode() == spv::Op::OpCapability) {
          i.InsertAfter(longer_instruction.data(), longer_instruction.size());
        }
      }

      std::vector<uint32_t> old_module = stream.GetWordsStream();

      // -2 is due to removing the instruction OpCapability which is 2 words
      // long
      REQUIRE(old_module.size() == static_cast<size_t>(size / 4));
    }

    SECTION("Using the new stream does not produce errors") {
      sut::OpcodeStream stream(data, size);
      for (auto &i : stream) {
        if (i.GetOpcode() == spv::Op::OpCapability) {
          i.InsertAfter(longer_instruction.data(), longer_instruction.size());
        }
      }

      sut::OpcodeStream new_stream = stream.EmitFilteredStream();
      std::vector<uint32_t> new_module = new_stream.GetWordsStream();

      REQUIRE(new_module.size() == ((size / 4) + longer_instruction.size()));

      for (auto &i : new_stream) {
        if (i.GetOpcode() == spv::Op::OpCapability) {
          i.InsertAfter(longer_instruction.data(), longer_instruction.size());
        }
      }

      sut::OpcodeStream new_stream_2 = new_stream.EmitFilteredStream();
      std::vector<uint32_t> new_module_2 = new_stream_2.GetWordsStream();

      REQUIRE(new_module_2.size() ==
              ((size / 4) + (longer_instruction.size() * 2)));
    }

    SECTION("Removing more than once throws") {
      sut::OpcodeStream stream(data, size);
      for (auto &i : stream) {
        if (i.GetOpcode() == spv::Op::OpCapability) {
          i.Remove();
          REQUIRE_THROWS_AS(i.Remove(), sut::InvalidOperation);
        }
      }
    }

    SECTION("Replacing more than once throws") {
      sut::OpcodeStream stream(data, size);
      for (auto &i : stream) {
        if (i.GetOpcode() == spv::Op::OpCapability) {
          i.Replace(longer_instruction.data(), longer_instruction.size());
          REQUIRE_THROWS_AS(
              i.Replace(longer_instruction.data(), longer_instruction.size()),
              sut::InvalidOperation);
        }
      }
    }

    SECTION("Replacing after removing throws") {
      sut::OpcodeStream stream(data, size);
      for (auto &i : stream) {
        if (i.GetOpcode() == spv::Op::OpCapability) {
          i.Remove();
          REQUIRE_THROWS_AS(
              i.Replace(longer_instruction.data(), longer_instruction.size()),
              sut::InvalidOperation);
        }
      }
    }

    SECTION("Removing after replacing throws") {
      sut::OpcodeStream stream(data, size);
      for (auto &i : stream) {
        if (i.GetOpcode() == spv::Op::OpCapability) {
          i.Replace(longer_instruction.data(), longer_instruction.size());
          REQUIRE_THROWS_AS(i.Remove(), sut::InvalidOperation);
        }
      }
    }
  }
}
