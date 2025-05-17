/** Tests for the SimpleByteStream classe
 *
 *  Note: since we are transmitting between
 *  javascript and data view we are big endian.
 *
 */

#include "simple_byte_stream.h"
#include "signals.h"
#include <array>
#include <span>
#include <gtest/gtest.h>

using namespace rcSignals;

/** Tests basic functionality for reading from stream.
 *
 *  Tests
 *
 *  - rc::SimpleInStream.eof()
 *  - rc::SimpleInStream.readUint8()
 *  - rc::SimpleInStream.tellg()
 *  - rc::SimpleInStream.seekg()
 */
TEST(SimpleInStreamTest, Reading) {

  uint8_t buf[] = {0x01u, 0x02u};
  std::span<const uint8_t> sp(buf);

  SimpleInStream stream(sp);

  EXPECT_FALSE(stream.eof());
  EXPECT_FALSE(stream.fail());
  EXPECT_EQ(0, stream.tellg());
  EXPECT_EQ(0x01u, stream.readUint8());

  EXPECT_EQ(1, stream.tellg());
  EXPECT_FALSE(stream.eof());
  EXPECT_EQ(0x02u, stream.readUint8());

  EXPECT_TRUE(stream.eof());
  EXPECT_FALSE(stream.fail());
  EXPECT_EQ(2, stream.tellg());
  EXPECT_EQ(0x00u, stream.readUint8());
  EXPECT_EQ(2, stream.tellg());
  EXPECT_TRUE(stream.fail());

  stream.seekg(1u);
  EXPECT_EQ(1, stream.tellg());
  EXPECT_FALSE(stream.eof());
  EXPECT_TRUE(stream.fail());
  EXPECT_EQ(0x02u, stream.readUint8());
  EXPECT_TRUE(stream.fail());
}

/** Tests basic functionality for writing from stream.
 *
 *  Tests
 *
 *  - rc::SimpleOutStream.eof()
 *  - rc::SimpleOutStream.writeUint8()
 *  - rc::SimpleInStream.tellg()
 *  - rc::SimpleInStream.seekg()
 */
TEST(SimpleOutStreamTest, Writing) {

  SimpleOutStream stream;
  uint8_t* buf = stream.buffer().data();

  EXPECT_EQ(0, stream.tellg());
  EXPECT_NE(0, stream.buffer().size()); // initial buffer
  EXPECT_NE(nullptr, stream.buffer().data());
  EXPECT_FALSE(stream.eof());
  EXPECT_FALSE(stream.fail());
  stream.writeUint8(0x01U);

  EXPECT_EQ(1, stream.tellg());
  EXPECT_FALSE(stream.eof());
  EXPECT_FALSE(stream.fail());
  stream.writeUint8(0x02U);

  EXPECT_EQ(2, stream.tellg());
  EXPECT_FALSE(stream.eof());
  EXPECT_FALSE(stream.fail());

  EXPECT_EQ(0x01U, buf[0]);
  EXPECT_EQ(0x02U, buf[1]);

  free(stream.buffer().data());
}

/** Tests realloc
 *
 *  Writes until the end of the initial buffer
 *  and check if a realloc with a new size happened.
 *
 *  - rc::SimpleOutStream.eof()
 *  - rc::SimpleOutStream.writeUint8()
 *  - rc::SimpleInStream.tellg()
 *  - rc::SimpleInStream.seekg()
 */
TEST(SimpleOutStreamTest, Realloc) {

  SimpleOutStream stream;
  auto origBuf = stream.buffer();

  // write beyond end
  stream.seekg(stream.buffer().size());
  stream.writeUint8(0x01U);

  EXPECT_NE(origBuf.data(), stream.buffer().data());
  EXPECT_LT(origBuf.size(), stream.buffer().size());
  EXPECT_FALSE(stream.eof());
  EXPECT_FALSE(stream.fail());

  free(stream.buffer().data());
}

