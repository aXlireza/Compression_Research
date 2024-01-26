/* Copyright 2023 The OpenXLA Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef XLA_SERVICE_GPU_TOPK_SPECIALIZER_H_
#define XLA_SERVICE_GPU_TOPK_SPECIALIZER_H_

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "xla/executable_run_options.h"
#include "xla/hlo/ir/hlo_module.h"
#include "xla/service/hlo_pass_interface.h"
#include "xla/statusor.h"
#include "tsl/platform/statusor.h"

namespace xla::gpu {

// This pass transforms eligible TopK CustomCall into a call to be executed by
// runtime/topk.cc.
class TopkSpecializer : public HloModulePass {
 public:
  absl::string_view name() const override { return "topk-specializer"; }

  using HloPassInterface::Run;
  absl::StatusOr<bool> Run(
      HloModule* module,
      const absl::flat_hash_set<absl::string_view>& execution_threads) override;
};

}  // namespace xla::gpu

#endif  // XLA_SERVICE_GPU_TOPK_SPECIALIZER_H_