#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <linux/spi/spidev.h>     
#include <sys/ioctl.h>   
#include <sys/socket.h>
#include <sys/mman.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/input.h>

#define RFID_DEV "/dev/input/by-path/platform-musb-hdrc.1-usb-0:1:1.0-event-kbd"

#define SERVER_PORT 3333

#define EEPROM_DEV "/sys/devices/platform/ocp/44e0b000.i2c/i2c-0/0-0050/eeprom"

int spidev_fd = -1;

static void safe_sleep(long sec,long usec)
{
	struct timeval to;

	to.tv_sec = sec;
	to.tv_usec = usec;

	select(0, NULL, NULL, NULL, &to);
}

int do_system(char *str_cmd, int* cmd_retcode)
{
    int status;

	if (NULL == str_cmd) {
		printf("Invalid params!\n");
		return -1;
	}

	if(cmd_retcode)
		*cmd_retcode = 0;
    
    status = system(str_cmd);
    
    if(status == -1) {
		printf("call system failed: %s\n",strerror(errno));
        return -1;
    } else {
		if(cmd_retcode!=NULL)
			*cmd_retcode = WEXITSTATUS(status);
        if(WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
				printf("\"%s\" run successfully.\n", str_cmd);
            } else {
				printf("\"%s\" run fail, exit code [ %d ]\n", str_cmd,  WEXITSTATUS(status));
				return -1;
            }
        } else {
			printf("exit status = [ %d ]\n", WEXITSTATUS(status));
            return -1;
        }
    }
	
    return 0;
}


int get_device_id(char device_id[10])
{
    int fd = 0;
    int nret = 0;

	if (NULL == device_id) {
        return -1;
	}
    
    if ((fd = open(EEPROM_DEV, O_RDWR)) < 0) {
        printf("open eeprom failed");
        return -1;
    }

    nret = read(fd, device_id, 9);
    if(nret != 9) {
        printf("read eeprom failed:%s, nret=%d\n", strerror(errno), nret);
        close(fd);
        return -1;
    }
    close(fd);
	return 0;
}


int init_serial(int *fd, const char *dev) {
    struct termios opt;

    /* open serial device */
    if ((*fd = open(dev, O_RDWR)) < 0) {
        perror("open()");
        return -1;
    }

    /* define termois */
    if (tcgetattr(*fd, &opt) < 0) {
        perror("tcgetattr()");
        return -1;
    }

    opt.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);
    opt.c_oflag  &= ~OPOST;

    /* Character length, make sure to screen out this bit before setting the data bit */
    opt.c_cflag &= ~CSIZE;

    /* No hardware flow control */
    opt.c_cflag &= ~CRTSCTS;

    /* 8-bit data length */
    opt.c_cflag |= CS8;

    /* 1-bit stop bit */
    opt.c_cflag &= ~CSTOPB;

    /* No parity bit */
    opt.c_iflag |= IGNPAR;

    /* Output mode */
    opt.c_oflag = 0;
    
    /* No active terminal mode */
    opt.c_lflag = 0;

    /* Input baud rate */
    if (cfsetispeed(&opt, B115200) < 0)
        return -1;

    /* Output baud rate */
    if (cfsetospeed(&opt, B115200) < 0)
        return -1;

    /* Overflow data can be received, but not read */
    if (tcflush(*fd, TCIFLUSH) < 0)
        return -1;

    if (tcsetattr(*fd, TCSANOW, &opt) < 0)
        return -1;

    return 0;
}


/*********************************************************************
* @fn	    uart0_thread_fn
* @brief    串口0接收数据处理线程
* @param    void
* @return   
* @history:
**********************************************************************/
void *uart0_thread_fn(void *arg)
{
    int fd_uart0 = *((int *)arg);
    int ret;
    fd_set fdset;
    struct timeval timeout = {10, 0};
	char buffer[1000] = {0};
    int i=0;
    char *test_str = "abcrs232\n";

    while(1) {
	    FD_ZERO(&fdset);
	    FD_SET(fd_uart0, &fdset);

		ret = select(fd_uart0+1, &fdset, NULL, NULL, &timeout);

		if (-1 == ret) {
            printf("HWTI_BEG:RS232 select error:HWTI_END\n");
            timeout.tv_sec  = 2;
			timeout.tv_usec = 0;
			continue;
		} else if (0 == ret) {
            /*　没有数据 */
            printf("HWTI_BEG:RS232 no data:HWTI_END\n");
            printf("HWTI_BEG:RS232 RX fail:HWTI_END\n");
			timeout.tv_sec  = 2;
			timeout.tv_usec = 0;
			return (void *)1;
		} else {
            /*　串口1接收缓冲区有数据　*/
			if (FD_ISSET(fd_uart0, &fdset)) {
				memset(buffer, 0x0, sizeof(buffer));

				ret = read(fd_uart0, buffer, 1000);

				if (ret < 0) {
					printf("HWTI_BEG:RS232 read error:HWTI_END\n");
					continue;
				} else if(ret == strlen(test_str)) {
                    if(strncmp(test_str, buffer, strlen(test_str)) == 0) {
                        printf("HWTI_BEG:RS232 RX success:HWTI_END\n");
                        return (void *)2;
                    } else {
                        printf("HWTI_BEG:%s:HWTI_END\n", buffer);
                        printf("HWTI_BEG:RS232 RX fail:HWTI_END\n");
                        return (void *)3;
                    }
                } else {
                    printf("ret length: %d\n", ret);
                    printf("HWTI_BEG:");
                    for(i=0; i<ret; i++) {
                        printf("%c", buffer[i]);
                    }
                    printf(":HWTI_END\n");
                    printf("HWTI_BEG:RS232 RX fail:HWTI_END\n");
                    return (void *)4;
                }
			}
		}
    }
}



