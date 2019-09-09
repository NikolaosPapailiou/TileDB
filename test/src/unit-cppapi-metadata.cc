/**
 * @file unit-cppapi-metadata.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017-2019 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * Tests the C++ API for array metadata.
 */

#include "test/src/helpers.h"
#include "tiledb/sm/c_api/tiledb.h"
#include "tiledb/sm/c_api/tiledb_struct_def.h"
#include "tiledb/sm/cpp_api/tiledb"

#ifdef _WIN32
#include "tiledb/sm/filesystem/win.h"
#else
#include "tiledb/sm/filesystem/posix.h"
#endif

#include <catch.hpp>
#include <chrono>
#include <iostream>
#include <thread>

using namespace tiledb;

/* ********************************* */
/*         STRUCT DEFINITION         */
/* ********************************* */

struct CPPMetadataFx {
  tiledb_ctx_t* ctx_;
  tiledb_vfs_t* vfs_;
  bool s3_supported_, hdfs_supported_;
  std::string temp_dir_;
  const std::string s3_bucket_name_ =
      "s3://" + random_bucket_name("tiledb") + "/";
  std::string array_name_;
  const char* ARRAY_NAME = "test_metadata";
  tiledb_array_t* array_ = nullptr;
  const char* key_ = "0123456789abcdeF0123456789abcdeF";
  const uint32_t key_len_ =
      (uint32_t)strlen("0123456789abcdeF0123456789abcdeF");
  const tiledb_encryption_type_t enc_type_ = TILEDB_AES_256_GCM;

  void create_default_array_1d();
  void create_default_array_1d_with_key();

  CPPMetadataFx();
  ~CPPMetadataFx();
};

CPPMetadataFx::CPPMetadataFx() {
  ctx_ = nullptr;
  vfs_ = nullptr;
  hdfs_supported_ = false;
  s3_supported_ = false;

  get_supported_fs(&s3_supported_, &hdfs_supported_);
  create_ctx_and_vfs(s3_supported_, &ctx_, &vfs_);
  create_s3_bucket(s3_bucket_name_, s3_supported_, ctx_, vfs_);

// Create temporary directory based on the supported filesystem
#ifdef _WIN32
  temp_dir_ = tiledb::sm::Win::current_dir() + "\\tiledb_test\\";
#else
  temp_dir_ = "file://" + tiledb::sm::Posix::current_dir() + "/tiledb_test/";
#endif
  create_dir(temp_dir_, ctx_, vfs_);

  array_name_ = temp_dir_ + ARRAY_NAME;
  int rc = tiledb_array_alloc(ctx_, array_name_.c_str(), &array_);
  CHECK(rc == TILEDB_OK);
}

CPPMetadataFx::~CPPMetadataFx() {
  tiledb_array_free(&array_);
  remove_dir(temp_dir_, ctx_, vfs_);
  tiledb_ctx_free(&ctx_);
  tiledb_vfs_free(&vfs_);
}

void CPPMetadataFx::create_default_array_1d() {
  uint64_t domain[] = {1, 10};
  uint64_t tile_extent = 5;
  create_array(
      ctx_,
      array_name_,
      TILEDB_DENSE,
      {"d"},
      {TILEDB_UINT64},
      {domain},
      {&tile_extent},
      {"a", "b", "c"},
      {TILEDB_INT32, TILEDB_CHAR, TILEDB_FLOAT32},
      {1, TILEDB_VAR_NUM, 2},
      {::Compressor(TILEDB_FILTER_NONE, -1),
       ::Compressor(TILEDB_FILTER_ZSTD, -1),
       ::Compressor(TILEDB_FILTER_LZ4, -1)},
      TILEDB_ROW_MAJOR,
      TILEDB_ROW_MAJOR,
      2);
}

void CPPMetadataFx::create_default_array_1d_with_key() {
  uint64_t domain[] = {1, 10};
  uint64_t tile_extent = 5;
  create_array(
      ctx_,
      array_name_,
      enc_type_,
      key_,
      key_len_,
      TILEDB_DENSE,
      {"d"},
      {TILEDB_UINT64},
      {domain},
      {&tile_extent},
      {"a", "b", "c"},
      {TILEDB_INT32, TILEDB_CHAR, TILEDB_FLOAT32},
      {1, TILEDB_VAR_NUM, 2},
      {::Compressor(TILEDB_FILTER_NONE, -1),
       ::Compressor(TILEDB_FILTER_ZSTD, -1),
       ::Compressor(TILEDB_FILTER_LZ4, -1)},
      TILEDB_ROW_MAJOR,
      TILEDB_ROW_MAJOR,
      2);
}

/* ********************************* */
/*                TESTS              */
/* ********************************* */

