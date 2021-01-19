/*
 * IRIG-B time sync daemon.
 * Usage: ServiceSyncTime -t [signal type] -I -d -i [Time sync interval] -s [Time Source] -p [Parity check mode] -B
 *  -t - [signal type]
 *      0 - TTL
 *      1 - DIFF
 *      default value is 1
 *  -I - inverse the input signal
 *  -s - [Time Source] The sync source from IRIG-B Port.
 *      2 - IRIG-B Port
 *      default value is 2
 *  -i - [Time sync interval] The time interval in seconds to sync the IRIG-B time into system time.
 *      1 ~ 86400 Time sync interval. Default is 10 second.
 *  -p - [Parity check mode] Set the parity bit
 *       0: EVEN 
 *       1: ODD
 *       2: NONE
 *  -B - Run daemon in the background
 *
 *	Usage example: Enable to sync time from IRIG-B Port 1 in TTL signal type every 10 seconds. The input signal is not inverse.
 *	root@Moxa:~#  ServiceSyncTime -t 0 -s 2 -i 10
 *	Usage example: Enable to sync time from IRIG-B Port 1 in DIFF signal type every 10 seconds. The input signal is not inverse.
 *	root@Moxa:~#  ServiceSyncTime -t 1 -s 2 -i 10
 *	Usage example: Enable to sync time from Fiber PORT 1 in TTL signal type every 10 seconds. The input signal is inverse.
 *	root@Moxa:~#  ServiceSyncTime -t 1 -s 1 -i 10 -I 1
 *
 * History:
 * Date		Author			Comment
 * 09-03-2014	Jared Wu.		Create it.
 * 11-04-2014	Jared Wu.		To support 32/64-bits Linux operating system.
 *					* Revise the "unsigned long" to "unsigned long long" in struct reg_val_pair_struct and struct reg_bit_pair_struct.
 *					* Add -B to run in daemon mode.
 *					* Add to create /var/run/ServiceSyncTime.pid file.
 * 11-17-2014	Jared Wu.		Support configures the parity mode EVEN/ODD/NONE by '-p' option.
 *					Revise the '-o' and '-s' option.
 * 11-19-2014	Jared Wu.		Remove the '-o', '-f' and '-w' features and options from the time sync daemon.
 * 12-02-2014	Jared Wu.		Set time_source_interface = 1; for DA-IRIGB-4DIO-PCI104 IRIG-B
 * 12-04-2014	Jared Wu.		Set default initial value, time_source_interface = 1; in the main() entry point for DA-IRIGB-4DIO-PCI104 IRIG-B
 * 05-08-2015	Jared Wu.		Fix the Fiber port should only accept the TTL signal, not the DIFF signal.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "../mxirig/Public.h"
#include "../mxirig/mxirig.h"

#ifdef __ENABLE_OUTPUT_FEATURE__
#define DEFAULT_OUTPUT_PORT		2
#define DEFAULT_OUTPUT_FROM		2
#define MIN_PULSE_PERSECONDS		0	/* Disable the pulse output */
#define MAX_PULSE_PERSECONDS		1000	/* Toggle the pulse in 1000 ms */
#define DEFAULT_PULSE_PERSECOND		0
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */

#define DEFAULT_TIME_SOURCE		2
#define DEFAULT_INTERFACE_TYPE		1
#define MIN_TIME_SYNC_INTERVAL		1	/* 1 second */
#define MAX_TIME_SYNC_INTERVAL		86400	/* 1 day */
#define DEFAULT_DISABLE_TIME_SYNC	0
#define DEFAULT_PARITY			0	/* EVEN PARITY */
#define DEFAULT_TIME_SYNC_INTERVAL	10
#define PIDFILE				"/var/run/ServiceSyncTime.pid"

/* Used to control the daemon running. 0 for running, else for running */
int bStopping = 0;

