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

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <utee_defines.h>

#include "oemlock_ta.h"

/*
 * Trusted Application Entry Points
 */

/*
 * OemLock storage format:
 * [Version, 1 byte, must be 0x01 | Carrier lock bit, 1 byte, default 1 |
 * Device lock bit, 1 byte, default 1 | Padding, 1 byte, always 0 | Hash SHA-1]
 */

/*
 * OemLock storage object ID
 */
static uint8_t OemLock_ID[] = { 0x96, 0xdb, 0x05, 0x9e };

#define OEMLOCK_STORAGE_SIZE        24
#define OEMLOCK_DATA_SIZE           4
#define OEMLOCK_HASH_SIZE           20

#define OEMLOCK_VERSION_OFFSET      0
#define OEMLOCK_CARRIER_OFFSET      1
#define OEMLOCK_DEVICE_OFFSET       2

TEE_Result readOemLockStorage(uint8_t* carrier, uint8_t* device);
TEE_Result writeOemLockStorage(uint8_t* carrier, uint8_t* device);

TEE_Result readOemLockStorage(uint8_t* carrier, uint8_t* device)
{
    TEE_Result res = TEE_SUCCESS;
    TEE_OperationHandle op = TEE_HANDLE_NULL;
    TEE_ObjectHandle obj = TEE_HANDLE_NULL;

    uint8_t oemLock_storage[OEMLOCK_STORAGE_SIZE];
    uint32_t sha1_size = OEMLOCK_HASH_SIZE;
    uint8_t sha1[OEMLOCK_HASH_SIZE];
    uint32_t actual_read = 0;

    TEE_MemFill(oemLock_storage, 0x00, OEMLOCK_STORAGE_SIZE);
    TEE_MemFill(sha1, 0x00, OEMLOCK_HASH_SIZE);

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, OemLock_ID,
    sizeof(OemLock_ID), TEE_DATA_FLAG_ACCESS_READ, &obj);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to open OemLock storage object, res=%x\n", res);
        goto err_3;
    }

    res = TEE_ReadObjectData(obj, (void *)oemLock_storage, OEMLOCK_STORAGE_SIZE,
            &actual_read);
    if ((res != TEE_SUCCESS) || (OEMLOCK_STORAGE_SIZE != actual_read)) {
        EMSG("Failed to read OemLock storage object, res=%x, read=%d\n", res,
                actual_read);
        goto err_2;
    }

    res = TEE_AllocateOperation(&op, TEE_ALG_SHA1, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate SHA-1 operation, res=%x\n", res);
        goto err_2;
    }

    res = TEE_DigestDoFinal(op, oemLock_storage, OEMLOCK_DATA_SIZE, sha1,
            &sha1_size);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to calculate OemLock storage object SHA-1, res=%x\n", res);
        goto err_1;
    }

    if (oemLock_storage[OEMLOCK_VERSION_OFFSET] != 0x01) {
        res = TEE_ERROR_BAD_STATE;
        EMSG("Version error OemLock storage object\n");
        goto err_1;
    }

    if (TEE_MemCompare(sha1, &oemLock_storage[OEMLOCK_DATA_SIZE],
            OEMLOCK_HASH_SIZE) != 0) {
        res = TEE_ERROR_BAD_STATE;
        EMSG("SHA-1 error OemLock storage object\n");
        goto err_1;
    }

    if (carrier){
        *carrier = oemLock_storage[OEMLOCK_CARRIER_OFFSET];
    }
    if (device){
        *device = oemLock_storage[OEMLOCK_DEVICE_OFFSET];
    }

err_1:
    TEE_FreeOperation(op);
err_2:
    TEE_CloseObject(obj);
err_3:
    return res;
}

TEE_Result writeOemLockStorage(uint8_t* carrier, uint8_t* device)
{
    TEE_Result res = TEE_SUCCESS;
    TEE_OperationHandle op = TEE_HANDLE_NULL;
    TEE_ObjectHandle obj = TEE_HANDLE_NULL;

    uint8_t oemLock_storage[OEMLOCK_STORAGE_SIZE];
    uint32_t sha1_size = OEMLOCK_HASH_SIZE;
    uint8_t sha1[OEMLOCK_HASH_SIZE];
    uint32_t actual_read = 0;

    TEE_MemFill(oemLock_storage, 0x00, OEMLOCK_STORAGE_SIZE);
    TEE_MemFill(sha1, 0x00, OEMLOCK_HASH_SIZE);

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, OemLock_ID,
    sizeof(OemLock_ID), TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE,
            &obj);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to open OemLock storage object, res=%x\n", res);
        goto err_3;
    }

    res = TEE_ReadObjectData(obj, (void *)oemLock_storage, OEMLOCK_DATA_SIZE,
            &actual_read);
    if (((res != TEE_SUCCESS) || (OEMLOCK_DATA_SIZE != actual_read))
            && (0 != actual_read)) { /*During first creation object size is 0*/
        EMSG("Failed to read OemLock storage object, res=%x, read=%d\n", res,
                actual_read);
        goto err_2;
    }

    res = TEE_AllocateOperation(&op, TEE_ALG_SHA1, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate SHA-1 operation, res=%x\n", res);
        goto err_2;
    }

    oemLock_storage[OEMLOCK_VERSION_OFFSET] = 0x01;

    if (carrier) {
        oemLock_storage[OEMLOCK_CARRIER_OFFSET] = *carrier;
    }
    if (device) {
        oemLock_storage[OEMLOCK_DEVICE_OFFSET] = *device;
    }

    res = TEE_DigestDoFinal(op, oemLock_storage, OEMLOCK_DATA_SIZE, sha1,
            &sha1_size);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to calculate OemLock storage object SHA-1, res=%x\n", res);
        goto err_1;
    }

    TEE_MemMove(&oemLock_storage[OEMLOCK_DATA_SIZE], sha1, OEMLOCK_HASH_SIZE);

    res = TEE_SeekObjectData(obj, 0, TEE_DATA_SEEK_SET);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to seek OemLock storage object, res=%x\n", res);
        goto err_1;
    }

    res = TEE_WriteObjectData(obj, (void *)oemLock_storage, OEMLOCK_STORAGE_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to write OemLock storage object, res=%x\n", res);
        goto err_1;
    }