/*********************************************************************
* @fn	    uart1_thread_fn
* @brief    串口1接收数据处理线程
* @param    void
* @return   
* @history:
**********************************************************************/
void *uart1_thread_fn(void *arg)
{
    int fd_uart1 = *((int *)arg);
    int ret;
    fd_set fdset;
    struct timeval timeout = {10, 0};
	char buffer[1000] = {0};
    int i=0;
    char *test_str = "abcrs485\n";

    while(1) {
	    FD_ZERO(&fdset);
	    FD_SET(fd_uart1, &fdset);

		ret = select(fd_uart1+1, &fdset, NULL, NULL, &timeout);

		if (-1 == ret) {
            printf("HWTI_BEG:RS485 select error:HWTI_END\n");
            timeout.tv_sec  = 2;
			timeout.tv_usec = 0;
			continue;
		} else if (0 == ret) {
            /*　没有数据 */
            printf("HWTI_BEG:RS485 no data:HWTI_END\n");
            printf("HWTI_BEG:RS485 RX fail:HWTI_END\n");
			timeout.tv_sec  = 2;
			timeout.tv_usec = 0;
			return (void *)1;
		} else {
            /*　串口1接收缓冲区有数据　*/
			if (FD_ISSET(fd_uart1, &fdset)) {
				memset(buffer, 0x0, sizeof(buffer));

				ret = read(fd_uart1, buffer, 1000);

				if (ret < 0) {
					printf("HWTI_BEG:RS485 read error:HWTI_END\n");
					continue;
				} else if(ret == strlen(test_str)) {
                    if(strncmp(test_str, buffer, strlen(test_str)) == 0) {
                        printf("HWTI_BEG:RS485 RX success:HWTI_END\n");
                        return (void *)2;
                    } else {
                        printf("HWTI_BEG:%s:HWTI_END\n", buffer);
                        printf("HWTI_BEG:RS485 RX fail:HWTI_END\n");
                        return (void *)3;
                    }
                } else {
                    printf("ret length: %d\n", ret);
                    printf("HWTI_BEG:");
                    for(i=0; i<ret; i++) {
                        printf("%c", buffer[i]);
                    }
                    printf(":HWTI_END\n");
                    printf("HWTI_BEG:RS485 RX fail:HWTI_END\n");
                    return (void *)4;
                }
			}
		}
    }
}