void usage(char *name) {

	printf("IRIG-B time sync daemon.\n");
	printf("Usage: ServiceSyncTime -t [signal type] -I -d -i [Time sync interval] -s [Time Source] -p [Parity check mode] -B\n");
#ifdef __ENABLE_OUTPUT_FEATURE__
	printf("-o [port to output] -f [from port] -w [PPS width]\n");
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
	printf("   -t - [signal type]\n");
	printf("       0 - TTL\n");
	printf("       1 - DIFF\n");
	printf("       default value is %d\n", DEFAULT_INTERFACE_TYPE);
#ifdef __ENABLE_OUTPUT_FEATURE__
	printf("   -o - [port to output] - Enable the Output Port 1, Output Port 2, Output Port 3 or Output Port 4 to output\n");
	printf("       1 - Enable Output Port 1 to output\n");
	printf("       2 - Enable Output Port 2 to output\n");
	printf("       3 - Enable Output Port 3 to output\n");
	printf("       4 - Enable Output Port 4 to output\n");
	printf("       default value is %d\n", DEFAULT_OUTPUT_PORT);
	printf("   -f - [from port] - The output signal from Fiber, IRIG-B Input Port, IRIG-B encode (Internal RTC), PPS encode port\n");
	printf("       0 - From FIBER Input Port\n");
	printf("       1 - From IRIG-B Input Port to output\n");
	printf("       2 - From IRIG-B encode (Internal RTC) to output\n");
	printf("       3 - From PPS encode to output\n");
	printf("       default value is %d\n", DEFAULT_OUTPUT_FROM);
	printf("   -w - [PPS width] Set the wide of pulse per second in ms\n");
	printf("       The PPS width should be 0 ~ 1000.\n");
	printf("       default value is %d\n", DEFAULT_PULSE_PERSECOND);
	printf("   -d - Disable time sync\n");
	printf("       Default this daemon enables the IRIG-B time sync from source port to system time.\n");
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
	printf("   -I - Inverse the input ");
#ifdef __ENABLE_OUTPUT_FEATURE__
	printf("and output");
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
	printf("signal\n");
	printf("   -s - [Time Source] The sync source from FREERUN(Internal RTC), Fiber or IRIG-B port\n");
	printf("       0 - FREERUN(Internal RTC) module\n");
	printf("       1 - Fiber port\n");
	printf("       2 - IRIG-B port\n");
	printf("       default value is %d\n", DEFAULT_TIME_SOURCE);
	printf("   -i - [Time sync interval] The time interval in seconds to sync the IRIG-B time into system time.\n");
	printf("       %d ~ %d Time sync interval. Default is %d second.\n", MIN_TIME_SYNC_INTERVAL, MAX_TIME_SYNC_INTERVAL, DEFAULT_TIME_SYNC_INTERVAL);
	printf("   -p - [Parity check mode] Set the parity bit\n");
	printf("       0: EVEN\n");
	printf("       1: ODD\n");
	printf("       2: NONE\n");
#ifdef __ENABLE_OUTPUT_FEATURE__
	printf("       (The 2:NONE is unavailable in output mode)\n");
#endif
	printf("       default value is %d\n", DEFAULT_PARITY);
	printf("   -B - Run daemon in the background\n");

#ifdef __ENABLE_OUTPUT_FEATURE__
	printf("Usage example: Enable to sync time from IRIG-B Port 1, in TTL signal type every 10 seconds, and enable to output IRIG-B signal from the IRIG-B encoder. The input and output signals are not inverse.\n");
	printf("root@Moxa:~# ServiceSyncTime -t 0 -o 1 -i 10\n");
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
	printf("Usage example: Enable to sync time from IRIG-B Port 1, in TTL signal type every 10 seconds. The input signals is not inverse.\n");
	printf("root@Moxa:~# ServiceSyncTime -t 0 -i 10\n");
 
}

