/*
 *
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

#define LOG_TAG "OemLockHAL"

#include <string.h>
#include <vector>
#include <memory>
#include <stdlib.h>

#include <utils/Log.h>
#include <cutils/properties.h>
#include <openssl/sha.h>

#include "OemLock.h"

namespace android {
namespace hardware {
namespace oemlock {
namespace V1_0 {
namespace renesas {

/**
 * HAL for managing the OEM lock state of the device.
 */
OemLock::OemLock()
    : m_is_init(false), m_BlockDeviceSize(0),
      m_carrier_flag(1), m_device_flag(255) {
    ALOGD("Create OemLock");
#if 0
    char value[PROPERTY_VALUE_MAX] = {0,};
    if (property_get(PST_DATA_BLOCK_PROP.c_str(), value, "") > 0) {
        m_DataBlockFile = value;
        m_is_init = true;
        ALOGD("Persistent partition is '%s'", m_DataBlockFile.c_str());
    } else {
        ALOGE("Error get property '%s'", PST_DATA_BLOCK_PROP.c_str());
    }
    if (m_is_init) {
        m_BlockDeviceSize = getPartitionSize();
        if (m_BlockDeviceSize != 0) {
            m_is_init = true;
            ALOGD("Persistent partition size is '%ld'", m_BlockDeviceSize);
        } else {
            m_is_init = false;
            ALOGE("Error get partition size");
        }
    }
    if (m_is_init) {
        if (checkPartitionHash() == false) {
            /* Set default value */
            setOemUnlockEnabledByDevice(false);
        }
        /* Always enabled (TODO: need further improvement) */
        setOemUnlockEnabledByCarrier(true);
    }
#else
    m_is_init = true;
#endif
};

OemLock::~OemLock() {
    ALOGD("Destroy OemLock");
};

/* Returns a vendor specific identifier of the HAL. */
Return<void> OemLock::getName(getName_cb _hidl_cb) {
    _hidl_cb(OemLockStatus::OK, {"Dummy OemLock HAL 1.0"});
    /*_hidl_cb(OemLockStatus::OK, {"Renesas OemLock HAL 1.0"});*/
    return Void();
};

/* Updates whether OEM unlock is allowed by the carrier. */
Return<OemLockSecureStatus> OemLock::setOemUnlockAllowedByCarrier(
        bool allowed, const hidl_vec<uint8_t>& signature) {
    if (!m_is_init) {
        return OemLockSecureStatus::FAILED;
    }
    // NOTE: This implementation does not require a signature
    if (signature.size()) {
        ALOGW("Signature provided but is not being used");
    }
    auto ret = setOemUnlockEnabledByCarrier(allowed);
    if (ret) {
        return OemLockSecureStatus::OK;
    }
    else {
        return OemLockSecureStatus::FAILED;
    }
};

/* Returns whether OEM unlock is allowed by the carrier. */
Return<void> OemLock::isOemUnlockAllowedByCarrier(
        isOemUnlockAllowedByCarrier_cb _hidl_cb) {
    if (!m_is_init) {
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }
    _hidl_cb(OemLockStatus::OK, getOemUnlockEnabledByCarrier());
    return Void();
};

/* Updates whether OEM unlock is allowed by the device. */
Return<OemLockStatus> OemLock::setOemUnlockAllowedByDevice(bool allowed) {
    if (!m_is_init) {
        return OemLockStatus::FAILED;
    }
    auto ret = setOemUnlockEnabledByDevice(allowed);
    if (ret) {
        return OemLockStatus::OK;
    }
    else {
        return OemLockStatus::FAILED;
    }
};

/* Returns whether OEM unlock ia allowed by the device. */
Return<void> OemLock::isOemUnlockAllowedByDevice(
        isOemUnlockAllowedByDevice_cb _hidl_cb) {
    if (!m_is_init) {
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }
    _hidl_cb(OemLockStatus::OK, getOemUnlockEnabledByDevice());
    return Void();
};

size_t OemLock::getPartitionSize(void)
{
    auto fd = open(m_DataBlockFile.c_str(), O_RDONLY);
    if (fd < 0) {
        ALOGE("Error open '%s'", m_DataBlockFile.c_str());
        return 0;
    }
    uint64_t size = 0;
    auto ret = ioctl(fd, BLKGETSIZE64, &size);
    if (ret < 0) {
        ALOGE("Error ioctl '%s'", m_DataBlockFile.c_str());
        size = 0;
    }
    close(fd);
    return size;
};

