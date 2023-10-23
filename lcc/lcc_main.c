
#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/boardctl.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <poll.h>

#include <nuttx/usb/usbdev.h>
#include <nuttx/usb/usbdev_trace.h>

#ifdef CONFIG_CDCACM
#  include <nuttx/usb/cdcacm.h>
#  include <nuttx/usb/cdc.h>
#endif

#include <nuttx/can/can.h>

#include "lcc-gridconnect.h"

static int can_fd;
static int tty_fd;
static char gridconnect_out[32];
static struct lcc_can_frame lcc_frame;

static int open_ttyfd(void){
	int tries = 0;
	do{
		tty_fd = open("/dev/ttyACM0", O_RDWR | O_NONBLOCK);
		if(tty_fd < 0 && errno == ENOTCONN){
			sleep(1);
		}else if(tty_fd < 0){
			printf("bad error: abort\n");
			return -1;
		}else{
			return 0;
		}
	}while(tries++ < 5);

	return -1;
}

static void write_tty_if_valid(void* buffer, int size){
	int get_val;
	if( ioctl(tty_fd, CAIOC_GETCTRLLINE, &get_val) < 0 ) {
		return;
	}

	if((get_val & CDCACM_UART_DTR) == 0){
		return;
	}

	if(write(tty_fd, buffer, size) < 0){
		printf("can't write: %s\n", strerror(errno));
	}
}

int main(int argc, FAR char *argv[]){
	struct boardioc_usbdev_ctrl_s ctrl;
	FAR void *handle;
	int ret;
	struct can_msg_s rxmsg;
	int nbytes_read;
	struct pollfd poll_fds[2];

	ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
	ctrl.action   = BOARDIOC_USBDEV_CONNECT;
	ctrl.instance = 0;
	ctrl.handle   = &handle;

	ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
	if (ret < 0){
		printf("ERROR: Failed to create the USB serial device: %d\n",
			-ret);
		return 1;
	}

	can_fd = open("/dev/can0", O_RDWR);
	if(can_fd < 0){
		printf("Can't open canfd: %s\n", strerror(errno));
		return 1;
	}

	if(open_ttyfd() < 0){
		printf("can't open tty\n");
		return 1;
	}

	poll_fds[0].fd = can_fd;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = tty_fd;
	poll_fds[1].events = POLLIN;

	printf("fd: %d %d\n", can_fd, tty_fd);
	while(1){
		if(poll(poll_fds, sizeof(poll_fds)/sizeof(poll_fds[0]), -1) < 0){
			perror("poll");
			break;
		}

		if(poll_fds[0].revents & POLLIN){
			nbytes_read = read(can_fd, &rxmsg, sizeof(rxmsg));
			if (nbytes_read < CAN_MSGLEN(0) || nbytes_read > sizeof(rxmsg)){
				perror("can read");
			}else{
				// Let's shoot this up to the user
				printf("can msg: ");
				//printf("0x%08X len: %d\n", rxmsg.cm_hdr.ch_id, rxmsg.cm_hdr.ch_dlc);

				lcc_frame.can_id = rxmsg.cm_hdr.ch_id;
				lcc_frame.can_len = rxmsg.cm_hdr.ch_dlc;
				memcpy(&lcc_frame.data, rxmsg.cm_data, 8);

				if(lcc_canframe_to_gridconnect(&lcc_frame, gridconnect_out, sizeof(gridconnect_out)) == LCC_OK){
					write_tty_if_valid(gridconnect_out, strlen(gridconnect_out));
					write_tty_if_valid("\n\n", 2);
				}
			}
		}

		if(poll_fds[1].revents & POLLIN){
			printf("tty msg\n");
			uint8_t buffer[12];
			int num = read(tty_fd, buffer, 12);

			if(num > 0){
				write_tty_if_valid(buffer, num);
			}
		}

	}

	return 0;
}
