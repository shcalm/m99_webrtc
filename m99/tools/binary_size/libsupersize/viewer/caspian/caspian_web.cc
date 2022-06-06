// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Command-line interface for checking the integrity of .size files.
// Intended to be called from WebAssembly code.

#include <stdint.h>
#include <stdlib.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "third_party/jsoncpp/source/include/json/json.h"
#include "third_party/re2/src/re2/re2.h"
#include "tools/binary_size/libsupersize/viewer/caspian/diff.h"
#include "tools/binary_size/libsupersize/viewer/caspian/file_format.h"
#include "tools/binary_size/libsupersize/viewer/caspian/lens.h"
#include "tools/binary_size/libsupersize/viewer/caspian/model.h"
#include "tools/binary_size/libsupersize/viewer/caspian/tree_builder.h"

namespace caspian {
namespace {
std::unique_ptr<SizeInfo> info;
std::unique_ptr<SizeInfo> before_info;
std::unique_ptr<DeltaSizeInfo> diff_info;
std::unique_ptr<TreeBuilder> builder;

std::unique_ptr<Json::StreamWriter> writer;

std::string JsonSerialize(const Json::Value& value) {
  if (!writer) {
    writer.reset(Json::StreamWriterBuilder().newStreamWriter());
  }
  std::stringstream s;
  writer->write(value, &s);
  return s.str();
}

re2::StringPiece Re2StringPiece(std::string_view str) {
  return re2::StringPiece(str.data(), str.size());
}

bool MatchesRegex(const GroupedPath& id_path,
                  const BaseSymbol& sym,
                  const RE2& regex) {
  if (RE2::PartialMatch(Re2StringPiece(id_path.path), regex)) {
    return true;
  }
  if (RE2::PartialMatch(Re2StringPiece(id_path.group), regex)) {
    return true;
  }
  return RE2::PartialMatch(Re2StringPiece(sym.FullName()), regex);
}

bool IsMultiContainer() {
  // If DeltaSizeInfo is active, still take |info| since it's the "after" info.
  return info->containers.size() > 1 || !info->containers[0].name.empty();
}

}  // namespace

extern "C" {
void LoadSizeFile(char* compressed, size_t size) {
  if (IsDiffSizeInfo(compressed, size)) {
    info = std::make_unique<SizeInfo>();
    before_info = std::make_unique<SizeInfo>();
    diff_info.reset(nullptr);
    ParseDiffSizeInfo(compressed, size, before_info.get(), info.get());
  } else {
    diff_info.reset(nullptr);
    info = std::make_unique<SizeInfo>();
    ParseSizeInfo(compressed, size, info.get());
  }
}

void LoadBeforeSizeFile(const char* compressed, size_t size) {
  diff_info.reset(nullptr);
  before_info = std::make_unique<SizeInfo>();
  ParseSizeInfo(compressed, size, before_info.get());
}

// Updates |builder| with provided filters and constructs the new tree.
// Typically called when the front-end form updates, to apply any new filters.
// Returns: True if the resulting tree is a diff, false if it is a snapshot.
bool BuildTree(bool method_count_mode,
               const char* group_by,
               const char* include_regex_str,
               const char* exclude_regex_str,
               const char* include_sections,
               int minimum_size_bytes,
               int match_flag) {
  std::vector<TreeBuilder::FilterFunc> filters;

  const bool diff_mode = info && before_info;

  if (method_count_mode && diff_mode) {
    // include_sections is used to filter to just .dex.method symbols.
    // For diffs, we also want to filter to just adds & removes.
    filters.push_back([](const GroupedPath&, const BaseSymbol& sym) -> bool {
      DiffStatus diff_status = sym.GetDiffStatus();
      return diff_status == DiffStatus::kAdded ||
             diff_status == DiffStatus::kRemoved;
    });
  }

  if (minimum_size_bytes > 0) {
    if (!diff_mode) {
      filters.push_back([minimum_size_bytes](const GroupedPath&,
                                             const BaseSymbol& sym) -> bool {
        return sym.Pss() >= minimum_size_bytes;
      });
    } else {
      filters.push_back([minimum_size_bytes](const GroupedPath&,
                                             const BaseSymbol& sym) -> bool {
        return abs(sym.Pss()) >= minimum_size_bytes;
      });
    }
  }

  // It's currently not useful to filter on more than one flag, so
  // |match_flag| can be assumed to be a power of two.
  if (match_flag) {
    std::cout << "Filtering on flag matching " << match_flag << std::endl;
    filters.push_back(
        [match_flag](const GroupedPath&, const BaseSymbol& sym) -> bool {
          return match_flag & sym.Flags();
        });
  }

  std::array<bool, 256> include_sections_map{};
  if (include_sections) {
    std::cout << "Filtering on sections " << include_sections << std::endl;
    for (const char* c = include_sections; *c; c++) {
      include_sections_map[static_cast<uint8_t>(*c)] = true;
    }
    filters.push_back([&include_sections_map](const GroupedPath&,
                                              const BaseSymbol& sym) -> bool {
      return include_sections_map[static_cast<uint8_t>(sym.Section())];
    });
  }

  std::unique_ptr<RE2> include_regex;
  if (include_regex_str && *include_regex_str) {
    include_regex.reset(new RE2(include_regex_str));
    if (include_regex->error_code() == RE2::NoError) {
      filters.push_back([&include_regex](const GroupedPath& id_path,
                                         const BaseSymbol& sym) -> bool {
        return MatchesRegex(id_path, sym, *include_regex);
      });
    }
  }

  std::unique_ptr<RE2> exclude_regex;
  if (exclude_regex_str && *exclude_regex_str) {
    exclude_regex.reset(new RE2(exclude_regex_str));
    if (exclude_regex->error_code() == RE2::NoError) {
      filters.push_back([&exclude_regex](const GroupedPath& id_path,
                                         const BaseSymbol& sym) -> bool {
        return !MatchesRegex(id_path, sym, *exclude_regex);
      });
    }
  }

  // BuildTree() is called every time a new filter is applied in the HTML
  // viewer, but if we already have a DeltaSizeInfo we can skip regenerating it
  // and let the TreeBuilder filter the symbols we care about.
  if (diff_mode && !diff_info) {
    diff_info.reset(new DeltaSizeInfo(Diff(before_info.get(), info.get())));
  }

  if (diff_mode) {
    builder.reset(new TreeBuilder(diff_info.get()));
  } else {
    builder.reset(new TreeBuilder(info.get()));
  }

  std::unique_ptr<BaseLens> lens;
  char sep = '/';
  std::cout << "group_by=" << group_by << std::endl;
  if (!strcmp(group_by, "source_path")) {
    lens = std::make_unique<IdPathLens>();
  } else if (!strcmp(group_by, "container")) {
    lens = std::make_unique<ContainerLens>();
  } else if (!strcmp(group_by, "component")) {
    lens = std::make_unique<ComponentLens>();
    sep = '>';
  } else if (!strcmp(group_by, "template")) {
    lens = std::make_unique<TemplateLens>();
    filters.push_back([](const GroupedPath&, const BaseSymbol& sym) -> bool {
      return sym.IsTemplate() && sym.IsNative();
    });
  } else if (!strcmp(group_by, "generated_type")) {
    lens = std::make_unique<GeneratedLens>();
  } else {
    std::cerr << "Unsupported group_by=" << group_by << std::endl;
    exit(1);
  }
  builder->Build(std::move(lens), sep, method_count_mode, filters);

  return bool(diff_info);
}

// Returns a string that can be parsed to a JS object.
const char* Open(const char* path) {
  static std::string result;
  Json::Value v = builder->Open(path);
  result = JsonSerialize(v);
  return result.c_str();
}

// Returns global properties.
const char* QueryProperty(const char* key) {
  if (!strcmp(key, "isMultiContainer")) {
    return IsMultiContainer() ? "true" : "false";
  }
  std::cerr << "Unknown property: " << key << std::endl;
  exit(1);
}

}  // extern "C"
}  // namespace caspian
