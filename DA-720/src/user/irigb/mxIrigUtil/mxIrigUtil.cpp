/*
  Copyright (C) MOXA Inc. All rights reserved.
  This software is distributed under the terms of the
  MOXA License.  See the file COPYING-MOXA for details.
*/

/**
 * @file mxIrigUtil.cpp : Utility for Moxa IRIGB Card.
 *
 * @version 1.1.0.0 - 2015/07/01
 *
 * @author holsety.chen@moxa.com
 */

#ifdef WIN32
#include <Windows.h>
#include <tchar.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../mxirig/mxirig.h"

#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "..\\Debug\\mxirig.lib")
#else
#pragma comment(lib, "..\\Release\\mxirig.lib")
#endif
#endif

const char *strTimeSrc[] = {
	"Free Run(Internal RTC)",
	"Port 0/Fiber In",
	"Port 1 In"
};

const char *strSignalStatus[] = {
	"Normal",
	"Off Line",
	"Frame Error",
	"Parity Error"
};

const char *strPortList[] = {
	"Port 0/Fiber",
	"Port 1",
	"Port 2",
	"Port 3",
	"Port 4"
};

const char *strSignalType[] = {
	"TTL",
	"DIFFERENTIAL"
};

const char *strOutputMode[] = {
	"From Port 0/Fiber In",
	"From Port 1 In",
	"IRIG-B",
	"PPS"
};

const char *strHardWareId[] = {
	"N/A",
	"DA_IRIGB_4DIO_PCI104",
	"DE2_IRIGB_4DIO",
	"N/A",
	"N/A",
	"N/A",
	"N/A",
	"DA_IRIGB_S"
};

const char *strParityCheckMode[] = {
	"Even",
	"Odd",
	"None"
};

enum _IRIG_CMD_FUN_CODE_LIST_ {
	FUNCODE_mxIrigbGetHardwareID,
	FUNCODE_mxIrigbGetTime,
	FUNCODE_mxIrigbSetTime,
	FUNCODE_mxIrigbGetSyncTimeSrc,
	FUNCODE_mxIrigbSetSyncTimeSrc,
	FUNCODE_mxIrigbGetSignalStatus,
	FUNCODE_mxIrigbGetInputParityCheckMode,
	FUNCODE_mxIrigbSetInputParityCheckMode,
	FUNCODE_mxIrigbGetOutputParityCheckMode,
	FUNCODE_mxIrigbSetOutputParityCheckMode,
	FUNCODE_mxIrigbGetPpsWidth,
	FUNCODE_mxIrigbSetPpsWidth,
	FUNCODE_mxIrigbGetInputSignalType,
	FUNCODE_mxIrigbSetInputSignalType,
	FUNCODE_mxIrigbGetOutputSignalType,
	FUNCODE_mxIrigbSetOutputSignalType,
	FUNCODE_mxIrigbGetDigitalOutputSignal,
	FUNCODE_mxIrigbSetDigitalOutputSignal,
	FUNCODE_mxIrigbGetDigitalInputSignal,
	FUNCODE_mxIrigbGetFpgaBuildDate,

	FUNCODE_MAX
};

struct irigcmd {
	int cmdFunCode;
	char *funDesc;
	int paraCount;
	char *paraDesc;
};