err_1:
    TEE_FreeOperation(op);
err_2:
    TEE_CloseObject(obj);
err_3:
    return res;
}

/* Called each time a new instance is created */
TEE_Result TA_CreateEntryPoint(void)
{
    TEE_Result res = TEE_SUCCESS;
    TEE_ObjectHandle obj = TEE_HANDLE_NULL;
    uint8_t carrier = 1; /*default value*/
    uint8_t device = 1; /*default value*/

    /*Check OemLock storage object presence*/
    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, OemLock_ID,
        sizeof(OemLock_ID), TEE_DATA_FLAG_ACCESS_READ, &obj);
    if (res == TEE_ERROR_ITEM_NOT_FOUND) {
        res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE, OemLock_ID,
                sizeof(OemLock_ID), TEE_DATA_FLAG_ACCESS_WRITE,
                TEE_HANDLE_NULL, NULL, 0, &obj);
        if (res != TEE_SUCCESS) {
            EMSG("Failed to create OemLock storage object, res=%x\n", res);
        } else {
            TEE_CloseObject(obj);
            res = writeOemLockStorage(&carrier, &device);
            if (res != TEE_SUCCESS) {
                EMSG("Failed to write OemLock storage object, res=%x\n", res);
            }
        }
    } else if (res == TEE_SUCCESS) {
        /*OemLock storage object is already created*/
        TEE_CloseObject(obj);
    } else {
        EMSG("Failed to open OemLock storage object, res=%x\n", res);
    }

    return res;
}

/* Called each time an instance is destroyed */
void TA_DestroyEntryPoint(void)
{
}

/* Called each time a session is opened */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
        TEE_Param params[TEE_NUM_PARAMS], void **sess_ctx)
{
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
                           TEE_PARAM_TYPE_NONE,
                           TEE_PARAM_TYPE_NONE,
                           TEE_PARAM_TYPE_NONE);
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    /* Unused parameters */
    (void)&params;
    (void)&sess_ctx;

    return TEE_SUCCESS;
}

/* Called each time a session is closed */
void TA_CloseSessionEntryPoint(void *sess_ctx)
{
    /* Unused parameters */
    (void)&sess_ctx;
}

/* Called when a command is invoked */
TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx, uint32_t cmd_id,
            uint32_t param_types, TEE_Param params[TEE_NUM_PARAMS])
{
    TEE_Result res = TEE_SUCCESS;
    uint8_t carrier = 0;
    uint8_t device = 0;

    if (TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE) != param_types) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    switch (cmd_id) {
    case GET_OEM_UNLOCK_ALLOWED_BY_CARRIER:
        res = readOemLockStorage(&carrier, NULL);
        if (res == TEE_SUCCESS) {
            params[0].value.a = carrier;
        } else {
            EMSG("Failed to get OEM lock allowed by the carrier, res=%x\n", res);
        }
        break;
    case SET_OEM_UNLOCK_ALLOWED_BY_CARRIER:
        carrier = params[0].value.a;
        res = writeOemLockStorage(&carrier, NULL);
        if (res != TEE_SUCCESS) {
            EMSG("Failed to set OEM lock allowed by the carrier, res=%x\n", res);
        }
        break;
    case GET_OEM_UNLOCK_ALLOWED_BY_DEVICE:
        res = readOemLockStorage(NULL, &device);
        if (res == TEE_SUCCESS) {
            params[0].value.a = device;
        } else {
            EMSG("Failed to get OEM lock allowed by the device, res=%x\n", res);
        }
        break;
    case SET_OEM_UNLOCK_ALLOWED_BY_DEVICE:
        device = params[0].value.a;
        res = writeOemLockStorage(NULL, &device);
        if (res != TEE_SUCCESS) {
            EMSG("Failed to set OEM lock allowed by the device, res=%x\n", res);
        }
        break;

    default:
        return TEE_ERROR_BAD_PARAMETERS;
    }

    /* Unused parameter */
    (void)&sess_ctx;

    return res;
}