TEST(SimpleOutStreamTest, WritingOperation) {

  SimpleOutStream stream;
  uint8_t* buf = stream.buffer().data();

  stream.seekg(0);
  uint8_t byte = 0x12;
  stream << byte;
  EXPECT_EQ(0x12U, buf[0]);

  stream.seekg(0);
  uint16_t word = 0x6789;
  stream << word;
  EXPECT_EQ(0x67U, buf[0]);
  EXPECT_EQ(0x89U, buf[1]);

  stream.seekg(0);
  uint32_t longWord = 0x12345678U;
  stream << longWord;
  EXPECT_EQ(0x12U, buf[0]);
  EXPECT_EQ(0x34U, buf[1]);
  EXPECT_EQ(0x56U, buf[2]);
  EXPECT_EQ(0x78U, buf[3]);

  EXPECT_FALSE(stream.eof());
  EXPECT_FALSE(stream.fail());

  free(stream.buffer().data());
}

/** Test the template write and read functions */
TEST(SimpleOutStreamTest, Template) {

  SimpleOutStream os;
  uint8_t* buf = os.buffer().data();

  os.write<uint32_t>(0x12345678u);
  EXPECT_EQ(0x12U, buf[0]);
  EXPECT_EQ(0x34U, buf[1]);
  EXPECT_EQ(0x56U, buf[2]);
  EXPECT_EQ(0x78U, buf[3]);

  std::span<const uint8_t> spc(os.buffer().data(), os.buffer().size());
  SimpleInStream is(spc);
  auto longWord = is.read<uint32_t>();
  EXPECT_EQ(0x12345678u, longWord);

  free(os.buffer().data());
}

TEST(SimpleOutStreamTest, int32) {

  SimpleOutStream os;
  uint8_t* buf = os.buffer().data();

  os.write<int16_t>(-1);
  os.write<int16_t>(-32768);
  EXPECT_EQ(0xFFU, buf[0]);
  EXPECT_EQ(0xFFU, buf[1]);
  EXPECT_EQ(0x80U, buf[2]);
  EXPECT_EQ(0x00U, buf[3]);

  std::span<const uint8_t> spc(os.buffer().data(), os.buffer().size());
  SimpleInStream is(spc);
  auto word = is.read<int16_t>();
  EXPECT_EQ(-1, word);
  word = is.read<int16_t>();
  EXPECT_EQ(-32768, word);

  free(os.buffer().data());
}

/** Tests reading and writing Signal Types
 *
 *  Tests
 *
 *  - rc::SimpleByteStream.readSignalTypes()
 *  - rc::SimpleByteStream.writeSignalTypes()
 */
TEST(SimpleByteStreamTest, SignalTypes) {

  std::array<SignalType, 3> types1 = {
      SignalType::ST_YAW,
      SignalType::ST_THROTTLE,
      SignalType::ST_THROTTLE_RIGHT};

  std::array<SignalType, 3> types2 = {
      SignalType::ST_BRAKE,
      SignalType::ST_BRAKE,
      SignalType::ST_BRAKE};

  SimpleOutStream os;
  uint8_t* buf = os.buffer().data();
  os << types1;
  EXPECT_EQ(0x01U, buf[0]);
  EXPECT_EQ(0x02U, buf[1]);
  EXPECT_EQ(0x03U, buf[2]);
  EXPECT_FALSE(os.eof());
  EXPECT_FALSE(os.fail());

  std::span<const uint8_t> spc(os.buffer().data(), os.buffer().size());
  SimpleInStream in(spc);
  in >> types2;
  EXPECT_EQ(SignalType::ST_YAW, types2[0]);
  EXPECT_EQ(SignalType::ST_THROTTLE, types2[1]);
  EXPECT_EQ(SignalType::ST_THROTTLE_RIGHT, types2[2]);
  EXPECT_FALSE(in.eof());
  EXPECT_FALSE(in.fail());

  free(os.buffer().data());
}
