// Copyright 2022 Google LLC
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

#include "internal/platform/bluetooth_connection_info.h"

#include <algorithm>
#include <string>

namespace location {
namespace nearby {

ByteArray BluetoothConnectionInfo::ToBytes() const {
  return ByteArray(absl::StrCat(mac_address_.AsStringView(), service_id_));
}

BluetoothConnectionInfo BluetoothConnectionInfo::FromBytes(ByteArray bytes) {
  std::string serial(bytes.AsStringView());
  ByteArray mac_address = ByteArray(serial.substr(0, kMacAddressLength));
  std::string service_id = serial.substr(kMacAddressLength);
  return BluetoothConnectionInfo(mac_address, service_id);
}
}  // namespace nearby
}  // namespace location
