/*
 * Copyright (C) 2019 GlobalLogic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OEMLOCK_HAL_H_
#define OEMLOCK_HAL_H_

#include <string>
#include <mutex>
#include <inttypes.h>

#include <android/hardware/oemlock/1.0/IOemLock.h>
#include <hidl/Status.h>

extern "C" {
    #include <tee_client_api.h>
}
#include "oemlock_ta.h"

namespace android {
namespace hardware {
namespace oemlock {
namespace V1_0 {
namespace renesas {

using ::android::hardware::oemlock::V1_0::IOemLock;
using ::android::hardware::oemlock::V1_0::OemLockSecureStatus;
using ::android::hardware::oemlock::V1_0::OemLockStatus;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

class OemLock: public IOemLock {

public:
    OemLock();
    ~OemLock();

    // Methods from ::android::hardware::oemlock::V1_0::IOemLock follow.
    Return<void> getName(getName_cb _hidl_cb) override;

    Return<OemLockSecureStatus> setOemUnlockAllowedByCarrier(
            bool allowed, const hidl_vec<uint8_t>& signature) override;

    Return<void> isOemUnlockAllowedByCarrier(
            isOemUnlockAllowedByCarrier_cb _hidl_cb) override;

    Return<OemLockStatus> setOemUnlockAllowedByDevice(bool allowed) override;

    Return<void> isOemUnlockAllowedByDevice(
            isOemUnlockAllowedByDevice_cb _hidl_cb) override;

    OemLock(const OemLock& rhs) = delete;
    OemLock& operator=(const OemLock& rhs) = delete;

private:
    bool connect(void);
    void disconnect(void);
    bool invoke(uint32_t cmd, bool& allowed);

    bool m_is_init; ///< initialization flag
    std::mutex m_mutexCar; ///< carrier mutex
    std::mutex m_mutexDev; ///< device mutex
    TEEC_Context m_ctx; ///< TEE Context
    TEEC_Session m_sess; ///< TEE Session
};

} // namespace renesas
} // namespace V1_0
} // namespace oemlock
} // namespace hardware
} // namespace android

#endif /* OEMLOCK_HAL_H_ */
