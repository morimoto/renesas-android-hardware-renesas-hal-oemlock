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

#ifndef OEMLOCK_TA_H
#define OEMLOCK_TA_H

/*
 * Please keep this define consistent with TA_UUID variable that defined
 * in Android.mk file
 */
#define TA_OEMLOCK_UUID { 0xbe1e65f4, 0x40ca, 0x11e9, \
    { 0xb2, 0x10, 0xd6, 0x63, 0xbd, 0x87, 0x3d, 0x93 } }

/*
 * OemLock commands
 */
typedef enum {
    GET_OEM_UNLOCK_ALLOWED_BY_CARRIER = 0,
    SET_OEM_UNLOCK_ALLOWED_BY_CARRIER,
    GET_OEM_UNLOCK_ALLOWED_BY_DEVICE,
    SET_OEM_UNLOCK_ALLOWED_BY_DEVICE,
} oemlock_command_t;

#endif /* OEMLOCK_TA_H */
