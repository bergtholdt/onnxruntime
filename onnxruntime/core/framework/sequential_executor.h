// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <vector>
#include <unordered_map>
#include "core/common/common.h"
#include "core/common/status.h"
#include "core/common/logging/logging.h"
#include "core/framework/iexecutor.h"
#include "core/framework/framework_common.h"
#include "core/framework/ml_value.h"
#include "core/framework/session_state.h"
#include "core/graph/graph_viewer.h"
#ifdef ONNXRUNTIME_ENABLE_INSTRUMENT
#include "core/platform/tracing.h"
#include <TraceLoggingActivity.h>
#endif

namespace onnxruntime {
class SequentialExecutor : public IExecutor {
 public:
#ifdef ONNXRUNTIME_ENABLE_INSTRUMENT
  SequentialExecutor(const void* ortrun_activity, const bool& terminate_flag = false)
      : terminate_flag_{terminate_flag}, ortrun_activity_((const TraceLoggingActivity<ort_provider>*)ortrun_activity) {}
#else
  SequentialExecutor(const void*, const bool& terminate_flag = false) : terminate_flag_{terminate_flag} {}
#endif


  common::Status Execute(const SessionState& session_state, const std::vector<int>& feed_mlvalue_idxs,
                         const std::vector<OrtValue>& feeds, const std::vector<int>& fetch_mlvalue_idxs,
                         std::vector<OrtValue>& fetches,
                         const std::unordered_map<size_t, CustomAllocator>& fetch_allocators,
                         const logging::Logger& logger) override;

 private:
  ORT_DISALLOW_COPY_ASSIGNMENT_AND_MOVE(SequentialExecutor);
  const bool& terminate_flag_;
#ifdef ONNXRUNTIME_ENABLE_INSTRUMENT
  const TraceLoggingActivity<ort_provider>* ortrun_activity_;
#endif
};
}  // namespace onnxruntime
