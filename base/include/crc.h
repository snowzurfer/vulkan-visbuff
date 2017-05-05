#ifndef SZT_CRC
#define SZT_CRC

#include <cstdint>

// http://www.codeproject.com/Articles/4251/Spoofing-the-Wily-Zip-CRC
namespace szt {

class CRC {
public:
  static uint32_t GetCRC(const char *_pString);
  static uint32_t GetICRC(const char *_pString);
  CRC(uint32_t _r = ~0);

private:
  void Update(const char *pbuf, int len,
              bool toUpper = false);      // update crc residual
  inline uint32_t GetU32() { return ~r; } // object yields current CRC

  void Clk(int i = 0); // clock in data, bit at a time
  void ClkRev();       // clk residual backwards

  enum { gf = 0xdb710641 }; // This is the generator
  uint32_t r;               // residual, polynomial mod gf
};                          // class CRC

} // namespace szt

#endif