TEST_CASE_METHOD(
    CPPMetadataFx,
    "C++ API: Metadata, basic errors",
    "[cppapi][metadata][errors]") {
  // Create default array
  create_default_array_1d();

  // Put metadata in an array opened for reads - error
  Context ctx;
  Array array(ctx, std::string(array_name_), TILEDB_READ);
  int v = 5;
  CHECK_THROWS(array.put_metadata("key", TILEDB_INT32, 1, &v));
  array.close();

  // Reopen array in WRITE mode
  array.open(TILEDB_WRITE);

  // Write null value
  CHECK_THROWS(array.put_metadata("key", TILEDB_INT32, 1, NULL));

  // Write zero values
  CHECK_THROWS(array.put_metadata("key", TILEDB_INT32, 0, &v));

  // Write value type ANY
  CHECK_THROWS(array.put_metadata("key", TILEDB_ANY, 1, &v));

  // Write a correct item
  array.put_metadata("key", TILEDB_INT32, 1, &v);

  // Close array
  array.close();

  // Open with key
  CHECK_THROWS(array.open(TILEDB_READ, enc_type_, key_, key_len_));
}

TEST_CASE_METHOD(
    CPPMetadataFx,
    "C++ API: Metadata, write/read",
    "[cppapi][metadata][read]") {
  // Create default array
  create_default_array_1d();

  // Open array in write mode
  Context ctx;
  Array array(ctx, std::string(array_name_), TILEDB_WRITE);

  // Write items
  int32_t v = 5;
  array.put_metadata("aaa", TILEDB_INT32, 1, &v);
  float f[] = {1.1f, 1.2f};
  array.put_metadata("bb", TILEDB_FLOAT32, 2, f);

  // Close array
  array.close();

  // Open the array in read mode
  array.open(TILEDB_READ);

  // Read
  const void* v_r;
  tiledb_datatype_t v_type;
  uint32_t v_num;
  array.get_metadata("aaa", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 5);

  array.get_metadata("bb", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_FLOAT32);
  CHECK(v_num == 2);
  CHECK(((const float_t*)v_r)[0] == 1.1f);
  CHECK(((const float_t*)v_r)[1] == 1.2f);

  array.get_metadata("foo", &v_type, &v_num, &v_r);
  CHECK(v_r == nullptr);

  uint64_t num = array.metadata_num();
  CHECK(num == 2);

  std::string key;
  CHECK_THROWS(array.get_metadata_from_index(10, &key, &v_type, &v_num, &v_r));

  array.get_metadata_from_index(1, &key, &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_FLOAT32);
  CHECK(v_num == 2);
  CHECK(((const float_t*)v_r)[0] == 1.1f);
  CHECK(((const float_t*)v_r)[1] == 1.2f);
  CHECK(key.size() == strlen("bb"));
  CHECK(!strncmp(key.data(), "bb", strlen("bb")));

  // Close array
  array.close();
}

TEST_CASE_METHOD(
    CPPMetadataFx, "C++ API: Metadata, UTF-8", "[cppapi][metadata][utf-8]") {
  // Create default array
  create_default_array_1d();

  // Open array in write mode
  Context ctx;
  Array array(ctx, std::string(array_name_), TILEDB_WRITE);

  // Write UTF-8 (≥ holds 3 bytes)
  int32_t v = 5;
  array.put_metadata("≥", TILEDB_INT32, 1, &v);

  // Close array
  array.close();

  // Open the array in read mode
  array.open(TILEDB_READ);

  // Read
  const void* v_r;
  tiledb_datatype_t v_type;
  uint32_t v_num;
  array.get_metadata("≥", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 5);

  std::string key;
  array.get_metadata_from_index(0, &key, &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 5);
  CHECK(key.size() == strlen("≥"));
  CHECK(!strncmp(key.data(), "≥", strlen("≥")));

  // Close array
  array.close();
}