void usage_DA_IRIGB_4DIO_PCI104(char *name) {

	printf("IRIG-B time sync daemon.\n");
	printf("Usage: ServiceSyncTime -t [signal type] -I -d -i [Time sync interval] -p [Parity check mode] -B\n");
#ifdef __ENABLE_OUTPUT_FEATURE__
	printf("-s [Time Source] -o [port to output] -f [from port] -w [PPS width]\n");
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
	printf("   -t - [signal type]\n");
	printf("       0 - TTL\n");
	printf("       1 - DIFF\n");
	printf("       default value is %d\n", DEFAULT_INTERFACE_TYPE);
#ifdef __ENABLE_OUTPUT_FEATURE__
	printf("   -o - [port to output] - Enable the Output Port 1 to output\n");
	printf("       1 - Enable Output Port 1 to output\n");
	printf("       default value is %d\n", DEFAULT_OUTPUT_PORT);
	printf("   -f - [from port] - The output signal from Fiber, IRIG-B Input Port, IRIG-B encode (Internal RTC), PPS encode port\n");
	printf("       0 - From FIBER Input Port\n");
	printf("       1 - From IRIG-B Input Port to output\n");
	printf("       2 - From IRIG-B encode (Internal RTC) to output\n");
	printf("       3 - From PPS encode to output\n");
	printf("       default value is %d\n", DEFAULT_OUTPUT_FROM);
	printf("   -w - [PPS width] Set the wide of pulse per second in ms\n");
	printf("       The PPS width should be 0 ~ 1000.\n");
	printf("       default value is %d\n", DEFAULT_PULSE_PERSECOND);
	printf("   -d - Disable time sync\n");
	printf("       Default this daemon enables the IRIG-B time sync from source port to system time.\n");
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
	printf("   -I - inverse the input or output signal\n");
	printf("   -i - [Time sync interval] The time interval in seconds to sync the IRIG-B time into system time.\n");
	printf("       %d ~ %d Time sync interval. Default is %d second.\n", MIN_TIME_SYNC_INTERVAL, MAX_TIME_SYNC_INTERVAL, DEFAULT_TIME_SYNC_INTERVAL);
	printf("   -p - [Parity check mode] Set the parity bit\n");
	printf("       0: EVEN\n");
	printf("       1: ODD\n");
	printf("       2: NONE\n");
#ifdef __ENABLE_OUTPUT_FEATURE__
	printf("       (The 2:NONE is unavailable in output mode)\n");
#endif
	printf("       default value is %d\n", DEFAULT_PARITY);
	printf("   -B - Run daemon in the background\n");

#ifdef __ENABLE_OUTPUT_FEATURE__
	printf("Usage example: Enable to sync time from IRIG-B Port 1, in TTL signal type every 10 seconds, and enable to output IRIG-B signal from the IRIG-B encoder. The input and output signals are not inverse.\n");
	printf("root@Moxa:~# ServiceSyncTime -t 0 -o 1 -i 10\n");
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
	printf("Usage example: Enable to sync time from IRIG-B Port 1, in TTL signal type every 10 seconds. The input is not inverse.\n");
	printf("root@Moxa:~# ServiceSyncTime -t 0 -i 10\n");
}

int remove_pid_file(const char *pidFile) {

	bStopping = 1;

	if ( unlink(pidFile) < 0 ) {
		printf("unlink(%s) fail\n", pidFile);
		return -EFAULT;
	}

	return 0;
}

int create_pid_file(const char *pidFile) {
	char buf[20];
	int fd_pid;

	fd_pid = open(pidFile, O_CREAT|O_TRUNC|O_RDWR);
	if ( fd_pid < 0 ) {
		printf("Open %s fail\n", pidFile);
		return -EEXIST;
	}
	sprintf(buf, "%d", getpid());
	if(write(fd_pid, buf, sizeof(buf)) < 0 ) {
		printf("Write %s fail\n", pidFile);
		return -EACCES;
	}

	close(fd_pid);

	return 0;
}

void sig_handler_for_stopping_process(int sig) {

	remove_pid_file(PIDFILE);
}

extern int optind, opterr, optopt; 
extern char *optarg;

