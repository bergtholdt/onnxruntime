// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <core/platform/ort_mutex.h>
#include <core/framework/ort_event.h>
#include "sync_api.h"

template <typename T>
class FixedCountFinishCallbackImpl {
 private:
  //remain tasks
  int s_;
  onnxruntime::OrtMutex m_;
  ORT_EVENT finish_event_;
  bool failed = false;
  std::vector<std::shared_ptr<T>> results_;

 public:
  FixedCountFinishCallbackImpl(const FixedCountFinishCallbackImpl&) = delete;
  FixedCountFinishCallbackImpl& operator=(const FixedCountFinishCallbackImpl&) = delete;

  const std::vector<std::shared_ptr<T>>& getResults() const {
    return results_;
  }

  FixedCountFinishCallbackImpl(int s) : s_(s), results_(s) {
    ORT_THROW_ON_ERROR(OrtCreateEvent(&finish_event_));
  }

  ~FixedCountFinishCallbackImpl() {
    if (finish_event_) OrtReleaseEvent(finish_event_);
  }

  ::onnxruntime::common::Status fail(ORT_CALLBACK_INSTANCE pci) {
    {
      std::lock_guard<onnxruntime::OrtMutex> g(m_);
      failed = true;
      s_ = 0;  //fail earlier
    }
    return OnnxRuntimeSetEventWhenCallbackReturns(pci, finish_event_);
  }

  ::onnxruntime::common::Status onFinished(size_t task_index, std::shared_ptr<T> result, ORT_CALLBACK_INSTANCE pci) {
    int v;
    {
      std::lock_guard<onnxruntime::OrtMutex> g(m_);
      v = --s_;
      results_.at(task_index) = result;
    }
    if (v == 0) {
      return OnnxRuntimeSetEventWhenCallbackReturns(pci, finish_event_);
    }
    return ::onnxruntime::common::Status::OK();
  }

  bool shouldStop() {
    std::lock_guard<onnxruntime::OrtMutex> g(m_);
    return failed;
  }
  //this function can only be invoked once
  bool wait() {
    ORT_THROW_ON_ERROR(OrtWaitAndCloseEvent(finish_event_));
    {
      std::lock_guard<onnxruntime::OrtMutex> g(m_);
      finish_event_ = nullptr;
      return !failed;
    }
  }
};
