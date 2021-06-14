// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#ifndef QUICK_LINT_JS_PIPE_WRITER_H
#define QUICK_LINT_JS_PIPE_WRITER_H

#include <quick-lint-js/char8.h>
#include <quick-lint-js/file-handle.h>

namespace quick_lint_js {
class byte_buffer_iovec;

class pipe_writer {
 public:
  explicit pipe_writer(platform_file_ref pipe);

  void write(byte_buffer_iovec&&);

 private:
  platform_file_ref pipe_;
};
}

#endif

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
