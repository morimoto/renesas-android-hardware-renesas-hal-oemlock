/*
 *
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

#define LOG_TAG "OemLockHAL"

#include <utils/Log.h>
#include <cutils/properties.h>

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
    : m_is_init(connect()) {
    ALOGD("Create OemLock");
};

OemLock::~OemLock() {
    ALOGD("Destroy OemLock");
    if (m_is_init) {
        disconnect();
    }
};

/* Returns a vendor specific identifier of the HAL. */
Return<void> OemLock::getName(getName_cb _hidl_cb) {
    _hidl_cb(OemLockStatus::OK, {"Renesas OemLock HAL 1.0"});
    return Void();
};

/* Updates whether OEM unlock is allowed by the carrier. */
Return<OemLockSecureStatus> OemLock::setOemUnlockAllowedByCarrier(
        bool allowed, const hidl_vec<uint8_t>& signature) {
    if (!m_is_init) {
        ALOGE("OemLock TA is not connected");
        return OemLockSecureStatus::FAILED;
    }

    std::lock_guard<std::mutex> lock(m_mutexCar);

    // NOTE: This implementation does not require a signature
    if (signature.size()) {
        ALOGW("Signature provided but is not being used");
    }

    if (invoke(SET_OEM_UNLOCK_ALLOWED_BY_CARRIER, allowed)) {
        return OemLockSecureStatus::OK;
    } else {
        ALOGE("Failed to set OEM lock allowed by the carrier");
        return OemLockSecureStatus::FAILED;
    }
};

/* Returns whether OEM unlock is allowed by the carrier. */
Return<void> OemLock::isOemUnlockAllowedByCarrier(
        isOemUnlockAllowedByCarrier_cb _hidl_cb) {
    if (!m_is_init) {
        ALOGE("OemLock TA is not connected");
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }

    std::lock_guard<std::mutex> lock(m_mutexCar);
    bool allowed = false;

    if (invoke(GET_OEM_UNLOCK_ALLOWED_BY_CARRIER, allowed)) {
        _hidl_cb(OemLockStatus::OK, allowed);
    } else {
        ALOGE("Failed to get OEM lock allowed by the carrier");
        _hidl_cb(OemLockStatus::FAILED, false);
    }

    return Void();
};

/* Updates whether OEM unlock is allowed by the device. */
Return<OemLockStatus> OemLock::setOemUnlockAllowedByDevice(bool allowed) {
    if (!m_is_init) {
        ALOGE("OemLock TA is not connected");
        return OemLockStatus::FAILED;
    }

    std::lock_guard<std::mutex> lock(m_mutexDev);

    if (invoke(SET_OEM_UNLOCK_ALLOWED_BY_DEVICE, allowed)) {
        return OemLockStatus::OK;
    } else {
        ALOGE("Failed to set OEM lock allowed by the device");
        return OemLockStatus::FAILED;
    }
};

/* Returns whether OEM unlock ia allowed by the device. */
Return<void> OemLock::isOemUnlockAllowedByDevice(
        isOemUnlockAllowedByDevice_cb _hidl_cb) {
    if (!m_is_init) {
        ALOGE("OemLock TA is not connected");
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }

    std::lock_guard<std::mutex> lock(m_mutexDev);
    bool allowed = false;

    if (invoke(GET_OEM_UNLOCK_ALLOWED_BY_DEVICE, allowed)) {
        _hidl_cb(OemLockStatus::OK, allowed);
    } else {
        ALOGE("Failed to get OEM lock allowed by the device");
        _hidl_cb(OemLockStatus::FAILED, false);
    }

    return Void();
};

/*-----------------------------Private Implementation-------------------------*/

bool OemLock::connect(void)
{
    TEEC_Result res = TEEC_SUCCESS;
    TEEC_UUID uuid = TA_OEMLOCK_UUID;
    uint32_t err_origin = 0;

    res = TEEC_InitializeContext(NULL, &m_ctx);
    if (res != TEEC_SUCCESS) {
        ALOGE("TEEC_InitializeContext failed with code 0x%x", res);
        return false;
    }

    res = TEEC_OpenSession(&m_ctx, &m_sess, &uuid, TEEC_LOGIN_PUBLIC,
            NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS) {
        TEEC_FinalizeContext(&m_ctx);
        ALOGE("TEEC_Opensession failed with code 0x%x, origin 0x%x",
            res, err_origin);

        return false;
    }

    ALOGD("Connection with OemLock TA was established");
    return true;
}

void OemLock::disconnect()
{
    TEEC_CloseSession(&m_sess);
    TEEC_FinalizeContext(&m_ctx);
}

bool OemLock::invoke(uint32_t cmd, bool& allowed)
{
    TEEC_Operation op;
    memset(&op, 0, sizeof(op));

    op.paramTypes = TEEC_PARAM_TYPES
            (TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    op.params[0].value.a = static_cast<uint32_t>(allowed);

    uint32_t err_origin;
    TEEC_Result res = TEEC_InvokeCommand(&m_sess, cmd, &op, &err_origin);
    if (res != TEEC_SUCCESS) {
        ALOGE("TEEC_InvokeCommand '%u' failed with "
                "code 0x%x, origin 0x%x", cmd, res, err_origin);
        return false;
    }

    allowed = (op.params[0].value.a != 0);

    return true;
}

}  // namespace renesas
}  // namespace V1_0
}  // namespace oemlock
}  // namespace hardware
}  // namespace android
