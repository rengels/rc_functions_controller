/** Tests for the proc/proc_storage.h */

#include "proc_storage.h"
#include "simple_byte_stream.h"
#include <gtest/gtest.h>

using namespace rcProc;

/** Tests basic functionality for reading from stream.
 *
 *  Tests
 *
 *  - ProcStorage::deserialize()
 */
TEST(StorageTest, deserialize) {

    // test with invalid buffer

    uint8_t buf[20] = {'I', 'I', 1, 0};
    std::span<const uint8_t> sp(buf);

    SimpleInStream stream(sp);

    ProcStorage storage;
    EXPECT_FALSE(storage.deserialize(stream));
}

/** Tests basic functionality for writing from stream.
 *
 *  Tests
 *  - ProcStorage::serialize()
 *  - ProcStorage::deserialize()
 */
TEST(StorageTest, serialize) {

    ProcStorage storage;

    // -- test with default proc-storage
    SimpleOutStream out;
    storage.serialize(out);
    uint8_t* buf = out.buffer().data();

    // we should have a magic number in
    EXPECT_EQ('R', buf[0]);
    EXPECT_EQ('C', buf[1]);
    EXPECT_EQ(1, buf[2]);
    // number of procs
    EXPECT_TRUE(buf[3] > 3);
    // the next should be a group
    EXPECT_EQ('G', buf[4]);
    EXPECT_EQ('R', buf[5]);

    EXPECT_TRUE(out.tellg() > 20);


    // -- deserialize (shouldn't crash)
    std::span<const uint8_t> spc(out.buffer().data(), out.buffer().size());
    SimpleInStream in(spc);
    storage.deserialize(in);

    free(out.buffer().data());
}

