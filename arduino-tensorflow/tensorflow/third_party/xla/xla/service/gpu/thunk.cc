/* Copyright 2017 The OpenXLA Authors.

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

#include "xla/service/gpu/thunk.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "absl/algorithm/container.h"
#include "absl/container/inlined_vector.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "xla/executable_run_options.h"
#include "xla/hlo/ir/hlo_instruction.h"
#include "xla/service/gpu/buffer_allocations.h"
#include "xla/service/gpu/gpu_executable_run_options.h"
#include "xla/service/service_executable_run_options.h"
#include "xla/stream_executor/stream.h"
#include "xla/translate/mhlo_to_hlo/location_exporter.h"
#include "tsl/platform/statusor.h"

namespace xla {
namespace gpu {

absl::StatusOr<Thunk::ExecuteParams> Thunk::ExecuteParams::Create(
    const ServiceExecutableRunOptions& run_options,
    const BufferAllocations& buffer_allocations, se::Stream* stream,
    se::Stream* command_buffer_trace_stream,
    absl::Span<se::Stream* const> async_streams) {
  TF_ASSIGN_OR_RETURN(auto nccl_params,
                      NcclExecuteParams::Create(
                          run_options, stream->parent()->device_ordinal()));
  return ExecuteParams(&buffer_allocations, stream, command_buffer_trace_stream,
                       {async_streams.begin(), async_streams.end()},
                       std::move(nccl_params),
                       run_options.run_options().device_to_host_stream(),
                       run_options.run_options().host_to_device_stream(),
                       run_options.run_options().send_device_memory_function(),
                       run_options.run_options().recv_device_memory_function());
}

Thunk::ExecuteParams::ExecuteParams(
    const BufferAllocations* buffer_allocations, se::Stream* stream,
    se::Stream* command_buffer_trace_stream,
    absl::InlinedVector<se::Stream*, 4> async_comms_streams,
    NcclExecuteParams nccl_params, se::Stream* device_to_host_stream,
    se::Stream* host_to_device_stream,
    SendDeviceMemoryFunction* send_device_memory_function,
    RecvDeviceMemoryFunction* recv_device_memory_function)
    : buffer_allocations(buffer_allocations),
      stream(stream),
      command_buffer_trace_stream(command_buffer_trace_stream),
      async_comms_streams(async_comms_streams),
      nccl_params(std::move(nccl_params)),
      device_to_host_stream(device_to_host_stream),
      host_to_device_stream(host_to_device_stream),
      send_device_memory_function(send_device_memory_function),
      recv_device_memory_function(recv_device_memory_function) {}

/*static*/ absl::string_view Thunk::KindToString(Thunk::Kind kind) {
#define CASE(x)  \
  case Thunk::x: \
    return #x
  switch (kind) {
    CASE(kCholesky);
    CASE(kCommandBuffer);
    CASE(kConditional);
    CASE(kConvolution);
    CASE(kConvolutionReorder);
    CASE(kCopy);
    CASE(kCubSort);
    CASE(kCublasLtMatmul);
    CASE(kCustomCall);
    CASE(kCustomKernel);
    CASE(kNcclAllGather);
    CASE(kNcclAllGatherStart);
    CASE(kNcclAllGatherDone);
    CASE(kNcclAllReduce);
    CASE(kNcclAllReduceStart);
    CASE(kNcclAllReduceDone);
    CASE(kNcclCollectivePermute);
    CASE(kNcclCollectivePermuteStart);
    CASE(kNcclCollectivePermuteDone);
    CASE(kNcclReduceScatter);
    CASE(kNcclReduceScatterStart);
    CASE(kNcclReduceScatterDone);
    CASE(kNcclAllToAll);
    CASE(kNcclAllToAllStart);
    CASE(kNcclAllToAllDone);
    CASE(kNcclSend);
    CASE(kNcclRecv);
    CASE(kFft);
    CASE(kFor);
    CASE(kGemm);
    CASE(kInfeed);
    CASE(kKernel);
    CASE(kMemset32BitValue);
    CASE(kMemzero);
    CASE(kNorm);
    CASE(kOutfeed);
    CASE(kSend);
    CASE(kSendDone);
    CASE(kPartitionId);
    CASE(kReplicaId);
    CASE(kRecv);
    CASE(kRecvDone);
    CASE(kSequential);
    CASE(kTriangularSolve);
    CASE(kWhile);
    CASE(kFusedMHA);
  }
}

std::ostream& operator<<(std::ostream& os, Thunk::Kind kind) {
  return os << Thunk::KindToString(kind);
}

std::string ThunkSequence::ToString(
    int indent,
    std::function<std::string(const Thunk*)> get_thunk_annotation) const {
  const std::string indent_str(indent * 2, ' ');
  if (empty()) return indent_str + "No thunks.";

  auto thunk_with_longest_kind = absl::c_max_element(
      *this,
      [](const std::unique_ptr<Thunk>& a, const std::unique_ptr<Thunk>& b) {
        return Thunk::KindToString(a->kind()).length() <
               Thunk::KindToString(b->kind()).length();
      });
  int64_t max_thunk_kind_len =
      Thunk::KindToString(thunk_with_longest_kind->get()->kind()).length();
  std::string result;
  for (const std::unique_ptr<Thunk>& thunk : *this) {
    // Write out the thunk kind, padded out to max_thunk_kind_len.
    absl::string_view kind_str = Thunk::KindToString(thunk->kind());
    absl::StrAppend(&result, indent_str, kind_str,
                    std::string(max_thunk_kind_len - kind_str.length(), ' '),
                    "\t");
    if (get_thunk_annotation) {
      absl::StrAppend(&result, get_thunk_annotation(thunk.get()));
    }
    absl::StrAppend(&result, thunk->ToStringExtra(indent));
    absl::StrAppend(&result, "\n");
  }
  return result;
}

bool IsReductionCollective(Thunk::Kind kind) {
  return kind == Thunk::kNcclAllReduce || kind == Thunk::kNcclAllReduceStart ||
         kind == Thunk::kNcclReduceScatter ||
         kind == Thunk::kNcclReduceScatterStart;
}

Thunk::ThunkInfo Thunk::ThunkInfo::WithProfileAnnotation(mlir::Operation* op) {
  ThunkInfo thunk_info(op);
  thunk_info.profile_annotation = absl::StrFormat(
      "Thunk:#hlo_op=%s#", mlir::mhlo::GetDebugNameFromLocation(op->getLoc()));
  return thunk_info;
}

Thunk::ThunkInfo Thunk::ThunkInfo::WithProfileAnnotation(
    const HloInstruction* instr) {
  ThunkInfo thunk_info(nullptr);
  thunk_info.profile_annotation =
      absl::StrFormat("Thunk:#hlo_op=%s#", instr->name());
  return thunk_info;
}

}  // namespace gpu
}  // namespace xla
