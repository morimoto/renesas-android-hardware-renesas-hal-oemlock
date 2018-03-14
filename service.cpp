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

#define LOG_TAG "OemLockHAL"

#include <new>
#include <utils/Log.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/LegacySupport.h>
#include <android-base/logging.h>

#include "OemLock.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::oemlock::V1_0::IOemLock;
using android::hardware::oemlock::V1_0::renesas::OemLock;

const uint32_t MAX_THREADS = 1;

int main(int /* argc */, char * /* argv */ []) {

    ALOGI("Loading OemLock HAL...");
    android::sp<IOemLock> oemlock = new (std::nothrow) OemLock();
    if (oemlock == nullptr) {
        ALOGE("Could not allocate OemLock");
        return 1;
    }

    configureRpcThreadpool(MAX_THREADS, true);

    CHECK_EQ(oemlock->registerAsService(), android::NO_ERROR)
        << "Failed to register OemLock HAL";

    joinRpcThreadpool();

    ALOGI("OemLock HAL is terminating...");
    return 0;
}
