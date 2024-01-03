#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#include "at_util.h"
#include "debug_and_log.h"


// set raw tty mode
static void xget1(int fd, struct termios *t, struct termios *oldt)
{
	tcgetattr(fd, oldt);
	*t = *oldt;
	cfmakeraw(t);
//	t->c_lflag &= ~(ISIG|ICANON|ECHO|IEXTEN);
//	t->c_iflag &= ~(BRKINT|IXON|ICRNL);
//	t->c_oflag &= ~(ONLCR);
//	t->c_cc[VMIN]  = 1;
//	t->c_cc[VTIME] = 0;
}

int xset1(int fd, struct termios *tio, const char *device)
{
	int ret = tcsetattr(fd, TCSAFLUSH, tio);

	if (ret) {
		LogError("can't tcsetattr for %s, error: %s", device, strerror(errno));
	}
	return ret;
}

int at_cmd_open(const char *pathname, int flags)
{
	int fd = -1;
	struct termios tiosfd, tio;
	speed_t speed = 9600;

	if((flags & O_NONBLOCK) == 0)
		flags |= O_NONBLOCK;

    fd = open(pathname, flags);
	if(-1 == fd){
		DBG_LOG_ERROR("open at port[%s] failed: %s",pathname, strerror(errno));
		return fd;	
	}
	fcntl(fd, F_SETFL, O_RDWR);

	// put device to "raw mode"
	xget1(fd, &tio, &tiosfd);
	// set device speed
	cfsetspeed(&tio, speed);
	if (tcsetattr(fd, TCSAFLUSH, &tio)) {
        LogError("can't tcsetattr for %s, error: %s", pathname, strerror(errno));
        at_cmd_close(fd);
		fd = -1;
		return fd;
    }
	LogTrace("open at port[%s] success, return fd=%d\n", pathname, fd);

	return fd;
}

void at_cmd_close(int fd)
{
	if (fd != -1)
	{
		close(fd);
		fd = -1;
	}
}

int at_cmd_send(int fd, char* at_cmd, char* final_rsp, char* at_rsp, int maxlen, long timeout_ms)
{
    int iret = 0;
    int ilen = 0;
    int rlen = 0;
	int offset = 0;
    int is_recv_end = 0;
    char at_cmd_real[100] = {0};
    char rsp_end_flag[100] = {0};
    char at_rsp_buff[100] = {0};
    struct timeval timeout = {0, 0};
    fd_set fds;

    strncpy(at_cmd_real, at_cmd, sizeof(at_cmd_real) - 1);
    sprintf(rsp_end_flag, "\r\n%s", final_rsp);
        
    // Add <cr><lf> if needed
    ilen = strlen(at_cmd);
    if ((at_cmd[ilen-1] != '\r') && (at_cmd[ilen-1] != '\n'))
    {
        ilen = sprintf(at_cmd_real, "%s\r\n", at_cmd); 
        at_cmd_real[ilen] = 0;
    }

    // Send AT
    iret = write(fd, at_cmd_real, ilen);
    LogTrace("send at cmd[%s], iret=%d", at_cmd, iret);
    
    // Wait for the response
	offset = 0;
	memset(at_rsp, 0, maxlen);
	timeout.tv_sec  = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;
	while (1)
	{
		FD_ZERO(&fds); 
		FD_SET(fd, &fds); 

		//LogTrace("timeout.tv_sec=%u, timeout.tv_usec: %u", timeout.tv_sec, timeout.tv_usec);
		iret = select(fd + 1, &fds, NULL, NULL, &timeout);
		if(iret == -1) {
			iret = 1;
			break;
		} else if(iret == 0) {
			//timeout
			iret = 2;
			break;
		} else if (FD_ISSET(fd, &fds)) {
			do {
				memset(at_rsp_buff, 0x0, sizeof(at_rsp_buff));
				rlen = read(fd, at_rsp_buff, sizeof(at_rsp_buff));

				//copy resutl
				if(rlen > 0 && offset < maxlen) {
					strncpy(at_rsp + offset, at_rsp_buff, maxlen - offset - 1);
					offset += rlen;
					LogTrace("read at response, len=%d, at_rsp_buff[%s] at_rsp[%s]", rlen, at_rsp_buff, at_rsp);
				}
				if ((rlen > 0) && (strstr(at_rsp, rsp_end_flag)     // final OK response
						|| strstr(at_rsp, "+CME ERROR:") 			 // +CME ERROR
						|| strstr(at_rsp, "+CMS ERROR:") 			 // +CMS ERROR
						|| strstr(at_rsp, "ERROR")))      			 // Unknown ERROR
				{
					is_recv_end = 1;
					iret = 0;
					break;
				}
			} while (rlen > 0);

			if(is_recv_end) {
				break;
			}
		} else {
			iret = 3;
			break;
		}

		/* Found the final response or timeout, return back */
		if (is_recv_end || iret == 2) {
		    break;
		}
   	}

   	return iret;
}