/*********************************************************************
* @fn	    test_rs232
* @brief    测试232接口，串口0
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_rs232(void)
{
    int ret = 0;
    pthread_t uart0_tid;
    int fd = -1;
    char *test_str = "abcrs232\n";
    int status = 0;

    do_system("killall ctrl_lcd", &status);

    ret = init_serial(&fd, "/dev/ttyS0");
    if (ret < 0) {
        close(fd);
        return -1;
    }

    printf("HWTI_BEG:RS232 RX start:HWTI_END\n");
	ret = pthread_create(&uart0_tid,  NULL,  uart0_thread_fn, &fd);
	if (ret) {
		printf("create uart1 thread error\n");
		return -1;
	}

    printf("HWTI_BEG:RS232 TX start:HWTI_END\n");
    printf("test str len: %d\n", strlen(test_str));
    ret = write(fd, test_str, strlen(test_str));
    if (ret == strlen(test_str)) {
        printf("rs232 send packet success\n");
    } else {
        printf("rs232 send packet fail! ret = %d\n", ret);
		return -1;
    }
       

    ret = pthread_join(uart0_tid, NULL);
    if(ret) {
        printf("thread join error\n");
        return -1;
    }

    close(fd);

    return 0;
}


/*********************************************************************
* @fn	    test_rs485
* @brief    测试485接口，串口0
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_rs485(void)
{
    int ret = 0;
    pthread_t uart1_tid;
    int fd = -1;
    char *test_str = "abcrs485\n";
    int status = 0;

    do_system("killall collect_sensor", &status);
    
    ret = init_serial(&fd, "/dev/ttyS1");
    if (ret < 0) {
        close(fd);
        return -1;
    }

    printf("HWTI_BEG:RS485 TX start:HWTI_END\n");
    ret = write(fd, test_str, strlen(test_str));
	if (ret == strlen(test_str)) {
        printf("rs485 send packet success\n");
    } else {
        printf("rs485 send packet fail! ret = %d\n", ret);
		return -1;
    }  

    safe_sleep(1, 0);

    printf("HWTI_BEG:RS485 RX start:HWTI_END\n");
	ret = pthread_create(&uart1_tid,  NULL,  uart1_thread_fn, &fd);
	if (ret) {
		printf("create uart1 thread error\n");
		return -1;
	}

    ret = pthread_join(uart1_tid, NULL);
    if(ret) {
        printf("thread join error\n");
        return -1;
    }

    close(fd);

    return 0;
}


/*********************************************************************
* @fn	    test_can
* @brief    测试CAN接口
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_can(void)
{
    int s, nbytes;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    unsigned char tmp[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    int status = 0;
    struct can_filter rfilter[1];
    fd_set fdset;
    struct timeval timeout = {10, 0};
    int ret;

    do_system("canconfig can0 bitrate 500000", &status);
    do_system("canconfig can0 start", &status);

    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    strcpy(ifr.ifr_name, "can0");
    ioctl(s, SIOCGIFINDEX, &ifr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    rfilter[0].can_id = 0x002100F1;
    rfilter[0].can_mask = CAN_EFF_MASK;

    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    //send can data
    printf("HWTI_BEG:CAN TX start:HWTI_END\n");
    frame.can_id = 0x002100F1 | CAN_EFF_FLAG;
    frame.can_dlc = sizeof(tmp);
    memcpy(frame.data, tmp, frame.can_dlc);
    ret = write(s, &frame, sizeof(frame));
    if(ret == sizeof(frame)) {
        printf("HWTI_BEG:CAN TX done with success:HWTI_END\n");
    } else {
        printf("HWTI_BEG:CAN TX done with failed:%d:HWTI_END\n", ret);
    }

    printf("HWTI_BEG:CAN RX start:HWTI_END\n");
    while(1) {

        FD_ZERO(&fdset);
	    FD_SET(s, &fdset);

		ret = select(s+1, &fdset, NULL, NULL, &timeout);

		if (-1 == ret) {
            printf("HWTI_BEG:CAN select error:HWTI_END\n");
            timeout.tv_sec  = 10;
			timeout.tv_usec = 0;
			continue;
		} else if (0 == ret) {
            /*　没有数据 */
            printf("HWTI_BEG:CAN no data:HWTI_END\n");
            printf("HWTI_BEG:CAN RX fail:HWTI_END\n");
			timeout.tv_sec  = 2;
			timeout.tv_usec = 0;
			break;
		} else {
            /*　串口1接收缓冲区有数据　*/
			if (FD_ISSET(s, &fdset)) {
                nbytes = read(s, &frame, sizeof(frame));
                if(nbytes > 0) {
                    printf("ID=0x%08x DLC=%d\n", frame.can_id, frame.can_dlc);
                    if(frame.can_dlc == 8) {
                        if(memcmp(tmp, frame.data, 8) == 0) {
                            printf("HWTI_BEG:CAN RX success:HWTI_END\n");
                        } else {
                            printf("HWTI_BEG:CAN RX fail:HWTI_END\n");
                            for(int i = 0; i < 8; i++) {
                                printf("%02x,", frame.data[i]);
                            }
                            printf("\n");
                        }
                    } else {
                        printf("HWTI_BEG:CAN RX fail:HWTI_END\n");
                    }
                    break;
                } else {
                    printf("HWTI_BEG:CAN RX fail:HWTI_END\n");
                }
            }
        }
    }

    close(s);
    do_system("canconfig can0 stop", &status);

    return 0;
}



/*********************************************************************
* @fn	    get_key_digit
* @brief    获取键盘数字
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
char get_key_digit(int code)
{
    char digit;
    switch(code) {
        case KEY_1:
            digit = 0x31;
            break;
        case KEY_2:
            digit = 0x32;
            break;
        case KEY_3:
            digit = 0x33;
            break;
        case KEY_4:
            digit = 0x34;
            break;
        case KEY_5:
            digit = 0x35;
            break;
        case KEY_6:
            digit = 0x36;
            break;
        case KEY_7:
            digit = 0x37;
            break;
        case KEY_8:
            digit = 0x38;
            break;
        case KEY_9:
            digit = 0x39;
            break;
        case KEY_0:
            digit = 0x30;
            break;
    }
    return digit;
}


/*********************************************************************
* @fn	    test_usb
* @brief    测试USB接口
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_usb(void)
{
    int fd = -1;
    int ret;
    fd_set fdset;
    struct timeval timeout = {15, 0};
    struct input_event buff;
    char cardnum[32] = {0};
    int offset = 0;
    int status = 0;

    // 循环重试
    do_system("killall ctrl_lcd", &status);

    fd = open(RFID_DEV, O_RDONLY);
    if(fd < 0) {
        printf("HWTI_BEG:USB fail:HWTI_END\n");
        printf("cannot open rfid scanner\n");
    } 

    while(1) {
	    FD_ZERO(&fdset);
	    FD_SET(fd, &fdset);

		ret = select(fd+1, &fdset, NULL, NULL, &timeout);

		if (-1 == ret) {
            timeout.tv_sec  = 15;
			timeout.tv_usec = 0;
			continue;
		} else if (0 == ret) {
            /*　没有数据 */
			timeout.tv_sec  = 15;
			timeout.tv_usec = 0;
            printf("HWTI_BEG:USB fail:HWTI_END\n");
			break;
		} else {
			if (FD_ISSET(fd, &fdset)) {
				memset(&buff, 0x0, sizeof(buff));
                ret = read(fd, &buff, sizeof(buff));
                if((buff.type == 1) && (buff.value == 1)) {
                    if(buff.code == 28) {
                        // 发送刷卡消息
                        printf("cardnum: %s\n", cardnum);
                        printf("HWTI_BEG:USB success:HWTI_END\n");
                        offset = 0;
                        memset(cardnum, 0, sizeof(cardnum));   
                        break;
                    } else {
                        if(offset < 32) {
                            cardnum[offset] = get_key_digit(buff.code);
                            offset++;
                        } else {
                            offset = 0;
                            memset(cardnum, 0, sizeof(cardnum));   
                        }
                    }
                }
			}
		}
    }

    close(fd);
    return 0;
}



