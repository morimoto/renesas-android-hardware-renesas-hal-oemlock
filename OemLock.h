/*
 * Copyright (C) 2018 GlobalLogic
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
#include <fstream>
#include <mutex>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <android/hardware/oemlock/1.0/IOemLock.h>
#include <hidl/Status.h>

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
    size_t getPartitionSize(void);
    bool checkPartitionHash(void);
    bool setOemUnlockEnabledByCarrier(bool enabled);
    bool getOemUnlockEnabledByCarrier(void);
    bool setOemUnlockEnabledByDevice(bool enabled);
    bool getOemUnlockEnabledByDevice(void);

    bool m_is_init; ///< initialization flag
    std::string m_DataBlockFile; ///< persistent path
    size_t m_BlockDeviceSize; ///< persistent size
    std::mutex m_mutexCar; ///< carrier mutex
    std::mutex m_mutexDev; ///< device mutex

    const std::string PST_DATA_BLOCK_PROP = "ro.frp.pst";
    const std::string OEM_UNLOCK_PROP = "sys.oem_unlock_allowed";
    const uint32_t DIGEST_SIZE_BYTES = 32;
#if 0
    const uint32_t CARRIER_OFFSET = 2;
    const uint32_t DEVICE_OFFSET = 1;
#endif

    /* Hack: as we don't have access to FRP partition now,
     * use these bytes instead. Remove this in real implementation */
    uint8_t m_carrier_flag;
    uint8_t m_device_flag;
};

} // namespace renesas
} // namespace V1_0
} // namespace oemlock
} // namespace hardware
} // namespace android

#endif /* OEMLOCK_HAL_H_ */
