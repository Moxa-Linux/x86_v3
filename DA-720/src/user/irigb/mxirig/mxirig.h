/*
  Copyright (C) MOXA Inc. All rights reserved.
  This software is distributed under the terms of the
  MOXA License.  See the file COPYING-MOXA for details.
*/

/**
 * @file mxirig.h : interface of the Moxa IRIGB Card.
 *
 * @version 1.1.0.0 - 2015/07/01
 *
 * @author holsety.chen@moxa.com
 */

#ifdef WIN32
    #ifdef MXIRIG_EXPORTS
    #define MXIRIG_API __declspec(dllexport)
    #else
    #define MXIRIG_API __declspec(dllimport)
    #endif
#else
    #include <unistd.h>
    #include <time.h>
    #include <errno.h>

    #define MXIRIG_API
    typedef int HANDLE;
    #ifdef BOOL 
    #undef BOOL
    #endif
    typedef int BOOL;
    #define FALSE   0
    #define TRUE    1
    typedef int * PBOOL;
    #ifdef DWORD
    #undef DWORD
    #endif
    typedef unsigned long DWORD;
    //typedef void * PDWORD;
    #ifdef PDWORD
    #undef PDWORD
    #endif
    typedef DWORD *PDWORD;
    #ifdef WORD
    #undef WORD
    #endif
    typedef unsigned short WORD;
    #ifdef UNINT32 
    #undef UNINT32
    #endif
    typedef unsigned int UNINT32;

    #define ShutdownMxDrv close
    #define SetLastError(ERROR_ACCESS_DENIED) (errno = -EPERM)
    #define GetLastError() errno
#endif

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

typedef struct _RTCTIME {
    int nanosec;        /* nano seconds after the second - [0,999999999] */
    int sec;            /* seconds after the minute - [0,60] */
    int min;            /* minutes after the hour - [0,59] */
    int hour;           /* hours since midnight - [0,23] */
    int mday;           /* day of the month - [1,31] */
    int mon;            /* months since January - [1,12] */
    int year;           /* years [00,99] */

    unsigned char lsp;  /* leap second pending at end of minute */
    unsigned char ls;   /* leap second type, 0=+, 1=- */
    unsigned char dsp;  /* daylight saving time change pending at end of minute */
    unsigned char dst;  /* daylight saving time in effect */
    unsigned char tzs;  /* Time zone offset sign (0=+) */
    unsigned char tzh;  /* Time zone offset (0??5) */
    unsigned char tz;   /* Time zone offset (0??5) */
    unsigned char tq;   /* Time quality (Binary, 0??5) */
} RTCTIME, *PRTCTIME;

enum _RTC_SYNC_SOURCE_
{
    TIMESRC_FREERUN = 0,
    TIMESRC_FIBER,
    TIMESRC_PORT1,

    TIMESRC_UNKNOWN
};

enum _IRIG_SIGNAL_STATUS_
{
    IRIG_STATUS_NORMAL = 0,
    IRIG_STATUS_OFF_LINE,
    IRIG_STATUS_FRAME_ERROR,
    IRIG_STATUS_PARITY_ERROR,

    IRIG_STATUS_UNKNOWN
};

enum _PORT_LIST_
{
    PORT_FIBER = 0,
    PORT_1,
    PORT_2,
    PORT_3,
    PORT_4,

    PORT_UNKNOWN
};

enum _SIGNAL_TYPE_
{
    TYPE_TTL = 0,
    TYPE_DIFFERENTIAL,

    TYPE_UNKNOWN
};

enum _OUTPUT_MODE_
{
    MODE_FROM_FIBER_IN = 0,
    MODE_FROM_PORT1_IN,
    MODE_IRIGB,
    MODE_PPS,

    MODE_UNKNOWN
};

enum _PARITY_CHECK_MODE_
{
    PARITY_CHECK_EVEN,
    PARITY_CHECK_ODD,
    PARITY_CHECK_NONE
};

enum _IRIGB_BOARD_HWID_
{
    DA_IRIGB_4DIO_PCI104 = 1,
    DE2_IRIGB_4DIO = 2,
    DA_IRIGB_S = 7,
    MAX_BOARD_HWID = 8
};

/**
 * Get Irigb board hardware ID
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pdwHwId - A pointer to get hardware ID, the value is one of _IRIGB_BOARD_HWID_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetHardwareID(HANDLE hDev, PDWORD pdwHwId);

/**
 * Open Irigb device
 * @param  [in] index - the device number (started from 0)
 * @return Pointer to device handle. Return -1 on failure.
 */
MXIRIG_API HANDLE mxIrigbOpen(int index);

/**
 * Close Irigb device
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @return None
 */
MXIRIG_API void mxIrigbClose(HANDLE hDev);