/*********************************************************************
* @fn	    test_fpga
* @brief    测试FPGA接口
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_fpga(void)
{
    int fd = -1;
    off_t phy_addr = 0x01000000;
    short *map_base = NULL;
    short *virt_addr = NULL;
    int length = 12;
    short reg[6] = {0x00};
    short reg_actual[6] = {0x0021, 0x0055, 0x00aa, 0x0077, 0x0088, 0x0042};
    int i = 0;

    printf("HWTI_BEG:FPGA start:HWTI_END\n");

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd  == -1) {
        printf("open fd error!\n");
        return -1;
    }

    off_t pa_offset = phy_addr & ~(sysconf(_SC_PAGE_SIZE) - 1); // 页偏移
    map_base = (short *)mmap(NULL, length + phy_addr - pa_offset, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pa_offset);
    if (map_base == (short *) -1) {
        close(fd);
        return -1;
    }
    virt_addr = map_base + phy_addr - pa_offset;

    for(i = 0; i < 6; i++) {
        reg[i] = virt_addr[i];
        printf("0x%02x,", reg[i]);
    }
    printf("\n");
    if(memcmp(reg, reg_actual, 6) == 0) {
        printf("HWTI_BEG:FPGA success:HWTI_END\n");
    } else {
        printf("HWTI_BEG:FPGA fail:HWTI_END\n");
    }

    close(fd);


    return 0;
}


/*********************************************************************
* @fn	    test_io
* @brief    测试IO接口
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_io(void)
{
    // int fd = -1;
    // off_t phy_addr_out = 0x01000050;
    // off_t phy_addr_in = 0x01000040;
    // short *map_base = NULL;
    // short *virt_addr = NULL;
    // int length = 2;
    // int i = 0;

    // fd = open("/dev/mem", O_RDWR | O_SYNC);
    // if(fd  == -1) {
    //     printf("open fd error!\n");
    //     return -1;
    // }

    // off_t pa_offset = phy_addr_out & ~(sysconf(_SC_PAGE_SIZE) - 1); // 页偏移
    // map_base = (short *)mmap(NULL, 2 + phy_addr_out - pa_offset, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pa_offset);
    // if (map_base == (short *) -1) {
    //     close(fd);
    //     return -1;
    // }
    // virt_addr = map_base + phy_addr_out - pa_offset;

    // virt_addr[i] = 0x0F;

    // close(fd);

    return 0;
}


/*********************************************************************
* @fn	    test_adc
* @brief    测试ADC
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_adc(void)
{
    int fd = -1;
    off_t phy_addr = 0x01000200;
    short *map_base = NULL;
    short *virt_addr = NULL;
    int length = 512*6;
    int i = 0;
    int status = 0;
    short *tmp = (short *)malloc(length);
    float channel[6] = {0};

    do_system("killall collect_sensor", &status);

    safe_sleep(2, 0);

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd  == -1) {
        printf("open fd error!\n");
        return -1;
    }

    off_t pa_offset = phy_addr & ~(sysconf(_SC_PAGE_SIZE) - 1); // 页偏移
    map_base = (short *)mmap(NULL, length + phy_addr - pa_offset, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pa_offset);
    if (map_base == (short *) -1) {
        close(fd);
        return -1;
    }
    virt_addr = map_base + phy_addr - pa_offset;

    for(i = 0; i < length/2; i++) {
        tmp[i] = virt_addr[i];
    }

    for(i = 0; i < length/2; i++) {
        if(i % 6 == 0) {
            printf("\n");
        }
        printf("%hd,", tmp[i]);
    }
    printf("\n");

    for(i = 0; i < 256; i++) {
        channel[0] += tmp[i*6];
        channel[1] += tmp[i*6 + 1];
        channel[2] += tmp[i*6 + 2];
        channel[3] += tmp[i*6 + 3];
        channel[4] += tmp[i*6 + 4];
        channel[5] += tmp[i*6 + 5];
    }

    for(i = 0; i < 6; i++) {
        channel[i] = channel[i] / 256;
        printf("channel[%d]: %f, ", i, channel[i]);
    }
    printf("\n");

    if(((channel[1] < 10) && (channel[1] > -10)) && ((channel[3] < 10) && (channel[3] > -10)) &&
       ((channel[0] < 6600) && (channel[0] > 6400)) &&  ((channel[2] < 6600) && (channel[2] > 6400)) &&
       ((channel[4] < 6600) && (channel[4] > 6400)) &&  ((channel[5] < 6600) && (channel[5] > 6400))) {
        printf("HWTI_BEG:ADC success:HWTI_END\n");
    } else {
        printf("HWTI_BEG:ADC fail:HWTI_END\n");

        if((channel[1] >= 10) || (channel[1] <= -10)) {
            printf("channel 4 error: %f\n", channel[1]);
        } 
        if((channel[2] >= 6600) || (channel[2] <= 6400)) {
            printf("channel 5 error: %f\n", channel[2]);
        }
        if((channel[3] >= 10) || (channel[3] <= -10)) {
            printf("channel 6 error: %f\n", channel[3]);
        }
        if((channel[4] >= 6600) || (channel[4] <= 6400)) {
            printf("channel 1 error: %f\n", channel[4]);
        }
        if((channel[5] >= 6600) || (channel[5] <= 6400)) {
            printf("channel 2 error: %f\n", channel[5]);
        }
        if((channel[0] >= 6600) || (channel[0] <= 6400)) {
            printf("channel 3 error: %f\n", channel[0]);
        }
    }
    
    
    free(tmp);
    close(fd);


    return 0;
}

/*********************************************************************
* @fn	    test_lan
* @brief    测试LAN接口
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_lan(char *ip_addr)
{
    int sockfd;
    struct sockaddr_in server_addr;
    // struct hostent *host;
    int ret;
    fd_set fdset;
    struct timeval timeout = {10, 0};
	char buffer[1000] = {0};
    int buf_len;
    int i=0;
    char *socket_str = "gfedcba";

    /*  AF_INET:Internet, SOCK_STREAM:TCP */
    printf("HWTI_BEG:LAN test start:HWTI_END\n");
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) {
        printf("HWTI_BEG:LAN socket error:HWTI_END\n");
        return -1;
    }

    /*　绑定TCP服务器地址与端口 */
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);

    if(connect(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr))==-1) {
        printf("HWTI_BEG:LAN connect error:HWTI_END\n");
        return -1;
    } else {
        printf("HWTI_BEG:LAN connect ok:HWTI_END\n");
    }

    /* 向服务器发送数据　*/
    ret = write(sockfd, socket_str, strlen(socket_str));
    if(ret != strlen(socket_str)) {
        printf("HWTI_BEG:LAN TX error:HWTI_END\n");
        return -1;
    } else {
        printf("HWTI_BEG:LAN TX start:HWTI_END\n");
    }

    /* 接收服务器数据　*/
    while(1) {

        printf("poll socket\n");

        FD_ZERO(&fdset);
	    FD_SET(sockfd, &fdset);

		ret = select(sockfd+1, &fdset, NULL, NULL, &timeout);

		if (-1 == ret) {
            printf("HWTI_BEG:LAN select error:HWTI_END\n");
            timeout.tv_sec  = 2;
			timeout.tv_usec = 0;
			continue;
		} else if (0 == ret) {
            /*　没有数据 */
            printf("HWTI_BEG:LAN RX fail:HWTI_END\n");
			timeout.tv_sec  = 2;
			timeout.tv_usec = 0;
			break;
		} else {
            /*　串口1接收缓冲区有数据　*/
			if (FD_ISSET(sockfd, &fdset)) {

				memset(buffer, 0x0, sizeof(buffer));
				buf_len = read(sockfd, buffer, 1000);

				if (buf_len < 0) {
					printf("HWTI_BEG:LAN read error:HWTI_END\n");
                    timeout.tv_sec  = 2;
			        timeout.tv_usec = 0;
					continue;
				} else if(buf_len == strlen(socket_str)) {
                    if(strncmp(socket_str, buffer, strlen(socket_str)) == 0) {
                        printf("HWTI_BEG:LAN RX success:HWTI_END\n");
                        break;
                    } else {
                        printf("HWTI_BEG:LAN RX fail:HWTI_END\n");
                        break;
                    }
                } else {
                    for(i=0; i<buf_len; i++) {
                        printf("%c", buffer[i]);
                    }
                    printf("\n");
                    printf("HWTI_BEG:LAN RX fail:HWTI_END\n");
                    break;
                }
			}
		}
    }


    close(sockfd);

    return 0;
}