int main(int argc, char *argv[])
{
	int i, remove_files_signal_list[] = {SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGKILL, SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2};
	HANDLE irigbCardHandle;
	DWORD dwHWID;
	long time_sync_interval = DEFAULT_TIME_SYNC_INTERVAL;
	int signal_type = DEFAULT_INTERFACE_TYPE;
#ifdef __ENABLE_OUTPUT_FEATURE__
	int port_to_output = DEFAULT_OUTPUT_PORT;
	int from_port = DEFAULT_OUTPUT_FROM;
	int pps_width = DEFAULT_PULSE_PERSECOND;
	int disable_time_sync = DEFAULT_DISABLE_TIME_SYNC;
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
	int inverse = 0;
	int pid;
	int time_source = DEFAULT_TIME_SOURCE;
	int time_source_interface = 1;	/* IRIG-B Port 1, IRIG-B decoded 1 */
	int parity_mode = DEFAULT_PARITY;
	int be_a_Daemon = 0;
#ifdef __ENABLE_OUTPUT_FEATURE__
	char optstring[] = "ht:o:f:Iw:ds:i:p:B";
#else
	char optstring[] = "ht:Ids:i:p:B";
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
	char c;


	irigbCardHandle = mxIrigbOpen(0);

	if( irigbCardHandle < 0 ) {
		fprintf(stderr,"mxIrigbOpen() fail! device not exist!\n");
		return 0;
	}

	/* The daemon written for DA-682A DA-IRIGB-4DIO-PCI104 and DA-820 IRIG-B module.
	 * We want to get the HWID for different module control.
	 * HWID:
	 *  0x01: For DA-682A DA-IRIGB-4DIO-PCI104 module
         *  0x07: For DA-820 IRIG-B module
	 */
	if (mxIrigbGetHardwareID(irigbCardHandle, &dwHWID))
		printf("Found the IRIG-B module, Hardware ID = %lu\r\n", dwHWID);
	else
		printf("Get Hardware ID error: %d\r\n", GetLastError());

	mxIrigbClose(irigbCardHandle);

	if ( argc <= 1 ) {
		switch(dwHWID) {
			case 0x02:
			case 0x01: usage_DA_IRIGB_4DIO_PCI104(argv[0]); break;
			case 0x07: usage(argv[0]); break;
		}
		return 0;
	}

	while ((c = getopt(argc, argv, optstring)) != -1)
		switch (c) {
		case 'h':
			switch(dwHWID) {
				case 0x02:
				case 0x01: usage_DA_IRIGB_4DIO_PCI104(argv[0]); break;
				case 0x07: usage(argv[0]); break;
			}
			return 0;
		case 't':
			signal_type = atoi(optarg);
			printf("signal_type - t:%d\n", signal_type);
			if ( signal_type < 0 || signal_type > 1 ) {
				printf("Invalid t:%d is not in 0(TTL) and 1(DIFF)\n", signal_type);
				return 0;
			}
			if ((dwHWID == DE2_IRIGB_4DIO) && (signal_type == TYPE_TTL)) {
				time_source = TIMESRC_FIBER;
				time_source_interface = PORT_FIBER;
			}
			break;
#ifdef __ENABLE_OUTPUT_FEATURE__
		case 'f':
			from_port = atoi(optarg) ;
			printf("from_port - f:%d\n", from_port);
			switch(dwHWID) {
				case 0x01:
				if ( from_port < 1 || from_port > 3  ) {
					printf("Invalid f:%d, please check the usage information\n", from_port);
					return 0;
				}
				break;
				case 0x07:
				if ( from_port < 0 || from_port > 3 ) {
					printf("Invalid f:%d, please check the usage information\n", from_port);
					return 0;
				}
				break;
			}
			break;
		case 'o':
			port_to_output = atoi(optarg) ;
			printf("port_to_output - o:%d\n", port_to_output);
			switch(dwHWID) {
				case 0x01:
				if ( port_to_output != 1 ) {
					printf("Invalid o:%d, please check the usage information\n", port_to_output);
					return 0;
				}
				break;
				case 0x07:
				if ( port_to_output < 1 || port_to_output > 4 ) {
					printf("Invalid o:%d, please check the usage information\n", port_to_output);
					return 0;
				}
				break;
			}
			break;
		case 'w':
			if ( from_port != 3 ) {
				printf("Invalid from_port - f:%d, you should select the PPS encode to output with '-f 3' option\n", from_port);
				return 0;
			}

			pps_width = atoi(optarg) ;
			printf("output pps_width - w:%d, 0 ~ 1000ms\n", pps_width);
			if ( pps_width < MIN_PULSE_PERSECONDS || pps_width > MAX_PULSE_PERSECONDS ) {
				printf("Invalid w:%d is not in 0 ~ 1000\n", pps_width);
				return 0;
			}
			break;
		case 'd':
			disable_time_sync = 1 ;
			printf("Invalid disable_time_sync - d:%d\n", disable_time_sync);
			break;
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */
		case 'I':
			inverse = 1 ;
			printf("inverse - I:%d, 0(The input signal is not inversed) 1(The input signal is inversed)\n", inverse);
			break;
		case 's':
			time_source = atoi(optarg);
			printf("timesource - s:%d, 0(FREERUN) 1(IRIG0) 2(IRIG1)\n", time_source);
			switch(dwHWID) {
				case 0x01:
				if ( time_source != 2 ) {
					printf("Invalid s:%d, please check the usage information\n", time_source);
					return 0;
				}
				break;
				case 0x02:
				case 0x07:
				if ( time_source < 0 || time_source > 2 ) {
					printf("Invalid s:%d, please check the usage information\n", time_source);
					return 0;
				}

				if ( time_source == 0 )  { /* Free Run (Internal RTC */
				}
				else if ( time_source == 1 )  { /* Fiber Port, IRIG-B decoded 0 */
					time_source_interface = 0;
					if ( signal_type != 0 ) {
						printf("Invalid t:%d, you should use '-t 0'\n", signal_type);
						return 0;
					}
				}
				else if ( time_source == 2 )  { /* IRIG-B Port 1, IRIG-B decoded 1 */
					time_source_interface = 1;
				}
			}
			break;
		case 'i':
			sscanf(optarg, "%lu", &time_sync_interval);
			printf("time_sync_interval - t:%lu\n", time_sync_interval);
			if ( time_sync_interval < MIN_TIME_SYNC_INTERVAL || time_sync_interval > MAX_TIME_SYNC_INTERVAL ) {
				printf("Invalid i:%lu is not in %d ~ %d\n", time_sync_interval, MIN_TIME_SYNC_INTERVAL, MAX_TIME_SYNC_INTERVAL);
				return 0;
			}
			break;
		case 'p':
			sscanf(optarg, "%d", &parity_mode);
			printf("Input and output parity_mode:%d\n", parity_mode);
			if ( parity_mode < 0 || parity_mode > 2 ) {
				printf("Invalid parity_mode:%d is not in 0, 1 or 2.\n", parity_mode);
				return 0;
			}
			break;
		case 'B':
			be_a_Daemon = 1;
			printf("be_a_Daemon - B:%d, 0(Not run in daemon) 1(Run in Daemon)\n", be_a_Daemon);
			break;
		case '?':
			printf("Invalid option, please check the usage information\n");
		default:
			switch(dwHWID) {
				case 0x02:
				case 0x01: usage_DA_IRIGB_4DIO_PCI104(argv[0]); break;
				case 0x07: usage(argv[0]); break;
			}
			return 0;
		}

	/* To run in Daemon mode */
	if ( be_a_Daemon ) {
		pid = fork();
		if ( pid < 0 ) {
			return -1;
		}
		else if ( pid > 0 ) {
			exit(0);
		}

		/* mew session founder process */

		setsid();
		pid = fork();
		if ( pid < 0 ) {
			return -1;
		}
		else if ( pid > 0 ) {
			exit(0);
		}

		/* child process */
		chdir("/");
		umask(0);

		/* Register the signal handler for the pid file removal */
		for( i=0; i < sizeof(remove_files_signal_list)/sizeof(int); i++ )
			signal(remove_files_signal_list[i], sig_handler_for_stopping_process);

		/* Create the child process pid file */
		create_pid_file(PIDFILE);
	}

	/* Get the IRIG-B file discriptor */
	irigbCardHandle = mxIrigbOpen(0);

	if( irigbCardHandle < 0 ) {
		fprintf(stderr,"mxIrigbOpen() fail!\n");
		return 0;
	}

	fprintf(stderr,"+++Services start\n");

	/* Sync Local Time from IRIG-B RTC */
	if(!mxIrigbSyncTime(irigbCardHandle, TRUE)) {
		fprintf(stderr, "mxIrigbSyncTime() fail\n");
		mxIrigbClose(irigbCardHandle);
		return 0;
	}

	/* Set sync time source */
	if (!mxIrigbSetSyncTimeSrc(irigbCardHandle, time_source) ) {
		printf("Set sync source fail\n");
		mxIrigbClose(irigbCardHandle);
		return 0;
	}

	/* Only Fiber port and IRIG-B port1 need to set time interface */
	if ( time_source == 1 || time_source == 2 ) {

		/* Configure IRIG-B input interface and type. */ 
		fprintf(stderr,"Set PORT(%d) time_source_interface: %d signal type: %d, inverse:%d\n", time_source, time_source_interface, signal_type, inverse);

		/* Set the interface type for Fiber port and IRIG-B port 1 */
		if(!mxIrigbSetInputSignalType(irigbCardHandle, time_source_interface, signal_type, inverse)) {
			fprintf(stderr, "mxIrigbSetSignalType() fail\n");
			mxIrigbClose(irigbCardHandle);
			return 0;
		}

		/* Configure the IRIG-B input parity mode */
		fprintf(stderr,"Set input interface %d parity %d\n", time_source, parity_mode);
		if (!mxIrigbSetInputParityCheckMode(irigbCardHandle, time_source, parity_mode)) {
			fprintf(stderr, "mxIrigbSetInputParityCheckMode fail\n");
			mxIrigbClose(irigbCardHandle);
			return 0;
		}
	}

#ifdef __ENABLE_OUTPUT_FEATURE__
	/* Configure IRIG-B output port and its input time source */
	fprintf(stderr,"Set IRIG-B output port:%d, signal type:%d, from_port:%d, inverse:%d\n", port_to_output, signal_type, from_port, inverse);
	if( ! mxIrigbSetOutputInterface(irigbCardHandle, port_to_output, signal_type, from_port, inverse) ) {
		fprintf(stderr,"mxIrigbSetOutputInterface(): fail\n");
		mxIrigbClose(irigbCardHandle);
		return 0;
	}

	/* Configure the IRIG-B output parity mode */
	fprintf(stderr,"Set IRIG-B output parity %d\n",  parity_mode);
	if ( parity_mode == 2 )  {
		printf("The parity(NONE) is unavailable in output mode.\n");
	}
	else {
		if (!mxIrigbSetOutputParityCheckMode(irigbCardHandle, parity_mode) ) {
			fprintf(stderr, "mxIrigbSetOutputParityCheckMode(): fail\n");
			mxIrigbClose(irigbCardHandle);
			return 0;
		}
	}

	/* For DA-682A DA-IRIGB-4DIO-PCI104, DA-820 IRIG-B module */
	if( pps_width ) {
		fprintf(stderr,"Set PPS Width: {%d} ms\n", pps_width);
		if(!mxIrigbSetPpsWidth(irigbCardHandle, pps_width)) {
			fprintf(stderr,"mxIrigbSetPpsWidth() pps_width:%d fail\n", pps_width);
			mxIrigbClose(irigbCardHandle);
			return 0;
		}
	}
#endif  /* end of __ENABLE_OUTPUT_FEATURE__ */

	/* Sync Time Source */
	fprintf(stderr,"Sync Time Source = {%d}\n", time_source);
	if(!mxIrigbSetSyncTimeSrc(irigbCardHandle, time_source)) {
		fprintf(stderr,"mxIrigbSetSyncTimeSrc() time_source:%d fail\n", time_source);
		mxIrigbClose(irigbCardHandle);
		return 0;
	}

	struct timeval tv={0,0};

	/* Stop running when process is killed */
	while ( !bStopping ) {
		fprintf(stderr,"Sync. Time From IRIG RTC...\n");
		if(!mxIrigbSyncTime(irigbCardHandle, FALSE)) {
			fprintf(stderr,"mxIrigbSyncTime() fail\n");
		}

/*
	TODO: Can get IRIG-B status and report to other process by IPC.
*/

		/* Delay for the time sync interval */
		tv.tv_sec = time_sync_interval ;
		select(0, NULL, NULL, NULL, &tv);
	}

	fprintf(stderr,"---Services stop\n");

	mxIrigbClose(irigbCardHandle);

	return 0;
}