TEST_CASE_METHOD(
    CPPMetadataFx, "C++ API: Metadata, delete", "[cppapi][metadata][delete]") {
  // Create default array
  create_default_array_1d();

  // Create and open array in write mode
  Context ctx;
  Array array(ctx, std::string(array_name_), TILEDB_WRITE);

  // Write items
  int32_t v = 5;
  array.put_metadata("aaa", TILEDB_INT32, 1, &v);
  float f[] = {1.1f, 1.2f};
  array.put_metadata("bb", TILEDB_FLOAT32, 2, f);

  // Close array
  array.close();

  // Prevent array metadata filename/timestamp conflicts
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Delete an item that exists and one that does not exist
  array.open(TILEDB_WRITE);
  array.delete_metadata("aaa");
  array.delete_metadata("foo");
  array.close();

  // Open the array in read mode
  array.open(TILEDB_READ);

  // Read
  const void* v_r;
  tiledb_datatype_t v_type;
  uint32_t v_num;
  array.get_metadata("aaa", &v_type, &v_num, &v_r);
  CHECK(v_r == nullptr);

  array.get_metadata("bb", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_FLOAT32);
  CHECK(v_num == 2);
  CHECK(((const float_t*)v_r)[0] == 1.1f);
  CHECK(((const float_t*)v_r)[1] == 1.2f);

  array.get_metadata("foo", &v_type, &v_num, &v_r);
  CHECK(v_r == nullptr);

  uint64_t num = array.metadata_num();
  CHECK(num == 1);

  std::string key;
  array.get_metadata_from_index(0, &key, &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_FLOAT32);
  CHECK(v_num == 2);
  CHECK(((const float_t*)v_r)[0] == 1.1f);
  CHECK(((const float_t*)v_r)[1] == 1.2f);
  CHECK(key.size() == strlen("bb"));
  CHECK(!strncmp(key.data(), "bb", strlen("bb")));

  // Close array
  array.close();
}

TEST_CASE_METHOD(
    CPPMetadataFx,
    "C++ API: Metadata, multiple metadata and cosnolidate",
    "[cppapi][metadata][multiple][consolidation]") {
  // Create default array
  create_default_array_1d();

  // Create and open array in write mode
  Context ctx;
  Array array(ctx, array_name_, TILEDB_WRITE);

  // Write items
  int32_t v = 5;
  array.put_metadata("aaa", TILEDB_INT32, 1, &v);
  float f[] = {1.1f, 1.2f};
  array.put_metadata("bb", TILEDB_FLOAT32, 2, f);

  // Close array
  array.close();

  // Prevent array metadata filename/timestamp conflicts
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Update
  array.open(TILEDB_WRITE);
  array.delete_metadata("aaa");
  v = 10;
  array.put_metadata("cccc", TILEDB_INT32, 1, &v);
  array.close();

  // Open the array in read mode
  array.open(TILEDB_READ);

  // Read
  const void* v_r;
  tiledb_datatype_t v_type;
  uint32_t v_num;
  array.get_metadata("aaa", &v_type, &v_num, &v_r);
  CHECK(v_r == nullptr);

  array.get_metadata("bb", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_FLOAT32);
  CHECK(v_num == 2);
  CHECK(((const float_t*)v_r)[0] == 1.1f);
  CHECK(((const float_t*)v_r)[1] == 1.2f);

  array.get_metadata("cccc", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 10);

  uint64_t num = array.metadata_num();
  CHECK(num == 2);

  std::string key;
  array.get_metadata_from_index(0, &key, &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_FLOAT32);
  CHECK(v_num == 2);
  CHECK(((const float_t*)v_r)[0] == 1.1f);
  CHECK(((const float_t*)v_r)[1] == 1.2f);
  CHECK(key.size() == strlen("bb"));
  CHECK(!strncmp(key.data(), "bb", strlen("bb")));

  // Close array
  array.close();

  // Consolidate
  Array::consolidate_metadata(ctx, array_name_, nullptr);

  // Open the array in read mode
  array.open(TILEDB_READ);

  num = array.metadata_num();
  CHECK(num == 2);

  // Close array
  array.close();

  // Write once more
  array.open(TILEDB_WRITE);

  // Write items
  v = 50;
  array.put_metadata("d", TILEDB_INT32, 1, &v);

  // Close array
  array.close();

  // Consolidate again
  Array::consolidate_metadata(ctx, array_name_, nullptr);

  // Open the array in read mode
  array.open(TILEDB_READ);

  num = array.metadata_num();
  CHECK(num == 3);

  array.get_metadata("cccc", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 10);

  array.get_metadata("d", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 50);

  // Close array
  array.close();
}

TEST_CASE_METHOD(
    CPPMetadataFx, "C++ Metadata, open at", "[cppapi][metadata][open-at]") {
  // Create default array
  create_default_array_1d();

  // Create and open array in write mode
  Context ctx;
  Array array(ctx, array_name_, TILEDB_WRITE);

  // Write items
  int32_t v = 5;
  array.put_metadata("aaa", TILEDB_INT32, 1, &v);
  float f[] = {1.1f, 1.2f};
  array.put_metadata("bb", TILEDB_FLOAT32, 2, f);

  // Close array
  array.close();

  // Prevent array metadata filename/timestamp conflicts
  auto timestamp = tiledb::sm::utils::time::timestamp_now_ms();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Update
  array.open(TILEDB_WRITE);
  array.delete_metadata("aaa");
  array.close();

  // Open the array in read mode at a timestamp
  array.open(TILEDB_READ, timestamp);

  // Read
  const void* v_r;
  tiledb_datatype_t v_type;
  uint32_t v_num;
  array.get_metadata("aaa", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 5);

  uint64_t num = array.metadata_num();
  CHECK(num == 2);

  // Close array
  array.close();
}