int test_ec20_boot()
{
    char dev_path[] = "/sys/class/leds/ec20-powerkey/brightness";
    int fd = 0;

    if ((fd = open(dev_path, O_RDWR)) < 0) {
        perror("open failed");
        return -1;
    }
    printf("success open[%s], fd=%d\n", dev_path, fd);

    int data = 1;
    int sleep_ms = 600;
    int nret = 0;
    printf("pull down ec20 powerkey and keep %dms\n", sleep_ms);
    nret = write(fd, &data, 1);
    if(nret != 1)
    {
        printf("pull down ec20 powerkey failed:%s, nret=%d\n", strerror(errno), nret);
        close(fd);
        return -1;
    }
    safe_sleep(10, sleep_ms * 1000);
    data=0;
    nret = write(fd, &data, 1);
    if(nret != 1)
    {
        printf("pull down ec20 powerkey failed:%s, nret=%d\n", strerror(errno), nret);
        close(fd);
        return -1;
    }
    printf("pull up ec20 powerkey, ec20 boot done\n");
    close(fd);
    
    return 0;
}


int test_ec20_boot_v2()
{
    char dev_path[] = "/sys/class/leds/ec20-powerkey/brightness";
    char cmd[1024] = {0};
    int power_on = 1;
    int sleep_ms = 600;
    int status = 0;
    
    printf("pull down ec20 powerkey and keep %dms\n", sleep_ms);

    snprintf(cmd, sizeof(cmd), "echo %d > %s", power_on, dev_path);
    if(do_system(cmd, &status) != 0)
    {
        printf("pull down ec20 powerkey failed, status=%d\n", status);
        return -1;
    }

    safe_sleep(0, sleep_ms * 1000);
    
    power_on = 0;
    snprintf(cmd, sizeof(cmd), "echo %d > %s", power_on, dev_path);
    if(do_system(cmd, &status) != 0)
    {
        printf("pull down ec20 powerkey failed, status=%d\n", status);
        return -1;
    }

    printf("ec20 boot done\n");
    
    return 0;
}

