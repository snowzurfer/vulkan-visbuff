# spirv-utils
Library which allows parsing and filtering of a spir-v binary module.

## Getting Started
### Prerequisites
* CMake 2.8 (or newer); see [below](#no-cmake) for usage 
  without cmake

### Example
```cpp
#include <spv_utils.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
  // Read spv binary module from a file
  std::ifstream spv_file("sample_spv_modules/test.frag.spv",
                         std::ios::binary | std::ios::ate | std::ios::in);
  if (spv_file.is_open()) {
    std::streampos size = spv_file.tellg();

    spv_file.seekg(0, std::ios::beg);
    char *data = new char[size];
    spv_file.read(data, size);
    spv_file.close();

    // Parse the module and create the object
    sut::OpcodeStream stream(data, size);

    // Iterate through the stream of instructions
    for (auto &i : stream) {
      if (i.GetOpcode() == spv::Op::OpCapability) {
        // Use the library's helper struct to define fictitious headers
        sut::OpcodeHeader header_0 =
          {1U, static_cast<uint16_t>(spv::Op::OpNop)};
        sut::OpcodeHeader header_1 =
          {4U, static_cast<uint16_t>(spv::Op::OpNop)};

        uint32_t instruction_0 = MergeSpvOpCode(header_0);
        uint32_t instruction_1 = MergeSpvOpCode(header_1);

        std::array<uint32_t, 4U> longer_instruction = 
          {instruction_1, 0xDEADBEEF,
           0xDEADBEEF, 0xDEADBEEF};
        std::array<uint32_t, 4U> longer_instruction_2 =
          {instruction_1, 0x1EADBEEF,
           0x1EADBEEF, 0x1EADBEEF};

        // Append before current instruction in LIFO order
        i.InsertBefore(longer_instruction.data(), longer_instruction.size());
        // Append after current instruction in LIFO order
        i.InsertAfter(&instruction, 1U);
        // Append after current instruction in LIFO order
        i.InsertAfter(longer_instruction.data(), longer_instruction.size());
        // Append after current instruction in LIFO order
        i.InsertAfter(longer_instruction.data(), longer_instruction.size());
        // Append before current instruction in LIFO order
        i.InsertBefore(longer_instruction_2.data(),
                       longer_instruction_2.size());
        // Remove the current instruction
        i.Remove();
        
        // Replace() calls Remove(). Also, it will throw if called after 
        // Remove() or if vice-versa, but it is shown here to explain usage.
        i.Replace(longer_instruction_2.data(),
                  longer_instruction_2.size());
      }
    }

    sut::OpcodeStream filtered_stream = stream.EmitFilteredStream();
    std::vector<uint32_t> filtered_module = filtered_stream.GetWordsStream();

    // Use the filtered module here...
    // ...
    //

    std::vector<uint32_t> original_module = stream.GetWordsStream();

    // Use the original module here...
    // ...
    //

    delete[] data;
    data = nullptr;
  }

  return 0;
};
```
## [Documentation](https://github.com/snowzurfer/spirv-utils/wiki/Documentation)
Please visit the Wiki or click
[here](https://github.com/snowzurfer/spirv-utils/wiki/Documentation)
to navigate to it.

## Installing
### CMake usage
* Clone the repo (or download the zip version):
```
git clone https://github.com/snowzurfer/spirv-utils.git
```

* Add the project as a subdirectory in your root CMakeLists:
```
add_subdirectory(${PATH_TO_SUT_ROOT_DIR})    
```
    
* The CMakeLists defines the library  
```
sut  
```
the variables  
```
${SUT_HEADERS} , ${SUT_SOURCES}  
```
and the option  
```
SUT_BUILD_TESTS
```
  
  By default, the tests won't be built (the option is set to
  `OFF`)
  and the build
  type will be `Debug`.    

  You can set these options either via the tick boxes
  in the CMake GUI or via
  command-line arguments.


* Link the library
```
target_link_libraries(your_target, sut)
```

* Use the library as shown in the example at the top of the `README`.


### <a name="no-cmake"></a>Usage without CMake
* Clone the repo (or download the zip version):
```
git clone https://github.com/snowzurfer/spirv-utils.git
```
* Setup your build to include the main source and include files, plus the
  external library's files.

## Running the tests
Set the option `SUT_BUILD_TESTS` to `ON` and re-build your project.
Now, every time you build, the tests will be run on the unit tests defined
in the `CMakeLists` of the `unit_tests` folder.

## Built with
* [Catch](http://github.com/philsquared/Catch) - The unit testing framework used
* [SPIRV-Headers](http://github.com/KhronosGroup/SPIRV-Headers/)
* [CMake](http://cmake.org)

## Tested platforms and compilers
* ArchLinux, kernel 4.9.6-1 x86_64, g++ 6.3.1 20170109
* Windows 10 x86_64, Visual Studio 2015, Toolset v140

## Authors
* **Alberto Taiuti** - *Developer* -
[@snowzurfer](https://github.com/snowzurfer)

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file
for details

## Acknowledgments
* **Dr Matthäus G. Chajdas** - *Mentoring and design* - [@Anteru](https://github.com/Anteru)