irigcmd irigcmdlist[] = {
	{	FUNCODE_mxIrigbGetHardwareID,
		"Get Hardware ID", 0,
		NULL
	},
	{	FUNCODE_mxIrigbGetTime,
		"Get IRIG-B RTC Time", 0,
		NULL
	},
	{	FUNCODE_mxIrigbSetTime,
		"Set IRIG-B RTC Time", 6,
		"year,month,day,hour,minute,second\n\t\t\
[2000-2099],[1-12],[1-31],[0-23],[0-59],[0-59]\
\n\t  default value is 2014,01,01,00,00,00 if no argument."
	},
	{	FUNCODE_mxIrigbGetSyncTimeSrc,
		"Get IRIG-B RTC Sync. Source", 0,
		NULL
	},
	{	FUNCODE_mxIrigbSetSyncTimeSrc,
		"Set IRIG-B RTC Sync. Source", 1,
		"Source\n\t\tSource:\t\
0: FreeRun In (Internal RTC)\n\t\t\t1: Port 0/Fiber In, 2: Port 1 In\
\n\t  default value is 2 if no argument."
	},
	{	FUNCODE_mxIrigbGetSignalStatus,
		"Get IRIG-B Signal Status", 1,
		"Source\n\t\tSource:\t1: Port 0/Fiber In, 2: Port 1 In\
\n\t  default value is 2 if no argument."
	},
	{	FUNCODE_mxIrigbGetInputParityCheckMode,
		"Get IRIG-B Input Parity Check Mode", 1,
		"Source\n\t\tSource:\t1: Port 0/Fiber In, 2: Port 1 In\
\n\t  default value is 2 if no argument."
	},
	{	FUNCODE_mxIrigbSetInputParityCheckMode,
		"Set IRIG-B Input Parity Check Mode", 2,
		"Source,Mode\n\t\tSource:\t1: Port 0/Fiber In, \
2: Port 1 In\n\t\tMode:\t0: Even, 1: Odd, 2: None\
\n\t  default value is 2,0 if no argument."
	},
	{	FUNCODE_mxIrigbGetOutputParityCheckMode,
		"Get IRIG-B Output Parity Check Mode", 0,
		NULL
	},
	{	FUNCODE_mxIrigbSetOutputParityCheckMode,
		"Set IRIG-B Output Parity Check Mode", 1,
		"Mode\n\t\tMode:\t0: Even, 1: Odd\
\n\t  default value is 0 if no argument."
	},
	{	FUNCODE_mxIrigbGetPpsWidth,
		"Get Pulse per second width(ms)", 0,
		NULL
	},
	{	FUNCODE_mxIrigbSetPpsWidth,
		"Set Pulse per second width(ms)", 1,
		"Width\n\t\t[0-999] (width: 0-999 ms)\
\n\t  default value is 0 if no argument."
	},
	{	FUNCODE_mxIrigbGetInputSignalType,
		"Get input signal type", 1,
		"Port\n\t\tPort:\t0: Port 0/Fiber, 1: Port 1\
\n\t  default value is 1 if no argument."
	},
	{	FUNCODE_mxIrigbSetInputSignalType,
		"Set input signal type", 3,
		"Port,Type,Inverse\n\t\t\
Port:\t0: Port 0/Fiber, 1: Port 1\n\t\t\
Type:\t0: TTL, 1: Differential\n\t\t\
Inverse:0: No inverse, 1: Inverse\
\n\t  default value is 1,1,0 if no argument."
	},
	{
		FUNCODE_mxIrigbGetOutputSignalType,
		"Get output signal type", 1,
		"Port\n\t\t[1-4] (output port[1-4])\
\n\t default value is 1 if no argument."
	},
	{
		FUNCODE_mxIrigbSetOutputSignalType,
		"Set output signal type", 4,
		"Port,Type,Mode,Inverse\n\t\t\
Port:\toutput port[1-4]\n\t\t\
Type:\t0: TTL, 1=Differential\n\t\t\
Mode:\t0: From Port 0/Fiber Input Port, 1: From Port 1 Input\n\t\t\t\
2: From IRIG-B encode(Internal RTC), 3: From PPS encode\n\t\t\
Inverse:0: No inverse, 1: Inverse)\
\n\t  default value is 1,1,2,0 if no argument."
	},
	{
		FUNCODE_mxIrigbGetDigitalOutputSignal,
		"Get Digital Output", 1,
		"Port\n\t\t[0-3] (digital output port 0-3)\
\n\t  default value is 0 if no argument."},
	{
		FUNCODE_mxIrigbSetDigitalOutputSignal,
		"Set Digital Output", 2,
		"Port,Level\n\t\tPort:\t[0-3] (digital output port[0-3])\n\t\t\
Level:\t[0-1] Output level)\n\t  default value is 0,0 if no argument."
	},
	{
		FUNCODE_mxIrigbGetDigitalInputSignal,
		"Get Digital Input", 1,
		"Port\n\t\t[0-3] (digital input port 0-3)\
\n\t  default value is 0 if no argument."
	},
	{
		FUNCODE_mxIrigbGetFpgaBuildDate,
		"Get FPGA firmware build date", 0,
		NULL
	},
};