int open_spidev_wifi(void)
{
    int ret = 0;
    unsigned char mode = 0;
    unsigned char bits = 8;
    unsigned int speed = 12*1000*1000;
    int status = 0;

    do_system("insmod /lib/modules/4.9.65-rt23/kernel/drivers/spi/spidev.ko", &status);

    spidev_fd = open("/dev/spidev2.1", O_RDWR);
    if (spidev_fd < 0) {
        printf("can't open device\n");
        return -1;
    } else {
        printf("open spidev2.1 succeed\n");
    }

    ret = ioctl(spidev_fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1) 
        printf("cann't set spi mode");
    ret = ioctl(spidev_fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1) 
        printf("can't get spi mode");
    ret = ioctl(spidev_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        printf("can't set bits per word");
    ret = ioctl(spidev_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
        printf("can't get bits per word");
    ret = ioctl(spidev_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        printf("can't set max speed hz");
    ret = ioctl(spidev_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        printf("can't get max speed hz");


    printf("spi mode: %d\n", mode);
    printf("bits per word: %d\n", bits);
    printf("max speed: %d KHz (%d MHz)\n", speed / 1000, speed / 1000 / 1000);

    return 0;
}

unsigned char spi_get_buff_stts(void)
{
    unsigned char rxbuff[3] = {0x00, 0x00, 0x00};
    unsigned char txbuff[3] = {0x03, 0xFF, 0xFF};

    struct spi_ioc_transfer tr ={
        .tx_buf = (unsigned long) txbuff,
        .rx_buf = (unsigned long) rxbuff,
        .len = 3,
        .delay_usecs = 0,
    };
    ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &tr);
    printf("rxbuff[0]: %02x, rxbuff[1]: %02x, rxbuff[2]: %02x\n", rxbuff[0], rxbuff[1], rxbuff[2]);
    return (rxbuff[1]&0x03);
}

unsigned char spi_int_host_stts(void)
{
    unsigned char rxbuff[3] = {0x00, 0x00, 0x00};
    unsigned char txbuff[3] = {0x06, 0xFF, 0xFF};

    struct spi_ioc_transfer tr ={
        .tx_buf = (unsigned long) txbuff,
        .rx_buf = (unsigned long) rxbuff,
        .len = 3,
        .delay_usecs = 0,
    };

    ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &tr);
    printf("rxbuff[0]: %02x, rxbuff[1]: %02x, rxbuff[2]: %02x\n", rxbuff[0], rxbuff[1], rxbuff[2]);
    
    return (rxbuff[1]&0x01);
}


unsigned short spi_rx_data_len(void)
{
    unsigned short temp = 0;
    unsigned char rxbuff[3] = {0x00, 0x00, 0x00};
    unsigned char txbuff[3] = {0x02, 0xFF, 0xFF};

    struct spi_ioc_transfer tr ={
        .tx_buf = (unsigned long) txbuff,
        .rx_buf = (unsigned long) rxbuff,
        .len = 3,
        .delay_usecs = 0,
    };

    ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &tr);
    printf("rxbuff[0]: %02x, rxbuff[1]: %02x, rxbuff[2]: %02x\n", rxbuff[0], rxbuff[1], rxbuff[2]);
    
    temp = (rxbuff[2] << 8) | rxbuff[1]; 
    return temp;
}

int spi_tx(unsigned char *buff, int len)
{
    unsigned char status;
   

    status = spi_get_buff_stts();
    if(status & 0x02) {
        write(spidev_fd, buff, len);
    } else {
        return -1;
    }
    return 0;
}

int spi_rx(unsigned char *buff)
{
    unsigned char status;
    unsigned short len;
    unsigned char rxbuff[64] = {0x00};
    unsigned char txbuff[64] = {0x00};
    int i = 0;
    char version[10] = {0x48, 0x01, 0x00, 0x00, 0x00, 0x00, 0x47, 0x03, 0x04, 0x00};

    struct spi_ioc_transfer tr ={
        .tx_buf = (unsigned long) txbuff,
        .rx_buf = (unsigned long) rxbuff,
        .len = 64,
        .delay_usecs = 0,
    };

    status = spi_int_host_stts();
    if(status) {
        len = spi_rx_data_len();
        if(len > 0) {
            printf("len: %d\n", len);
            if(len % 4) {
                len = (len/4 + 1) * 4;
            }
            txbuff[0] = 0x10;
            for(i = 0; i < len; i++) {
                txbuff[i+1] = 0xFF;
            }
            tr.len = len + 1;
            ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &tr);
            printf("rxbuff: ");
            for(i = 0; i < len; i++) {
                printf("%02x,", rxbuff[i+1]);
            }
            printf("\n");

            if(memcmp(&rxbuff[13], version, 10) == 0) {
                printf("HWTI_BEG:WIFI success:HWTI_END\n");
            }
        }
    } 
    return 0;
}