bool OemLock::checkPartitionHash(void) {
    std::ifstream inputStream(m_DataBlockFile,
            std::ifstream::in | std::ifstream::binary);
    if (!inputStream.is_open()) {
        ALOGE("Error open persistent partition");
        return false;
    }
    SHA256_CTX sha256_ctx;
    if (SHA256_Init(&sha256_ctx) != 1) {
        ALOGE("Error init SHA-256");
        return false;
    }

    std::unique_ptr<uint8_t[]> storedHash(new uint8_t[DIGEST_SIZE_BYTES]);
    inputStream.seekg(0, std::ios::beg);
    inputStream.read(reinterpret_cast<char*>(storedHash.get()),
            DIGEST_SIZE_BYTES);

    std::unique_ptr<uint8_t[]> data(new uint8_t[m_BlockDeviceSize]);
    memset(data.get(), 0, DIGEST_SIZE_BYTES);
    /* Include 0 checksum in digest */
    SHA256_Update(&sha256_ctx, data.get(), DIGEST_SIZE_BYTES);

    inputStream.read(reinterpret_cast<char*>(data.get()),
            m_BlockDeviceSize - DIGEST_SIZE_BYTES);
    size_t bytes_read = inputStream.gcount();
    if (bytes_read != m_BlockDeviceSize - DIGEST_SIZE_BYTES) {
        ALOGE("Error read partition data");
        return false;
    } else {
        /* All read successfully */
        SHA256_Update(&sha256_ctx, data.get(),
                m_BlockDeviceSize - DIGEST_SIZE_BYTES);
    }

    std::unique_ptr<uint8_t[]> computedHash(new uint8_t[DIGEST_SIZE_BYTES]);
    SHA256_Final(computedHash.get(), &sha256_ctx);

    if (memcmp(storedHash.get(), computedHash.get(), DIGEST_SIZE_BYTES)) {
        ALOGW("Persistent partition hash mismatch!");
        return false;
    }
    return true;
};

bool OemLock::getOemUnlockEnabledByCarrier(void) {

    std::lock_guard<std::mutex> lock(m_mutexCar);
#if 0
    std::ifstream inputStream(m_DataBlockFile,
            std::ifstream::in | std::ifstream::binary);
    if (!inputStream.is_open()) {
        ALOGE("Error open persistent partition");
        return false;
    }

    char is_enable = 0;
    inputStream.seekg(m_BlockDeviceSize - CARRIER_OFFSET, std::ios::beg);
    inputStream.read(&is_enable, sizeof(is_enable));

    return (is_enable != 0);
#else
    return (m_carrier_flag != 0);
#endif
};

bool OemLock::getOemUnlockEnabledByDevice(void) {

    std::lock_guard<std::mutex> lock(m_mutexDev);
#if 0
    std::ifstream inputStream(m_DataBlockFile,
            std::ifstream::in | std::ifstream::binary);
    if (!inputStream.is_open()) {
        ALOGE("Error open persistent partition");
        return false;
    }

    char is_enable = 0;
    inputStream.seekg(m_BlockDeviceSize - DEVICE_OFFSET, std::ios::beg);
    inputStream.read(&is_enable, sizeof(is_enable));

    return (is_enable != 0);
#else
    if (m_device_flag == 255) {
        /*Read once:*/
        char value[PROPERTY_VALUE_MAX] = {0,};
        if (property_get(OEM_UNLOCK_PROP.c_str(), value, "") > 0) {
            m_device_flag = atoi(value);
            ALOGD("'%s' is '%s' [%d]", OEM_UNLOCK_PROP.c_str(), value, m_device_flag);
        } else {
            m_device_flag = 0;
            ALOGE("Error get property '%s'", OEM_UNLOCK_PROP.c_str());
        }
    }
    return (m_device_flag != 0);
#endif
};

bool OemLock::setOemUnlockEnabledByCarrier(bool enabled) {

    std::lock_guard<std::mutex> lock(m_mutexCar);
#if 0
    std::ofstream outputStream(m_DataBlockFile,
            std::ifstream::out | std::ifstream::binary);
    if (!outputStream.is_open()) {
        ALOGE("Error open persistent partition");
        return false;
    }

    char lock_byte = enabled ? 1 : 0;
    outputStream.seekp(m_BlockDeviceSize - CARRIER_OFFSET, std::ios::beg);
    outputStream.write(&lock_byte,sizeof(lock_byte));
    outputStream.flush();
#else
    m_carrier_flag = enabled ? 1 : 0;
#endif
    return true;
};

bool OemLock::setOemUnlockEnabledByDevice(bool enabled) {

    std::lock_guard<std::mutex> lock(m_mutexDev);
#if 0
    std::ofstream outputStream(m_DataBlockFile,
            std::ifstream::out | std::ifstream::binary);
    if (!outputStream.is_open()) {
        ALOGE("Error open persistent partition");
        return false;
    }

    char lock_byte = enabled ? 1 : 0;
    outputStream.seekp(m_BlockDeviceSize - DEVICE_OFFSET, std::ios::beg);
    outputStream.write(&lock_byte, sizeof(lock_byte));
    outputStream.flush();
#else
    m_device_flag = enabled ? 1 : 0;
#endif
    return true;
};

}  // namespace renesas
}  // namespace V1_0
}  // namespace oemlock
}  // namespace hardware
}  // namespace android
