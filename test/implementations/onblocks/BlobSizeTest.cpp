#include "testutils/BlobStoreTest.h"
#include <messmer/cpp-utils/data/Data.h>
#include <messmer/cpp-utils/data/DataFixture.h>

using std::unique_ptr;

using namespace blobstore;
using blockstore::Key;
using cpputils::Data;
using cpputils::DataFixture;

class BlobSizeTest: public BlobStoreTest {
public:
  BlobSizeTest(): blob(blobStore->create()) {}

  static constexpr uint32_t MEDIUM_SIZE = 5 * 1024 * 1024;
  static constexpr uint32_t LARGE_SIZE = 10 * 1024 * 1024;

  unique_ptr<Blob> blob;
};
constexpr uint32_t BlobSizeTest::MEDIUM_SIZE;
constexpr uint32_t BlobSizeTest::LARGE_SIZE;

TEST_F(BlobSizeTest, CreatedBlobIsEmpty) {
  EXPECT_EQ(0, blob->size());
}

TEST_F(BlobSizeTest, Growing_1Byte) {
  blob->resize(1);
  EXPECT_EQ(1, blob->size());
}

TEST_F(BlobSizeTest, Growing_Large) {
  blob->resize(LARGE_SIZE);
  EXPECT_EQ(LARGE_SIZE, blob->size());
}

TEST_F(BlobSizeTest, Shrinking_Empty) {
  blob->resize(LARGE_SIZE);
  blob->resize(0);
  EXPECT_EQ(0, blob->size());
}

TEST_F(BlobSizeTest, Shrinking_1Byte) {
  blob->resize(LARGE_SIZE);
  blob->resize(1);
  EXPECT_EQ(1, blob->size());
}

TEST_F(BlobSizeTest, ResizingToItself_Empty) {
  blob->resize(0);
  EXPECT_EQ(0, blob->size());
}

TEST_F(BlobSizeTest, ResizingToItself_1Byte) {
  blob->resize(1);
  blob->resize(1);
  EXPECT_EQ(1, blob->size());
}

TEST_F(BlobSizeTest, ResizingToItself_Large) {
  blob->resize(LARGE_SIZE);
  blob->resize(LARGE_SIZE);
  EXPECT_EQ(LARGE_SIZE, blob->size());
}

TEST_F(BlobSizeTest, EmptyBlobStaysEmptyWhenLoading) {
  Key key = blob->key();
  blob.reset();
  auto loaded = blobStore->load(key);
  EXPECT_EQ(0, loaded->size());
}

TEST_F(BlobSizeTest, BlobSizeStaysIntactWhenLoading) {
  blob->resize(LARGE_SIZE);
  Key key = blob->key();
  blob.reset();
  auto loaded = blobStore->load(key);
  EXPECT_EQ(LARGE_SIZE, loaded->size());
}

TEST_F(BlobSizeTest, WritingAtEndOfBlobGrowsBlob_Empty) {
  int value;
  blob->write(&value, 0, 4);
  EXPECT_EQ(4, blob->size());
}

TEST_F(BlobSizeTest, WritingAfterEndOfBlobGrowsBlob_Empty) {
  int value;
  blob->write(&value, 2, 4);
  EXPECT_EQ(6, blob->size());
}

TEST_F(BlobSizeTest, WritingOverEndOfBlobGrowsBlob_NonEmpty) {
  blob->resize(1);
  int value;
  blob->write(&value, 0, 4);
  EXPECT_EQ(4, blob->size());
}

TEST_F(BlobSizeTest, WritingAtEndOfBlobGrowsBlob_NonEmpty) {
  blob->resize(1);
  int value;
  blob->write(&value, 1, 4);
  EXPECT_EQ(5, blob->size());
}

TEST_F(BlobSizeTest, WritingAfterEndOfBlobGrowsBlob_NonEmpty) {
  blob->resize(1);
  int value;
  blob->write(&value, 2, 4);
  EXPECT_EQ(6, blob->size());
}

TEST_F(BlobSizeTest, ChangingSizeImmediatelyFlushes) {
  blob->resize(LARGE_SIZE);
  auto loaded = blobStore->load(blob->key());
  EXPECT_EQ(LARGE_SIZE, loaded->size());
}

class BlobSizeDataTest: public BlobSizeTest {
public:
  BlobSizeDataTest()
    :ZEROES(LARGE_SIZE),
     randomData(DataFixture::generate(LARGE_SIZE)) {
    ZEROES.FillWithZeroes();
  }

  Data readBlob(const Blob &blob) {
    Data data(blob.size());
    blob.read(data.data(), 0, data.size());
    return data;
  }

  Data ZEROES;
  Data randomData;
};

TEST_F(BlobSizeDataTest, BlobIsZeroedOutAfterGrowing) {
  blob->resize(LARGE_SIZE);
  EXPECT_EQ(0, std::memcmp(readBlob(*blob).data(), ZEROES.data(), LARGE_SIZE));
}

TEST_F(BlobSizeDataTest, BlobIsZeroedOutAfterGrowingAndLoading) {
  blob->resize(LARGE_SIZE);
  auto loaded = blobStore->load(blob->key());
  EXPECT_EQ(0, std::memcmp(readBlob(*loaded).data(), ZEROES.data(), LARGE_SIZE)); 
}

TEST_F(BlobSizeDataTest, DataStaysIntactWhenGrowing) {
  blob->resize(MEDIUM_SIZE);
  blob->write(randomData.data(), 0, MEDIUM_SIZE);
  blob->resize(LARGE_SIZE);
  EXPECT_EQ(0, std::memcmp(readBlob(*blob).data(), randomData.data(), MEDIUM_SIZE));
  EXPECT_EQ(0, std::memcmp((uint8_t*)readBlob(*blob).data() + MEDIUM_SIZE, ZEROES.data(), LARGE_SIZE-MEDIUM_SIZE));
}

TEST_F(BlobSizeDataTest, DataStaysIntactWhenShrinking) {
  blob->resize(LARGE_SIZE);
  blob->write(randomData.data(), 0, LARGE_SIZE);
  blob->resize(MEDIUM_SIZE);
  EXPECT_EQ(0, std::memcmp(readBlob(*blob).data(), randomData.data(), MEDIUM_SIZE));
}

TEST_F(BlobSizeDataTest, ChangedAreaIsZeroedOutWhenShrinkingAndRegrowing) {
  blob->resize(LARGE_SIZE);
  blob->write(randomData.data(), 0, LARGE_SIZE);
  blob->resize(MEDIUM_SIZE);
  blob->resize(LARGE_SIZE);
  EXPECT_EQ(0, std::memcmp(readBlob(*blob).data(), randomData.data(), MEDIUM_SIZE));
  EXPECT_EQ(0, std::memcmp((uint8_t*)readBlob(*blob).data() + MEDIUM_SIZE, ZEROES.data(), LARGE_SIZE-MEDIUM_SIZE));
}