int get_wifi_version(void)
{
    unsigned char buf[17] = {0};
    unsigned char wifi_version[64] = {0};    

    buf[0] = 0x91;
    buf[1] = 0xaa;
    buf[2] = 0x01;
    buf[3] = 0x00;
    buf[4] = 0x04;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf[8] = 0x05;

    buf[9]  = 0x01;
    buf[10] = 0x07;
    buf[11] = 0x00;
    buf[12] = 0x00;
    buf[13] = 0x08;

    buf[14] = 0x00;
    buf[15] = 0x00;
    buf[16] = 0x00;

    spi_tx(buf, 17);
    safe_sleep(0, 10000);
    spi_rx(wifi_version);

    return 0;
}

int test_wifi(void)
{
    open_spidev_wifi();
    get_wifi_version();

    return 0;
}


int test_ec20_sim(void)
{
    //int status = 0;
    //char cmd[1024] = {0};

    // do_system("killall connect_internet", &status);
    // do_system("cat /dev/ttyUSB2 &", &status);
    // snprintf(cmd, 1024, "echo -e %s > /dev/ttyUSB2", "AT+CCID\r\n");
    // do_system(cmd, &status);
    // safe_sleep(5, 0);
    // do_system("killall cat /dev/ttyUSB2", &status);
    
    return 0;
}



/*********************************************************************
* @fn	    test_ec20_4G_network
* @brief    测试EC20 4G网络
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_ec20_4G_network(void)
{
	FILE *fp = NULL;
	char *p = NULL;
    int has_wwan0 = 0;
	char buf[100];
    int cnt = 0;
    
    fp = popen("ifconfig", "r");
    if(!fp) {
        printf("popen(ifconfig) failed: %s", strerror(errno));
        return -1;
    }

    while(cnt < 60) {
        while(memset(buf, 0, sizeof(buf)), fgets(buf, sizeof(buf) - 1, fp) != 0) {
            //LogDebug("read buf[%s], has_wwan0=%d", buf, has_wwan0);
            if(has_wwan0 == 0) {
                p = strstr(buf, "wwan0");
                if(p!=NULL)  {
                    has_wwan0 = 1;
                } 
            } else {
                p = strstr(buf, "inet addr:");
                if(p!=NULL)  {
                    pclose(fp);
                    printf("HWTI_BEG:4G_NETWORK success:HWTI_END\n");
                    return 0;
                }
            }
        }
        cnt++;
        safe_sleep(1, 0);
    }
    
    
    pclose(fp);
    printf("HWTI_BEG:4G_NETWORK timeout:HWTI_END\n");
	
    return -1;
}

/*********************************************************************
* @fn	    test_rtc
* @brief    测试RTC时钟
* @param    void
* @return   0: success -1: fail
* @history:
**********************************************************************/
int test_rtc(void)
{
    //存在rtc0设备就行
    int fd = -1;

    fd = open("/dev/rtc0",O_RDONLY);    
    if(fd == -1) {
        printf("error open /dev/rtc0\n");
        printf("HWTI_BEG:RTC fail:HWTI_END\n");;
    } else {
        printf("HWTI_BEG:RTC success:HWTI_END\n");;
    }

    close(fd);

    return 0;
}

void test_reset(void) {
    printf("HWTI_BEG:RESET success:HWTI_END\n"); 
}

