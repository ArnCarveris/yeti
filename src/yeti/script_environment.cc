//===-- yeti/script_environment.cc ------------------------*- mode: C++ -*-===//
//
//                             __ __     _   _
//                            |  |  |___| |_|_|
//                            |_   _| -_|  _| |
//                              |_| |___|_| |_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#include "yeti/script_environment.h"

#include "yeti/script.h"

namespace yeti {

ScriptEnvironment::ScriptEnvironment() {
  this->reset();
}

ScriptEnvironment::~ScriptEnvironment() {
}

void ScriptEnvironment::reset() {
  this->counts.vec2 = 0;
  this->counts.vec3 = 0;
  this->counts.vec4 = 0;
}

} // yeti