TEST_CASE_METHOD(
    CPPMetadataFx, "C++ Metadata, reopen", "[cppapi][metadata][reopen]") {
  // Create default array
  create_default_array_1d();

  // Open array in write mode
  Context ctx;
  Array array(ctx, array_name_, TILEDB_WRITE);

  // Write items
  int32_t v = 5;
  array.put_metadata("aaa", TILEDB_INT32, 1, &v);
  float f[] = {1.1f, 1.2f};
  array.put_metadata("bb", TILEDB_FLOAT32, 2, f);

  // Close array
  array.close();

  // Prevent array metadata filename/timestamp conflicts
  auto timestamp = tiledb::sm::utils::time::timestamp_now_ms();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Update
  array.open(TILEDB_WRITE);
  array.delete_metadata("aaa");
  array.close();

  // Open the array in read mode at a timestamp
  array.open(TILEDB_READ, timestamp);

  // Read
  const void* v_r;
  tiledb_datatype_t v_type;
  uint32_t v_num;
  array.get_metadata("aaa", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 5);

  uint64_t num = array.metadata_num();
  CHECK(num == 2);

  // Reopen
  array.reopen();

  // Read
  array.get_metadata("aaa", &v_type, &v_num, &v_r);
  CHECK(v_r == nullptr);

  num = array.metadata_num();
  CHECK(num == 1);

  // Close array
  array.close();
}

TEST_CASE_METHOD(
    CPPMetadataFx,
    "C++ Metadata, encryption",
    "[cppapi][metadata][encryption]") {
  // Create default array
  create_default_array_1d_with_key();

  // Create and open array in write mode
  Context ctx;
  Array array(ctx, array_name_, TILEDB_WRITE, enc_type_, key_, key_len_);

  // Write items
  int32_t v = 5;
  array.put_metadata("aaa", TILEDB_INT32, 1, &v);
  float f[] = {1.1f, 1.2f};
  array.put_metadata("bb", TILEDB_FLOAT32, 2, f);

  // Close array
  array.close();

  // Prevent array metadata filename/timestamp conflicts
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Update
  array.open(TILEDB_WRITE, enc_type_, key_, key_len_);
  array.delete_metadata("aaa");
  v = 10;
  array.put_metadata("cccc", TILEDB_INT32, 1, &v);
  array.close();

  // Open the array in read mode
  array.open(TILEDB_READ, enc_type_, key_, key_len_);

  // Read
  const void* v_r;
  tiledb_datatype_t v_type;
  uint32_t v_num;
  array.get_metadata("aaa", &v_type, &v_num, &v_r);
  CHECK(v_r == nullptr);

  array.get_metadata("bb", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_FLOAT32);
  CHECK(v_num == 2);
  CHECK(((const float_t*)v_r)[0] == 1.1f);
  CHECK(((const float_t*)v_r)[1] == 1.2f);

  array.get_metadata("cccc", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 10);

  uint64_t num = array.metadata_num();
  CHECK(num == 2);

  std::string key;
  array.get_metadata_from_index(0, &key, &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_FLOAT32);
  CHECK(v_num == 2);
  CHECK(((const float_t*)v_r)[0] == 1.1f);
  CHECK(((const float_t*)v_r)[1] == 1.2f);
  CHECK(key.size() == strlen("bb"));
  CHECK(!strncmp(key.data(), "bb", strlen("bb")));

  // Close array
  array.close();

  // Consolidate without key - error
  CHECK_THROWS(Array::consolidate_metadata(ctx, array_name_, nullptr));

  // Consolidate with key - ok
  Array::consolidate_metadata(
      ctx, array_name_, enc_type_, key_, key_len_, nullptr);

  // Open the array in read mode
  array.open(TILEDB_READ, enc_type_, key_, key_len_);

  num = array.metadata_num();
  CHECK(num == 2);

  // Close array
  array.close();

  // Write once more
  array.open(TILEDB_WRITE, enc_type_, key_, key_len_);

  // Write items
  v = 50;
  array.put_metadata("d", TILEDB_INT32, 1, &v);

  // Close array
  array.close();

  // Consolidate again
  Array::consolidate_metadata(
      ctx, array_name_, enc_type_, key_, key_len_, nullptr);

  // Open the array in read mode
  array.open(TILEDB_READ, enc_type_, key_, key_len_);

  num = array.metadata_num();
  CHECK(num == 3);

  array.get_metadata("cccc", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 10);

  array.get_metadata("d", &v_type, &v_num, &v_r);
  CHECK(v_type == TILEDB_INT32);
  CHECK(v_num == 1);
  CHECK(*((const int32_t*)v_r) == 50);

  // Close array
  array.close();
}