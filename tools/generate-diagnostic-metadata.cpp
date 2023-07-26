// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <cstdio>
#include <cstdlib>
#include <quick-lint-js/cli/arg-parser.h>
#include <quick-lint-js/cli/cli-location.h>
#include <quick-lint-js/container/concat.h>
#include <quick-lint-js/container/hash-map.h>
#include <quick-lint-js/container/padded-string.h>
#include <quick-lint-js/container/string-view.h>
#include <quick-lint-js/io/file.h>
#include <quick-lint-js/io/output-stream.h>
#include <quick-lint-js/port/char8.h>
#include <quick-lint-js/port/span.h>
#include <quick-lint-js/port/unreachable.h>
#include <quick-lint-js/port/warning.h>
#include <quick-lint-js/reflection/cxx-parser.h>
#include <quick-lint-js/util/algorithm.h>
#include <quick-lint-js/util/cpp.h>
#include <string_view>

QLJS_WARNING_IGNORE_GCC("-Wshadow=compatible-local")

using namespace std::literals::string_view_literals;

namespace quick_lint_js {
namespace {
// Returns the Diagnostic_Arg_Type enum value name for the given C++ type name.
//
// Returns an empty string on failure.
String8_View diagnostic_arg_type_code_from_type(String8_View type) {
#define QLJS_CASE(type_name, arg_type)             \
  do {                                             \
    if (type == QLJS_CPP_QUOTE_U8_SV(type_name)) { \
      return QLJS_CPP_QUOTE_U8_SV(arg_type);       \
    }                                              \
  } while (false)

  QLJS_CASE(Char8, char8);
  QLJS_CASE(Enum_Kind, enum_kind);
  QLJS_CASE(Source_Code_Span, source_code_span);
  QLJS_CASE(Statement_Kind, statement_kind);
  QLJS_CASE(String8_View, string8_view);
  QLJS_CASE(Variable_Kind, variable_kind);

#undef QLJS_CASE

  return u8""_sv;
}

void write_cxx_string_literal(Output_Stream& out, String8_View string) {
  out.append_copy(u8'"');
  for (Char8 c : string) {
    if (c == u8'"' || c == u8'\\') {
      out.append_copy(u8'\\');
      out.append_copy(c);
    } else {
      // TODO(strager): Escape other characters.
      out.append_copy(c);
    }
  }
  out.append_copy(u8'"');
}

void write_file_begin(Output_Stream& out) {
  out.append_literal(
      u8R"(// Code generated by tools/generate-diagnostic-metadata.cpp. DO NOT EDIT.
// source: src/quick-lint-js/diag/diagnostic-types-2.h

// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

)");
}

void write_file_end(Output_Stream& out) {
  out.append_literal(
      u8R"(
// quick-lint-js finds bugs in JavaScript programs.
// Copyright (C) 2020  Matthew "strager" Glazar
//
// This file is part of quick-lint-js.
//
// quick-lint-js is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// quick-lint-js is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with quick-lint-js.  If not, see <https://www.gnu.org/licenses/>.
)");
}

void write_type_list_h(Span<const CXX_Diagnostic_Type> types,
                       Output_Stream& out) {
  write_file_begin(out);

  out.append_literal(
      u8R"(#include <quick-lint-js/diag/diagnostic.h>

namespace quick_lint_js {
// clang-format off
#define QLJS_X_DIAG_TYPE_NAMES \
)"_sv);
  for (const CXX_Diagnostic_Type& type : types) {
    out.append_literal(u8"  QLJS_DIAG_TYPE_NAME("_sv);
    out.append_copy(type.name);
    out.append_literal(u8") \\\n"_sv);
  }
  out.append_literal(
      u8R"(  /* END */
// clang-format on
)"_sv);

  out.append_literal(u8"\ninline constexpr int Diag_Type_Count = "_sv);
  out.append_decimal_integer(types.size());
  out.append_literal(u8";\n"_sv);

  out.append_literal(
      u8"\nextern const Diagnostic_Info all_diagnostic_infos[Diag_Type_Count];\n"_sv);

  out.append_literal(u8"}\n"_sv);

  write_file_end(out);
}

void write_info_cpp(Span<const CXX_Diagnostic_Type> types, Output_Stream& out) {
  write_file_begin(out);

  out.append_literal(
      u8R"(#include <quick-lint-js/diag/diagnostic-metadata-generated.h>
#include <quick-lint-js/diag/diagnostic-types-2.h>
#include <quick-lint-js/diag/diagnostic.h>
#include <quick-lint-js/port/constinit.h>

namespace quick_lint_js {
// clang-format off
// If you see an error with the following lines, translation-table-generated.h
// is probably out of date. Run tools/update-translator-sources to rebuild this
// file.
const QLJS_CONSTINIT Diagnostic_Info all_diagnostic_infos[] = {
)");
  bool is_first = true;
  for (const CXX_Diagnostic_Type& type : types) {
    if (!is_first) {
      out.append_literal(u8"\n"_sv);
    }

    out.append_literal(u8"    // "_sv);
    out.append_copy(type.name);
    out.append_literal(u8"\n"_sv);

    out.append_literal(u8"    {\n"_sv);

    out.append_literal(u8"      .code = "_sv);
    out.append_decimal_integer(type.code_number());
    out.append_literal(u8",\n"_sv);

    out.append_literal(u8"      .severity = Diagnostic_Severity::"_sv);
    out.append_copy(type.severity);
    out.append_literal(u8",\n"_sv);

    out.append_literal(u8"      .message_formats = {\n"_sv);
    for (const CXX_Diagnostic_Message& message : type.messages) {
      out.append_literal(u8"        QLJS_TRANSLATABLE("_sv);
      write_cxx_string_literal(out, message.message);
      out.append_literal(u8"),\n"_sv);
    }
    out.append_literal(u8"      },\n"_sv);

    out.append_literal(u8"      .message_args = {\n"_sv);
    for (const CXX_Diagnostic_Message& message : type.messages) {
      out.append_literal(u8"        {\n"_sv);
      for (String8_View arg : message.argument_variables) {
        out.append_literal(
            u8"          Diagnostic_Message_Arg_Info(offsetof("_sv);
        out.append_copy(type.name);
        out.append_literal(u8", "_sv);
        out.append_copy(arg);
        out.append_literal(u8"), Diagnostic_Arg_Type::"_sv);
        const CXX_Diagnostic_Variable* var = type.variable_from_name(arg);
        if (var == nullptr) {
          out.append_literal(u8"(error: type not found)"_sv);
        } else {
          out.append_copy(diagnostic_arg_type_code_from_type(var->type));
        }
        out.append_literal(u8"),\n"_sv);
      }
      out.append_literal(u8"        },\n"_sv);
    }
    out.append_literal(u8"      },\n"_sv);

    out.append_literal(u8"    },\n"_sv);

    if (is_first) {
      is_first = false;
    }
  }

  out.append_literal(
      u8R"(};
}
)"_sv);

  write_file_end(out);
}
}
}

int main(int argc, char** argv) {
  using namespace quick_lint_js;

  const char* diagnostic_types_file_path = nullptr;
  const char* output_info_cpp_path = nullptr;
  const char* output_type_list_h_path = nullptr;
  Arg_Parser parser(argc, argv);
  QLJS_ARG_PARSER_LOOP(parser) {
    QLJS_ARGUMENT(const char* argument) {
      if (diagnostic_types_file_path != nullptr) {
        std::fprintf(stderr, "error: unexpected argument: %s\n", argument);
        std::exit(2);
      }
      diagnostic_types_file_path = argument;
    }

    QLJS_OPTION(const char* arg_value, "--output-info-cpp"sv) {
      output_info_cpp_path = arg_value;
    }

    QLJS_OPTION(const char* arg_value, "--output-type-list-h"sv) {
      output_type_list_h_path = arg_value;
    }

    QLJS_UNRECOGNIZED_OPTION(const char* unrecognized) {
      std::fprintf(stderr, "error: unrecognized option: %s\n", unrecognized);
      std::exit(2);
    }
  }
  if (diagnostic_types_file_path == nullptr) {
    std::fprintf(stderr, "error: missing path to diagnostic types file\n");
    std::exit(2);
  }

  Result<Padded_String, Read_File_IO_Error> diagnostic_types_source =
      read_file(diagnostic_types_file_path);
  if (!diagnostic_types_source.ok()) {
    std::fprintf(stderr, "error: %s\n",
                 diagnostic_types_source.error_to_string().c_str());
    std::exit(1);
  }

  CXX_Parser cxx_parser(diagnostic_types_file_path, &*diagnostic_types_source);
  cxx_parser.parse_file();

  if (!cxx_parser.check_diag_codes()) {
    std::exit(1);
  }

  {
    Result<Platform_File, Write_File_IO_Error> type_list_h =
        open_file_for_writing(output_type_list_h_path);
    if (!type_list_h.ok()) {
      std::fprintf(stderr, "error: %s\n",
                   type_list_h.error_to_string().c_str());
      std::exit(1);
    }
    File_Output_Stream out(type_list_h->ref());
    write_type_list_h(Span<const CXX_Diagnostic_Type>(cxx_parser.parsed_types),
                      out);
    out.flush();
  }

  {
    Result<Platform_File, Write_File_IO_Error> info_cpp =
        open_file_for_writing(output_info_cpp_path);
    if (!info_cpp.ok()) {
      std::fprintf(stderr, "error: %s\n", info_cpp.error_to_string().c_str());
      std::exit(1);
    }
    File_Output_Stream out(info_cpp->ref());
    write_info_cpp(Span<const CXX_Diagnostic_Type>(cxx_parser.parsed_types),
                   out);
    out.flush();
  }

  return 0;
}

// quick-lint-js finds bugs in JavaScript programs.
// Copyright (C) 2020  Matthew "strager" Glazar
//
// This file is part of quick-lint-js.
//
// quick-lint-js is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// quick-lint-js is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with quick-lint-js.  If not, see <https://www.gnu.org/licenses/>.