/**
 * Get internal RTC time from Irigb device
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pRtcTime - A pointer to a RTCTIME structure to receive the current date and time.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetTime(HANDLE hDev, PRTCTIME pRtcTime);

/**
 * Set internal RTC time to Irigb device
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] pRtcTime - A pointer to a RTCTIME structure that contains the new date and time.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetTime(HANDLE hDev, PRTCTIME pRtcTime);

/**
 * Synchronize local time with internal RTC
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] pRtbToFrom - 0: Set internal RTC to local time, 1: Set local time to internal RTC
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSyncTime(HANDLE hDev, BOOL bToFrom);

/**
 * Set internal RTC synchronization source
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwSource - internal RTC synchronization source, the value is one of _RTC_SYNC_SOURCE_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetSyncTimeSrc(HANDLE hDev, DWORD dwSource);

/**
 * Get internal RTC synchronization source
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pdwSource - A point to get internal RTC synchronization source, the value is one of _RTC_SYNC_SOURCE_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetSyncTimeSrc(HANDLE hDev, PDWORD pdwSource);

/**
 * Get IRIGB signal status
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwSource - IRIGB signal source, the value is one of _PORT_LIST_.
 * @param  [out] dwStatus - A point to get IRIGB signal status, the value is one of _IRIG_SIGNAL_STATUS_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetSignalStatus(HANDLE hDev, DWORD dwSource, PDWORD pdwStatus);

/**
 * Set IRIGB input Parity check mode
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwSource - the value is one of _RTC_SYNC_SOURCE_, should not be TIMESRC_FREERUN.
 * @param  [in] dwMode - the value is one of _PARITY_CHECK_MODE_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetInputParityCheckMode(HANDLE hDev, DWORD dwSource, DWORD dwMode);

/**
 * Get IRIGB input Parity check mode
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwSource - the value is one of _RTC_SYNC_SOURCE_, should not be TIMESRC_FREERUN.
 * @param  [out] pdwMode - A point to get output parity check mode, the value is one of _PARITY_CHECK_MODE_.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetInputParityCheckMode(HANDLE hDev, DWORD dwSource, PDWORD pdwMode);

/**
 * Set IRIGB output Parity check mode
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwMode - the value is one of _PARITY_CHECK_MODE_, should not be PARITY_CHECK_NONE.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetOutputParityCheckMode(HANDLE hDev, DWORD dwMode);

/**
 * Get IRIGB output Parity check mode
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pdwMode - A point to get output parity check mode.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetOutputParityCheckMode(HANDLE hDev, PDWORD pdwMode);

/**
 * Set Pulse Per Second output width
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwMilliSecond - The pulse width per millisecond.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetPpsWidth(HANDLE hDev, DWORD dwMilliSecond);

/**
 * Get Pulse Per Second output width
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pdwMilliSecond - A point to get the pulse width per millisecond value.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetPpsWidth(HANDLE hDev, PDWORD pdwMilliSecond);

/**
 * Set input signal type
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - Signal source, the value is one of _PORT_LIST_.
 * @param  [in] dwType - Signal type, the value is one of _SIGNAL_TYPE_.
 * @param  [in] invert - Signal inverse mode, if nonzero, invert the input signal.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetInputSignalType(HANDLE hDev, DWORD dwPort, DWORD dwType, BOOL invert);

/**
 * Get input signal type
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - Signal source, the value is one of _PORT_LIST_.
 * @param  [out] pdwType - A point to get IRIGB signal type, the value is one of _SIGNAL_TYPE_.
 * @param  [out] pbInvert - A point to get signal mode, if nonzero, the signal is inverse.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetInputSignalType(HANDLE hDev, DWORD dwPort, PDWORD pdwType, PBOOL pbInvert);

/**
 * Set output signal type
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - Signal source, the value is one of _PORT_LIST_.
 * @param  [in] dwType - Signal type, the value is one of _SIGNAL_TYPE_.
 * @param  [in] dwMode - Signal output mode, the value is one of _OUTPUT_MODE_.
 * @param  [in] invert - Signal inverse mode, if nonzero, invert the output signal.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetOutputSignalType(HANDLE hDev, DWORD dwPort, DWORD dwType, DWORD dwMode, BOOL invert);

/**
 * Get output signal type
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - Signal source, the value is one of _PORT_LIST_.
 * @param  [out] pdwType - A point to get signal type, the value is one of _SIGNAL_TYPE_.
 * @param  [out] pdwMode - A point to get signal output mode, the value is one of _OUTPUT_MODE_.
 * @param  [out] pbInvert - A point to get signal mode, if nonzero, the signal is inverse.
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetOutputSignalType(HANDLE hDev, DWORD dwPort, PDWORD pdwType, PDWORD pdwMode, PBOOL pbInvert);

/**
 * Set digital output signal
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - the port number (started from 0)
 * @param  [in] value - the port data, 1:HIGH, 0:LOW
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbSetDigitalOutputSignal(HANDLE hDev, DWORD dwPort, DWORD value);

/**
 * Get digital output signal
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - the port number (started from 0)
 * @param  [out] pValue - A point to get port data, 1:HIGH, 0:LOW
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetDigitalOutputSignal(HANDLE hDev, DWORD dwPort, PDWORD pValue);

/**
 * Get digital input signal
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [in] dwPort - the port number (started from 0)
 * @param  [out] pValue - A point to get port data, 1:HIGH, 0:LOW
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetDigitalInputSignal(HANDLE hDev, DWORD dwPort, PDWORD pValue);

/**
 * Get FPGA firmware build date
 * @param  [in] hDev - A valid handle value return from "mxIrigbOpen" function.
 * @param  [out] pValue - A point to get FPGA firmware build date, the value is BCD style yyyyMMdd, eg.:0x20140101
 * @return - If the operation completes successfully, the return value is nonzero.
 *           If the operation fails or is pending, the return value is zero. 
 *           To get extended error information, call GetLastError.
 */
MXIRIG_API BOOL mxIrigbGetFpgaBuildDate(HANDLE hDev, PDWORD pValue);

#ifdef __cplusplus    // If used by C++ code, 
}
#endif

