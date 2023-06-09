// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BREADCRUMBS_CORE_BREADCRUMB_MANAGER_H_
#define COMPONENTS_BREADCRUMBS_CORE_BREADCRUMB_MANAGER_H_

#include <list>
#include <string>

#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/time/time.h"

namespace breadcrumbs {

class BreadcrumbManagerObserver;

// Stores events logged with |AddEvent| in memory which can later be retrieved
// with |GetEvents|. Events will be silently dropped after a certain amount of
// time has passed unless no more recent events are available. The internal
// management of events aims to keep relevant events available while clearing
// stale data.
class BreadcrumbManager {
 public:
  // Returns the singleton BreadcrumbManager. Creates it if it does not exist.
  static BreadcrumbManager& GetInstance();

  BreadcrumbManager(const BreadcrumbManager&) = delete;
  BreadcrumbManager& operator=(const BreadcrumbManager&) = delete;

  // Returns a list of the collected breadcrumb events which are still relevant.
  // Events returned will have a timestamp prepended to the original `event`
  // string representing when `AddEvent` was called.
  // Note: This method may drop old events so the returned events can change
  // even if no new events have been added, but time has passed.
  const std::list<std::string> GetEvents();

  // Logs a breadcrumb event with message data |event|.
  // NOTE: |event| must not include newline characters as newlines are used by
  // BreadcrumbPersistentStorageManager as a delimiter.
  void AddEvent(const std::string& event);

  // Adds and removes observers.
  void AddObserver(BreadcrumbManagerObserver* observer);
  void RemoveObserver(BreadcrumbManagerObserver* observer);

  // Resets timestamps to 0:00:00 and removes all events. Does not remove
  // observers or notify observers about removed events.
  void ResetForTesting();

 private:
  friend class base::NoDestructor<BreadcrumbManager>;

  BreadcrumbManager();
  ~BreadcrumbManager();

  // Drops events which are considered stale. Note that stale events are not
  // guaranteed to be removed. Explicitly, stale events will be retained while
  // newer events are limited.
  void DropOldEvents();

  // Returns the time since |start_time_|.
  base::TimeDelta GetElapsedTime();

  // The time when breadcrumbs logging started, used to calculate elapsed time
  // for event timestamps.
  base::TimeTicks start_time_ = base::TimeTicks::Now();

  // List of events, paired with the time they were logged in minutes. Newer
  // events are at the end of the list.
  struct EventBucket {
    int minutes_elapsed;
    std::list<std::string> events;

    explicit EventBucket(int minutes_elapsed);
    EventBucket(const EventBucket&);
    ~EventBucket();
  };
  std::list<EventBucket> event_buckets_;

  base::ObserverList<BreadcrumbManagerObserver, /*check_empty=*/true>
      observers_;
};

}  // namespace breadcrumbs

#endif  // COMPONENTS_BREADCRUMBS_CORE_BREADCRUMB_MANAGER_H_