void usage(char *name) {
    printf("Get/set Moxa DA-IRIGB utility\n");
    printf("Usage: %s -f function_id [-p parameters] [-c] [-h]\n", name);
    printf("    Show the utility information if no argument apply.\n");
    printf("    -h: Show this information.\n");
    printf("    -c: Indicate the n-the IRIG-B Card.\n");
    printf("    -f: Pass function id argument to execute specify functionality\n");
    printf("    -p: Parameters for each function, use comma to pass multiple varible\n");
    printf("\nFor example: Set IRIG-B RTC Time 2014/01/01 03:25:00\n");
    printf("\t%s -f %d -p 2014,1,1,3,25,0\n", name, FUNCODE_mxIrigbSetTime);
    printf("\nFunction description list:\n");
    for (int i=0; i<(sizeof(irigcmdlist)/sizeof(irigcmd));i++)
    {
        printf("\t%d: %s\n", irigcmdlist[i].cmdFunCode, irigcmdlist[i].funDesc);
        if (irigcmdlist[i].paraCount)
            printf("\t  -p %s\n", irigcmdlist[i].paraDesc);
    }
    printf("\n");
}

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

#ifdef WIN32
extern int
getopt(int nargc, char *const *nargv, const char *ostr);
#else
#include <unistd.h> 	/* For getopt() */
#endif
extern int optind, opterr, optopt;
extern char *optarg;

#ifdef __cplusplus	// If used by C++ code, 
}
#endif

int parsingString( char* source, char **split, char *key, int count )
{
	char *pch = strtok(source, key );
	int i;

	for (i=0;pch!=NULL && i<count;i++)
	{
		split[i] = pch;
		pch = strtok(NULL, key );	
	}

	return i;
}

#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	int cardIndex = 0;
	int controlMode = -1;
	char parameters[260] = "";
	char optstring[] = "hc:f:p:";
	char c;
	BOOL ret = FALSE;
	int result = 0;
	char *p[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	RTCTIME rtctime;

	if ( argc == 1 ) {
		usage(argv[0]);
		return 0;
	}

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			cardIndex=atoi(optarg);
			break;
		case 'f':
			controlMode = atoi(optarg);
			if ( controlMode < 0 || controlMode > (FUNCODE_MAX-1) ) {
				printf("The controlMode is over range:%d\n", controlMode);
				return 0;
			}
			break;
		case 'p':
			strcpy(parameters,optarg);
			break;
		case '?':
			printf("Invalid option\n");
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 0;
        }
	}

	if ( controlMode > (sizeof(irigcmdlist)/sizeof(irigcmd))) {
		fprintf(stderr, "Invalid Function code!\n");
		return -1;
	}

	if (irigcmdlist[controlMode].paraCount) {
		if ( irigcmdlist[controlMode].paraCount < 
			parsingString( parameters, p, ",", 
				irigcmdlist[controlMode].paraCount ) ) {
			fprintf(stderr, "Invalid Parameters!\n");
			return -1;
		}
	}

	HANDLE hDev = mxIrigbOpen(cardIndex);
