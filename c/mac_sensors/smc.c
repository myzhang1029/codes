/*
 * Apple System Management Control (SMC) Tool
 * Copyright (C) 2006 devnull
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 USA.
 */

#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <string.h>

#include "smc.h"

static io_connect_t conn;

UInt32 _strtoul(char *str, int size, int base)
{
    UInt32 total = 0;
    int i;

    for (i = 0; i < size; i++)
    {
        if (base == 16)
            total += str[i] << (size - 1 - i) * 8;
        else
            total += (unsigned char)(str[i] << (size - 1 - i) * 8);
    }
    return total;
}

float _flttof(SMCBytes_t str)
{
    fltUnion flt;
    flt.b[0] = str[0];
    flt.b[1] = str[1];
    flt.b[2] = str[2];
    flt.b[3] = str[3];

    return flt.f;
}

void _ultostr(char *str, UInt32 val)
{
    str[0] = '\0';
    sprintf(str, "%c%c%c%c", (unsigned int)val >> 24, (unsigned int)val >> 16,
            (unsigned int)val >> 8, (unsigned int)val);
}

kern_return_t SMCOpen(void)
{
    kern_return_t result;
    mach_port_t masterPort;
    io_iterator_t iterator;
    io_object_t device;

    result = IOMasterPort(MACH_PORT_NULL, &masterPort);

    CFMutableDictionaryRef matchingDictionary = IOServiceMatching("AppleSMC");
    result =
        IOServiceGetMatchingServices(masterPort, matchingDictionary, &iterator);
    if (result != kIOReturnSuccess)
    {
        printf("Error: IOServiceGetMatchingServices() = %08x\n", result);
        return 1;
    }

    device = IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    if (device == 0)
    {
        printf("Error: no SMC found\n");
        return 1;
    }

    result = IOServiceOpen(device, mach_task_self(), 0, &conn);
    IOObjectRelease(device);
    if (result != kIOReturnSuccess)
    {
        printf("Error: IOServiceOpen() = %08x\n", result);
        return 1;
    }

    return kIOReturnSuccess;
}

kern_return_t SMCClose() { return IOServiceClose(conn); }

kern_return_t SMCCall(int index, SMCKeyData_t *inputStructure,
                      SMCKeyData_t *outputStructure)
{
    size_t structureInputSize;
    size_t structureOutputSize;

    structureInputSize = sizeof(SMCKeyData_t);
    structureOutputSize = sizeof(SMCKeyData_t);

#if MAC_OS_X_VERSION_10_5
    return IOConnectCallStructMethod(conn, index,
                                     // inputStructure
                                     inputStructure, structureInputSize,
                                     // ouputStructure
                                     outputStructure, &structureOutputSize);
#else
    return IOConnectMethodStructureIStructureO(
        conn, index, structureInputSize, /* structureInputSize */
        &structureOutputSize,            /* structureOutputSize */
        inputStructure,                  /* inputStructure */
        outputStructure);                /* ouputStructure */
#endif
}

kern_return_t SMCReadKey(UInt32Char_t key, SMCVal_t *val)
{
    kern_return_t result;
    SMCKeyData_t inputStructure;
    SMCKeyData_t outputStructure;

    memset(&inputStructure, 0, sizeof(SMCKeyData_t));
    memset(&outputStructure, 0, sizeof(SMCKeyData_t));
    memset(val, 0, sizeof(SMCVal_t));

    inputStructure.key = _strtoul(key, 4, 16);
    inputStructure.data8 = SMC_CMD_READ_KEYINFO;

    result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
        return result;

    val->dataSize = outputStructure.keyInfo.dataSize;
    _ultostr(val->dataType, outputStructure.keyInfo.dataType);
    inputStructure.keyInfo.dataSize = val->dataSize;
    inputStructure.data8 = SMC_CMD_READ_BYTES;

    result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
        return result;

    memcpy(val->bytes, outputStructure.bytes, sizeof(outputStructure.bytes));

    return kIOReturnSuccess;
}

float SMCGetTemperature(UInt32Char_t key)
{
    SMCVal_t val;
    kern_return_t result;

    result = SMCReadKey(key, &val);
    if (result == kIOReturnSuccess)
    {
        // read succeeded - check returned value
        if (val.dataSize > 0)
        {
            if (strcmp(val.dataType, DATATYPE_SP78) == 0)
            {
                // convert sp78 value to temperature
                int intValue = (val.bytes[0] * 256 + val.bytes[1]) >> 2;
                return intValue / 64.0;
            }
        }
    }
    // read failed
    return 0.0;
}

float SMCGetFanSpeed(UInt32Char_t key)
{
    SMCVal_t val;
    kern_return_t result;

    result = SMCReadKey(key, &val);
    if (result == kIOReturnSuccess)
    {
        // read succeeded - check returned value
        if (val.dataSize > 0)
        {
            if (strcmp(val.dataType, DATATYPE_FPE2) == 0)
            {
                // convert fpe2 value to rpm
                int intValue = (unsigned char)val.bytes[0] * 256 +
                               (unsigned char)val.bytes[1];
                return intValue / 4.0;
            }
            else if (strcmp(val.dataType, DATATYPE_FLT) == 0)
            {
                // 2018 models have the flt type for this key
                return _flttof(val.bytes);
            }
        }
    }
    // read failed
    return 0.0;
}

float SMCGetVoltageCurrent(UInt32Char_t key)
{
    SMCVal_t val;
    kern_return_t result;

    result = SMCReadKey(key, &val);
    if (result == kIOReturnSuccess)
    {
        // read succeeded - check returned value
        if (val.dataSize > 0)
        {
            if (strcmp(val.dataType, DATATYPE_FLT) == 0)
                return _flttof(val.bytes);
        }
    }
    // read failed
    return 0.0;
}

int SMCGetFanNumber(UInt32Char_t key)
{
    SMCVal_t val;
    kern_return_t result;

    result = SMCReadKey(key, &val);
    return _strtoul((char *)val.bytes, val.dataSize, 10);
}

