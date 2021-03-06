/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/shader.h"

#include <cinttypes>
#include <cstring>

#include "third_party/fmt/include/fmt/format.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/math.h"
#include "xenia/base/memory.h"
#include "xenia/base/string.h"
#include "xenia/gpu/ucode.h"

namespace xe {
namespace gpu {
using namespace ucode;

Shader::Shader(xenos::ShaderType shader_type, uint64_t ucode_data_hash,
               const uint32_t* ucode_dwords, size_t ucode_dword_count)
    : shader_type_(shader_type), ucode_data_hash_(ucode_data_hash) {
  // We keep ucode data in host native format so it's easier to work with.
  ucode_data_.resize(ucode_dword_count);
  xe::copy_and_swap(ucode_data_.data(), ucode_dwords, ucode_dword_count);
}

Shader::~Shader() = default;

std::string Shader::GetTranslatedBinaryString() const {
  std::string result;
  result.resize(translated_binary_.size());
  std::memcpy(const_cast<char*>(result.data()), translated_binary_.data(),
              translated_binary_.size());
  return result;
}

std::pair<std::filesystem::path, std::filesystem::path> Shader::Dump(
    const std::filesystem::path& base_path, const char* path_prefix) {
  // Ensure target path exists.
  auto target_path = base_path;
  if (!target_path.empty()) {
    target_path = std::filesystem::absolute(target_path);
    std::filesystem::create_directories(target_path);
  }

  auto base_name =
      fmt::format("shader_{}_{:016X}", path_prefix, ucode_data_hash_);

  std::string txt_name, bin_name;
  if (shader_type_ == xenos::ShaderType::kVertex) {
    txt_name = base_name + ".vert";
    bin_name = base_name + ".bin.vert";
  } else {
    txt_name = base_name + ".frag";
    bin_name = base_name + ".bin.frag";
  }

  std::filesystem::path txt_path, bin_path;
  txt_path = base_path / txt_name;
  bin_path = base_path / bin_name;

  FILE* f = filesystem::OpenFile(txt_path, "wb");
  if (f) {
    fwrite(translated_binary_.data(), 1, translated_binary_.size(), f);
    fprintf(f, "\n\n");
    auto ucode_disasm_ptr = ucode_disassembly().c_str();
    while (*ucode_disasm_ptr) {
      auto line_end = std::strchr(ucode_disasm_ptr, '\n');
      fprintf(f, "// ");
      fwrite(ucode_disasm_ptr, 1, line_end - ucode_disasm_ptr + 1, f);
      ucode_disasm_ptr = line_end + 1;
    }
    fprintf(f, "\n\n");
    if (!host_disassembly_.empty()) {
      fprintf(f, "\n\n/*\n%s\n*/\n", host_disassembly_.c_str());
    }
    fclose(f);
  }

  f = filesystem::OpenFile(bin_path, "wb");
  if (f) {
    fwrite(ucode_data_.data(), 4, ucode_data_.size(), f);
    fclose(f);
  }

  return {std::move(txt_path), std::move(bin_path)};
}

}  //  namespace gpu
}  //  namespace xe