/*********************************************************************
* @fn	    main
* @brief    进程主函数
* @param    argc[in]: 参数个数
* @param    argv[in]: 参数列表
* @return   0: success -1: fail
* @history:
**********************************************************************/
int main(int argc, char* argv[])
{
    char *cmd = NULL;
    int i = 0;
    char device_info[100] = {0};
    char deviceid[10] = {0};
    char syscmd[1024] ={0};
    int ret = 0;
    int eeprom_flag = 0;

    if(argc < 2) {
        printf("Usage: /home/root/test/hw_test [options]\n");
        return -1;
    }

    for(i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n",  i,  argv[i]);
    }

    cmd = argv[1];
    if(cmd == NULL) {
        printf("Usage: /home/root/test/hw_test [options]\n");
        return -1;
    }

    // 设置DEVICEID
    if (strcmp(cmd, "GET_DEV_INFO") == 0) {
        get_device_id(deviceid);
        printf("HWTI_BEG:GET_DEV_INFO start:HWTI_END\n");
        sprintf(device_info, "%s|192.168.225.1|%s|%s", deviceid, "V2.0.0", "V1.0.0");
        printf("HWTI_BEG:GET_DEV_INFO device info %s:HWTI_END\n", device_info);
        goto out;
    } else if(strcmp(cmd, "EEPROM") == 0) {
        printf("HWTI_BEG:EEPROM start:HWTI_END\n");
        get_device_id(deviceid);
        for(i = 0; i < 9; i++) {
            deviceid[i] = deviceid[i] - '0';
            if((deviceid[i] < 0) || (deviceid[i] > 9)) {
                printf("HWTI_BEG:EEPROM fail:HWTI_END\n");
                eeprom_flag = 1;
            } 
        }
        if(eeprom_flag == 0) {
            printf("HWTI_BEG:EEPROM success:HWTI_END\n");
        }
        goto out;
    } else if(strcmp(cmd, "SET_DEVICE_ID") == 0) {
        if(argc != 3) {
            printf("HWTI_BEG:SET_DEVICE_ID paramerter number not right:HWTI_END\n");
            goto out;
        }
        if(strlen(argv[2]) != 9) {
            printf("HWTI_BEG:SET_DEVICE_ID deviceid is composed of 9 numbers:HWTI_END\n");
            goto out;
        }

        printf("HWTI_BEG:SET_DEVICE_ID start:HWTI_END\n");
        memcpy(deviceid, argv[2], 9);
        for(i = 0; i < 9; i++) {
            deviceid[i] = deviceid[i] - '0';
            if((deviceid[i] < 0) || (deviceid[i] > 9)) {
                printf("HWTI_BEG:SET_DEVICE_ID deviceid is composed of 9 numbers:HWTI_END\n");
                printf("HWTI_BEG:SET_DEVICE_ID fail:HWTI_END\n");
                goto out;
            }
        }

        snprintf(syscmd, 1024, "echo %s > /sys/devices/platform/ocp/44e0b000.i2c/i2c-0/0-0050/eeprom", argv[2]);

        do_system(syscmd, &ret);
        if(ret) {
            printf("HWTI_BEG:SET_DEVICE_ID fail:HWTI_END\n");
        } else {
            printf("HWTI_BEG:SET_DEVICE_ID success:HWTI_END\n");
        }
    } else if(strcmp(cmd, "GET_DEVICE_ID") == 0) {
        get_device_id(deviceid);
        printf("%s\n", deviceid);
    }

    
    // 打印测试项
    if(strcmp(cmd, "RESET") == 0) {
        test_reset();
    } else if(strcmp(cmd, "RS232") == 0) {
        printf("HWTI_BEG:RS232 start:HWTI_END\n");        
        test_rs232();
    } else if(strcmp(cmd, "RS485") == 0) {
        printf("HWTI_BEG:RS485 start:HWTI_END\n");
        test_rs485();
    } else if(strcmp(cmd, "CAN") == 0) {
        printf("HWTI_BEG:CAN start:HWTI_END\n");
        test_can();
    } else if(strcmp(cmd, "FPGA") == 0) {
        printf("HWTI_BEG:FPGA start:HWTI_END\n");
        test_fpga();
    } else if(strcmp(cmd, "USB") == 0) {
        printf("HWTI_BEG:USB start:HWTI_END\n");
        test_usb();
    } else if(strcmp(cmd, "LAN") == 0) {
        if(argc != 3) {
            printf("HWTI_BEG:LAN paramerter number not right:HWTI_END\n");
            goto out;
        }
        printf("HWTI_BEG:LAN start:HWTI_END\n");
        test_lan(argv[2]);
    } else if(strcmp(cmd, "ADC") == 0) {
        printf("HWTI_BEG:ADC start:HWTI_END\n");
        test_adc();
    } else if(strcmp(cmd, "IO") == 0) {
        printf("HWTI_BEG:IO start:HWTI_END\n");
        test_io();
    } else if(strcmp(cmd, "EC20_SIM") == 0) {
        printf("HWTI_BEG:EC20_SIM start:HWTI_END\n");
        // test_ec20_sim();
    } else if(strcmp(cmd, "4G_NETWORK") == 0) {
        printf("HWTI_BEG:4G_NETWORK start:HWTI_END\n");
        test_ec20_4G_network();
    } else if(strcmp(cmd, "EC20_RESET") == 0) {
        printf("HWTI_BEG:EC20_RESET start:HWTI_END\n");
        // test_ec20_reset();
    } else if(strcmp(cmd, "WIFI") == 0) {
        printf("HWTI_BEG:WIFI start:HWTI_END\n");
        test_wifi();
    } else if(strcmp(cmd, "RTC") == 0) {
        printf("HWTI_BEG:RTC start:HWTI_END\n");
        test_rtc();
    } 


out:
    printf("hw_test exit\n");
    return 0;
}

