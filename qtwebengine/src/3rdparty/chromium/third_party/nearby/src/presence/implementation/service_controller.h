// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_NEARBY_PRESENCE_IMPLEMENTATION_SERVICE_CONTROLLER_H_
#define THIRD_PARTY_NEARBY_PRESENCE_IMPLEMENTATION_SERVICE_CONTROLLER_H_

namespace nearby {
namespace presence {

/*
 * This class is owned in {@code PresenceService}. It specifies the function
 * signatures. {@code ServiceControllerImpl} and {@code MockServiceController}
 * inherit this class and provides real implementation and mock impl for tests.
 */
class ServiceController {
 public:
  virtual ~ServiceController() = default;
};

}  // namespace presence
}  // namespace nearby

#endif  // THIRD_PARTY_NEARBY_PRESENCE_IMPLEMENTATION_SERVICE_CONTROLLER_H_
