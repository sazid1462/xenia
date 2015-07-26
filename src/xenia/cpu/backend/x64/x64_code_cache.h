/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_BACKEND_X64_X64_CODE_CACHE_H_
#define XENIA_BACKEND_X64_X64_CODE_CACHE_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "xenia/base/memory.h"
#include "xenia/base/mutex.h"
#include "xenia/cpu/backend/code_cache.h"

namespace xe {
namespace cpu {
namespace backend {
namespace x64 {

class X64CodeCache : public CodeCache {
 public:
  ~X64CodeCache() override;

  static std::unique_ptr<X64CodeCache> Create();

  virtual bool Initialize();

  std::wstring file_name() const override { return file_name_; }
  uint32_t base_address() const override { return kGeneratedCodeBase; }
  uint32_t total_size() const override { return kGeneratedCodeSize; }

  // TODO(benvanik): ELF serialization/etc
  // TODO(benvanik): keep track of code blocks
  // TODO(benvanik): padding/guards/etc

  void set_indirection_default(uint32_t default_value);
  void AddIndirection(uint32_t guest_address, uint32_t host_address);

  void CommitExecutableRange(uint32_t guest_low, uint32_t guest_high);

  void* PlaceCode(uint32_t guest_address, void* machine_code, size_t code_size,
                  size_t stack_size);

  uint32_t PlaceData(const void* data, size_t length);

 protected:
  const static uint64_t kIndirectionTableBase = 0x80000000;
  const static uint64_t kIndirectionTableSize = 0x1FFFFFFF;
  const static uint64_t kGeneratedCodeBase = 0xA0000000;
  const static uint64_t kGeneratedCodeSize = 0x0FFFFFFF;

  struct UnwindReservation {
    size_t data_size = 0;
    size_t table_slot = 0;
    uint8_t* entry_address = 0;
  };

  X64CodeCache();

  virtual UnwindReservation RequestUnwindReservation(uint8_t* entry_address) {
    return UnwindReservation();
  }
  virtual void PlaceCode(uint32_t guest_address, void* machine_code,
                         size_t code_size, size_t stack_size,
                         void* code_address,
                         UnwindReservation unwind_reservation) {}

  std::wstring file_name_;
  xe::memory::FileMappingHandle mapping_ = nullptr;

  // Must be held when manipulating the offsets or counts of anything, to keep
  // the tables consistent and ordered.
  xe::mutex allocation_mutex_;

  // Value that the indirection table will be initialized with upon commit.
  uint32_t indirection_default_value_ = 0xFEEDF00D;

  // Fixed at kIndirectionTableBase in host space, holding 4 byte pointers into
  // the generated code table that correspond to the PPC functions in guest
  // space.
  uint8_t* indirection_table_base_ = nullptr;
  // Fixed at kGeneratedCodeBase and holding all generated code, growing as
  // needed.
  uint8_t* generated_code_base_ = nullptr;
  // Current offset to empty space in generated code.
  size_t generated_code_offset_ = 0;
  // Current high water mark of COMMITTED code.
  std::atomic<size_t> generated_code_commit_mark_ = {0};
};

}  // namespace x64
}  // namespace backend
}  // namespace cpu
}  // namespace xe

#endif  // XENIA_BACKEND_X64_X64_CODE_CACHE_H_