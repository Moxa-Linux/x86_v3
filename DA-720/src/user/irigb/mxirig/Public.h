/*++
    Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/
  
//
// The following value is arbitrarily chosen from the space defined 
// by Microsoft as being "for non-Microsoft use"
//
// NOTE: we use OSR's GUID_OSR_PLX_INTERFACE GUID value so that we 
// can use OSR's PLxTest program      :-)
//
#ifdef WIN32

DEFINE_GUID (GUID_MXIRIGB_INTERFACE, 
   0xcdc34ffb, 0x4967, 0x4124, 0x99, 0x2, 0x30, 0x32, 0x59, 0xbf, 0xa2, 0x3);

#define IOCTL_GET_REGISTER              CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_REGISTER              CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SETCLR_REGISTER_BIT       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_TIMESRC_STATUS        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#else

#include    <sys/types.h>
#include    <sys/stat.h>
#include    <fcntl.h>
#include    <unistd.h>
#include    <sys/ioctl.h>
#include    <string.h>
#include    <time.h>
#include    <errno.h>

#define     _POSIX_C_SOURCE 200809L
#include    <inttypes.h>
#include    <math.h>

#define MX_IRIGB_MINOR  111

#define IOCTL_GET_REGISTER          _IOR(MX_IRIGB_MINOR,0x800,unsigned long)
#define IOCTL_SET_REGISTER          _IOW(MX_IRIGB_MINOR,0x801,unsigned long)
#define IOCTL_SETCLR_REGISTER_BIT   _IOW(MX_IRIGB_MINOR,0x802,unsigned long)
#define IOCTL_GET_TIMESRC_STATUS    _IOW(MX_IRIGB_MINOR,0x803,unsigned long)

#define MAX_PAIRS 4

struct reg_val_pair_struct {
    unsigned long long count;
    unsigned long long addr[MAX_PAIRS];
    unsigned long long val[MAX_PAIRS];
};

struct reg_bit_pair_struct {
    unsigned long long addr;
    unsigned long long set_bit;
    unsigned long long clear_bit;
};

#endif

