// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gin/gin_features.h"
#include "base/metrics/field_trial_params.h"

namespace features {

// Enable code space compaction when finalizing a full GC with stack.
BASE_FEATURE(kV8CompactCodeSpaceWithStack,
             "V8CompactCodeSpaceWithStack",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enable compaction when finalizing a full GC with stack.
BASE_FEATURE(kV8CompactWithStack,
             "V8CompactWithStack",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables compaction of maps in a full GC.
BASE_FEATURE(kV8CompactMaps,
             "V8CompactMaps",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Crashes on evacuation failures in a full GC instead of aborting evacuation.
BASE_FEATURE(kV8CrashOnEvacuationFailure,
             "V8CrashOnEvacuationFailure",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables a separate heap space for all map objects.
BASE_FEATURE(kV8UseMapSpace, "V8UseMapSpace", base::FEATURE_ENABLED_BY_DEFAULT);

// Enables optimization of JavaScript in V8.
BASE_FEATURE(kV8OptimizeJavascript,
             "V8OptimizeJavascript",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables flushing of JS bytecode in V8.
BASE_FEATURE(kV8FlushBytecode,
             "V8FlushBytecode",
             base::FEATURE_ENABLED_BY_DEFAULT);
const base::FeatureParam<int> kV8FlushBytecodeOldAge{
    &kV8FlushBytecode, "V8FlushBytecodeOldAge", 5};

// Enables flushing of baseline code in V8.
BASE_FEATURE(kV8FlushBaselineCode,
             "V8FlushBaselineCode",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables finalizing streaming JS compilations on a background thread.
BASE_FEATURE(kV8OffThreadFinalization,
             "V8OffThreadFinalization",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables lazy feedback allocation in V8.
BASE_FEATURE(kV8LazyFeedbackAllocation,
             "V8LazyFeedbackAllocation",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables per-context marking worklists in V8 GC.
BASE_FEATURE(kV8PerContextMarkingWorklist,
             "V8PerContextMarkingWorklist",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables flushing of the instruction cache for the embedded blob.
BASE_FEATURE(kV8FlushEmbeddedBlobICache,
             "V8FlushEmbeddedBlobICache",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables reduced number of concurrent marking tasks.
BASE_FEATURE(kV8ReduceConcurrentMarkingTasks,
             "V8ReduceConcurrentMarkingTasks",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Disables reclaiming of unmodified wrappers objects.
BASE_FEATURE(kV8NoReclaimUnmodifiedWrappers,
             "V8NoReclaimUnmodifiedWrappers",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables W^X code memory protection in V8.
// This is enabled in V8 by default. To test the performance impact, we are
// going to disable this feature in a finch experiment.
BASE_FEATURE(kV8CodeMemoryWriteProtection,
             "V8CodeMemoryWriteProtection",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables fallback to a breadth-first regexp engine on excessive backtracking.
BASE_FEATURE(kV8ExperimentalRegexpEngine,
             "V8ExperimentalRegexpEngine",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables experimental Maglev compiler.
BASE_FEATURE(kV8Maglev, "V8Maglev", base::FEATURE_DISABLED_BY_DEFAULT);

// Enables Sparkplug compiler. Note that this only sets the V8 flag when
// manually overridden; otherwise it defers to whatever the V8 default is.
BASE_FEATURE(kV8Sparkplug, "V8Sparkplug", base::FEATURE_ENABLED_BY_DEFAULT);

// Enables the concurrent Sparkplug compiler.
BASE_FEATURE(kV8ConcurrentSparkplug,
             "V8ConcurrentSparkplug",
             base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<int> kV8ConcurrentSparkplugMaxThreads{
    &kV8ConcurrentSparkplug, "V8ConcurrentSparkplugMaxThreads", 0};
BASE_FEATURE(kV8ConcurrentSparkplugHighPriorityThreads,
             "V8ConcurrentSparkplugHighPriorityThreads",
             base::FEATURE_DISABLED_BY_DEFAULT);
// Makes sure the experimental Sparkplug compiler is only enabled if short
// builtin calls are enabled too.
BASE_FEATURE(kV8SparkplugNeedsShortBuiltinCalls,
             "V8SparkplugNeedsShortBuiltinCalls",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Enables short builtin calls feature.
BASE_FEATURE(kV8ShortBuiltinCalls,
             "V8ShortBuiltinCalls",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Enables fast API calls in TurboFan.
BASE_FEATURE(kV8TurboFastApiCalls,
             "V8TurboFastApiCalls",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Artificially delays script execution.
BASE_FEATURE(kV8ScriptAblation,
             "V8ScriptAblation",
             base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<int> kV8ScriptDelayOnceMs{&kV8ScriptAblation,
                                                   "V8ScriptDelayOnceMs", 0};
const base::FeatureParam<int> kV8ScriptDelayMs{&kV8ScriptAblation,
                                               "V8ScriptDelayMs", 0};
const base::FeatureParam<double> kV8ScriptDelayFraction{
    &kV8ScriptAblation, "V8ScriptDelayFraction", 0.0};

// Enables slow histograms that provide detailed information at increased
// runtime overheads.
BASE_FEATURE(kV8SlowHistograms,
             "V8SlowHistograms",
             base::FEATURE_DISABLED_BY_DEFAULT);
// Multiple finch experiments might use slow-histograms. We introduce
// separate feature flags to circumvent finch limitations.
BASE_FEATURE(kV8SlowHistogramsCodeMemoryWriteProtection,
             "V8SlowHistogramsCodeMemoryWriteProtection",
             base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kV8SlowHistogramsSparkplug,
             "V8SlowHistogramsSparkplug",
             base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kV8SlowHistogramsSparkplugAndroid,
             "V8SlowHistogramsSparkplugAndroid",
             base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kV8SlowHistogramsScriptAblation,
             "V8SlowHistogramsScriptAblation",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kV8DelayMemoryReducer,
             "V8DelayMemoryReducer",
             base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<base::TimeDelta> kV8MemoryReducerStartDelay{
    &kV8DelayMemoryReducer, "delay", base::Seconds(8)};

}  // namespace features