#ifdef WIN32
	if (hDev == INVALID_HANDLE_VALUE || hDev == NULL) {
#else
	if ( hDev < 0 ) {
#endif
		fprintf(stderr, "mxIrigbOpen ");
#ifdef WIN32
		printf("error number = %d\n", GetLastError());
#endif
		return -1;
	}

	DWORD dwHwId;
	ret = mxIrigbGetHardwareID(hDev, &dwHwId);
	if ( ret ) {
		printf("Hardware ID = %ld (%s)\n",
			dwHwId, strHardWareId[dwHwId]);
	} else {
		fprintf(stderr, "mxIrigbGetHardwareID error!");
		return -1;
	}

	if (FUNCODE_mxIrigbGetHardwareID == controlMode) {
		printf("Hardware ID = %ld (%s)\n",
			dwHwId, strHardWareId[dwHwId]);
	} else if (FUNCODE_mxIrigbGetTime == controlMode) {
		ret = mxIrigbGetTime( hDev, &rtctime);
		if ( ret ) {
			printf("IRIG-B RTC = %d/%d/%d %d:%d:%d.%09d\nTZ = ",
				rtctime.year, rtctime.mon, rtctime.mday,
				rtctime.hour, rtctime.min, rtctime.sec,
				rtctime.nanosec);
			if (rtctime.tzs) {
				printf("-");
			} else {
				printf("+");
			}
			printf("%d", rtctime.tz);
			if (rtctime.tzh) {
				printf(".5");
			}
			printf(", TQ = %d", rtctime.tq);
			printf(", LSP = %d", rtctime.lsp);
			printf(", LS = %d", rtctime.ls);
			printf(", DSP = %d", rtctime.dsp);
			printf(", DST = %d", rtctime.dst);
			printf("\n");
		}
	} else if (FUNCODE_mxIrigbSetTime==controlMode) {
		rtctime.year = ( !p[0] ) ? 2014 : atoi(p[0]);
		rtctime.mon = ( !p[1] ) ? 1 : atoi(p[1]);
		rtctime.mday = ( !p[2] ) ? 1 : atoi(p[2]);
		rtctime.hour = ( !p[3] ) ? 0 : atoi(p[3]);
		rtctime.min = ( !p[4] ) ? 0 : atoi(p[4]);
		rtctime.sec = ( !p[5] ) ? 0 : atoi(p[5]);

		/* 2000 and 2099 is the O.S. limitation. 
			We don't allow to set the IRIG-B time over this range
		*/
		if (rtctime.year>=2000 && rtctime.year<=2099 &&
			rtctime.mon>=1 && rtctime.mon<=12 &&
			rtctime.mday>=1 && rtctime.mday<=31 &&
			rtctime.hour>=0 && rtctime.hour<=23 &&
			rtctime.min>=0 && rtctime.min<=59 &&
			rtctime.sec>=0 && rtctime.sec<=59 ) {
			ret = mxIrigbSetTime( hDev, &rtctime);
			if (ret) {
				printf("Set IRIGB RTC = %d/%d/%d %d:%d:%d\n", 
					rtctime.year, rtctime.mon, rtctime.mday,
					rtctime.hour, rtctime.min, rtctime.sec);
			}
		} else {
			result = -1;
		}
	} else if (FUNCODE_mxIrigbGetSyncTimeSrc == controlMode) {
		DWORD dwSync;
		ret = mxIrigbGetSyncTimeSrc( hDev, &dwSync);
		if (ret) {
			printf("Sync. Source = %ld (%s)\n",
				dwSync, strTimeSrc[dwSync]);
		}
	} else if (FUNCODE_mxIrigbSetSyncTimeSrc == controlMode) {
		DWORD dwSync = ( !p[0] ) ? TIMESRC_PORT1 : atoi(p[0]);

		ret = mxIrigbSetSyncTimeSrc(hDev, dwSync);
		if (ret) {
			printf("Set Sync. Source = %ld\n", dwSync);
		}
	} else if (FUNCODE_mxIrigbGetSignalStatus == controlMode) {
		DWORD dwSync = ( !p[0] ) ? TIMESRC_PORT1 : atoi(p[0]);
		DWORD dwStatus;
		
		ret = mxIrigbGetSignalStatus(hDev, dwSync, &dwStatus);
		if (ret) {
			printf("%s Signal status = %ld(%s)\n",
				strTimeSrc[dwSync], dwStatus,
				strSignalStatus[dwStatus]);
		}
	} else if (FUNCODE_mxIrigbGetPpsWidth == controlMode) {
		DWORD dwPpsWidth;

		ret = mxIrigbGetPpsWidth(hDev, &dwPpsWidth);
		if (ret) {
			printf("PPS width = %ld ms\n", dwPpsWidth);
		}
	} else if (FUNCODE_mxIrigbSetPpsWidth == controlMode) {
		DWORD dwPpsWidth = ( !p[0] ) ? 0 : atoi(p[0]);

		ret = mxIrigbSetPpsWidth(hDev, dwPpsWidth);
		if (ret) {
			printf("Set PPS width = %ld ms\n", dwPpsWidth);
		}
	} else if (FUNCODE_mxIrigbGetInputSignalType == controlMode) {
		DWORD dwPort = ( !p[0] ) ? PORT_1 : atoi(p[0]);
		DWORD dwSignalType;
		BOOL bInvert;
		ret = mxIrigbGetInputSignalType( hDev,
			dwPort, &dwSignalType, &bInvert);
		if (ret) {
			printf("%s Input signal = %ld(%s), Inverse = %d\n", 
				strPortList[dwPort], dwSignalType,
				strSignalType[dwSignalType], bInvert);
		}
	} else if (FUNCODE_mxIrigbSetInputSignalType == controlMode) {
		DWORD dwPort = ( !p[0] ) ? PORT_1 : atoi(p[0]);
		DWORD dwSignalType = ( !p[1] ) ? TYPE_DIFFERENTIAL : atoi(p[1]);
		BOOL bInvert = ( !p[2] ) ? 0 : atoi(p[2]);

		if( ! ((bInvert==0) || (bInvert==1)) ) {
			printf("The inverse only support value 0 or 1.\n");
			mxIrigbClose(hDev);
			return -1;
		}

		/* The fiber only support TTL signal */
		if ( dwPort == PORT_FIBER &&
			dwSignalType == TYPE_DIFFERENTIAL) {
			printf("This port support TTL signal only.\n");
			mxIrigbClose(hDev);
			return -1;
		}

		if (dwHwId == DE2_IRIGB_4DIO) {
			if ( dwPort == PORT_1 && dwSignalType == TYPE_TTL) {
				printf("This port does not support TTL.\n");
				mxIrigbClose(hDev);
				return -1;
			}
		}

		ret = mxIrigbSetInputSignalType(
			hDev, dwPort, dwSignalType, bInvert);
		if (ret) {
			printf("Set %s Input signal = %ld(%s), Inverse = %d\n",
				strPortList[dwPort], dwSignalType,
				strSignalType[dwSignalType], bInvert);
		}
	} else if (FUNCODE_mxIrigbGetOutputSignalType == controlMode) {
		DWORD dwPort = ( !p[0] ) ? PORT_1 : atoi(p[0]);
		DWORD dwSignalType;
		DWORD dwMode;
		BOOL bInvert;

		ret = mxIrigbGetOutputSignalType(
			hDev, dwPort, &dwSignalType, &dwMode, &bInvert);
		if (ret) {
			printf("%s Output signal = %ld(%s), ",
				strPortList[dwPort], dwSignalType,
				strSignalType[dwSignalType] );
			printf("Mode = %ld(%s), Inverse = %d\n",
				dwMode, strOutputMode[dwMode], bInvert);
		}
	} else if (FUNCODE_mxIrigbSetOutputSignalType == controlMode) {
		DWORD dwPort = ( !p[0] ) ? PORT_1 : atoi(p[0]);
		DWORD dwSignalType = ( !p[1] ) ? TYPE_DIFFERENTIAL : atoi(p[1]);
		DWORD dwMode = ( !p[2] ) ? MODE_IRIGB : atoi(p[2]);
		BOOL bInvert = ( !p[3] ) ? 0 : atoi(p[3]);

		ret = mxIrigbSetOutputSignalType(
			hDev, dwPort, dwSignalType, dwMode, bInvert);
		if (ret) {
			printf("%s Output signal = %ld(%s), ",
				strPortList[dwPort], dwSignalType,
				strSignalType[dwSignalType] );
			printf("Mode = %ld(%s), Inverse = %d\n",
				dwMode, strOutputMode[dwMode], bInvert);
		}
	} else if (FUNCODE_mxIrigbGetDigitalOutputSignal == controlMode) {
		DWORD dwPort = ( !p[0] ) ? 0 : atoi(p[0]);
		DWORD dwValue;

		ret = mxIrigbGetDigitalOutputSignal( hDev, dwPort, &dwValue);
		if (ret) {
			printf("Get DO %ld = %ld\n", dwPort, dwValue);
		}
	} else if (FUNCODE_mxIrigbSetDigitalOutputSignal == controlMode) {
		DWORD dwPort = ( !p[0] ) ? 0 : atoi(p[0]);
		DWORD dwValue = ( !p[1] ) ? 0 : atoi(p[1]);

		ret = mxIrigbSetDigitalOutputSignal( hDev, dwPort, dwValue);
		if (ret) {
			printf("Set DO %ld = %ld\n", dwPort, dwValue);
		}
	} else if (FUNCODE_mxIrigbGetDigitalInputSignal == controlMode) {
		DWORD dwPort = ( !p[0] ) ? 0 : atoi(p[0]);
		DWORD dwValue;

		if (dwHwId == DA_IRIGB_4DIO_PCI104 && dwPort >= 3) {
			printf("Not support!\n");
			mxIrigbClose(hDev);
			return -1;
		}
		ret = mxIrigbGetDigitalInputSignal( hDev, dwPort, &dwValue);
		if (ret) {
			printf("Get DI %ld = %ld\n", dwPort, dwValue);
		}
	} else if (FUNCODE_mxIrigbSetInputParityCheckMode == controlMode) {
		DWORD dwPort = ( !p[0] ) ? TIMESRC_PORT1 : atoi(p[0]);
		DWORD dwValue = ( !p[1] ) ? PARITY_CHECK_EVEN : atoi(p[1]);

		ret = mxIrigbSetInputParityCheckMode( hDev, dwPort, dwValue);
		if (ret) {
			printf("Set %s IRIG-B input parity check mode = %s\n",
				strTimeSrc[dwPort], strParityCheckMode[dwValue]);
		}
	} else if (FUNCODE_mxIrigbGetInputParityCheckMode == controlMode) {
		DWORD dwPort = ( !p[0] ) ? TIMESRC_PORT1 : atoi(p[0]);
		DWORD dwValue;

		ret = mxIrigbGetInputParityCheckMode( hDev, dwPort, &dwValue);
		if (ret) {
			printf("%s IRIG-B input parity check mode = %s\n",
				strTimeSrc[dwPort], strParityCheckMode[dwValue]);
		}
	} else if (FUNCODE_mxIrigbSetOutputParityCheckMode == controlMode) {
		DWORD dwValue = ( !p[0] ) ? PARITY_CHECK_EVEN : atoi(p[0]);

		ret = mxIrigbSetOutputParityCheckMode( hDev, dwValue);
		if (ret) {
			printf("Set IRIG-B output parity check mode = %s\n",
				strParityCheckMode[dwValue]);
		}
	} else if (FUNCODE_mxIrigbGetOutputParityCheckMode == controlMode) {
		DWORD dwValue;

		ret = mxIrigbGetOutputParityCheckMode( hDev, &dwValue);
		if (ret) {
			printf("Get IRIG-B Output parity check mode = %s\n",
				strParityCheckMode[dwValue]);
		}
	} else if (FUNCODE_mxIrigbGetFpgaBuildDate == controlMode) {
		DWORD dwValue;

		ret = mxIrigbGetFpgaBuildDate( hDev, &dwValue);
		if (ret) {
			printf("FPGA firmware build date = %08x\n", dwValue);
		}
	}

	mxIrigbClose(hDev);
	if (!ret)
	{
		fprintf(stderr, "Invalid Command\n");
		result = -1;
	}

	return result;
}

