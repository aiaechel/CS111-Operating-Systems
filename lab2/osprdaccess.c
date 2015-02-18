#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "osprd.h"

void usage(int status)
{
	fprintf(stderr, "\
Reads from or writes to an encrypted OSP ramdisk device.\n\
Usage: ./osprdaccess -w [SIZE] [OPTIONS] [DEVICE...] < DATA\n\
   or: ./osprdaccess -w [SIZE] -z [DEVICE...]        (writes zeros)\n\
   or: ./osprdaccess -r [SIZE] [OPTIONS] [DEVICE...] > DATA\n\
   or: ./osprdaccess -p [PASSWORD] [OPTIONS] [DEVICE...] \n\
   Sets a new password for DEVICE. Default is no password.\n\
   To change the password, you must be authorized (see -u option below)\n\
   SIZE is the number of bytes to read/write.  Default is whole file.\n\
   Options are:\n\
   -o OFF\n\
       Seek forward into the file to offset OFF before reading/writing.\n\
   -l [DELAY]\n\
       Lock the ramdisk after opening it.  DELAY, if given, is the number of\n\
       seconds to wait after opening but before acquiring the lock.\n\
   -L [DELAY]\n\
       Attempt to lock the ramdisk without blocking.  This is like -l, but if\n\
       -l would block, -L will return a \"resource busy\" error instead.\n\
   -d DELAY\n\
       Wait DELAY seconds before reading/writing (but after locking).\n\
   -u [PASSWORD] Authorize the current process to decrypt the ramdisk.\n\
       If not authorized, read output will be encrypted.\n\
       Default for PASSWORD is empty string.\n\
   DEVICE is the device to read/write.  The default is /dev/osprda.\n\
   You can also give more than one device name.  All devices are opened, but\n\
   only the last device is modified.\n");
	exit(status);
}

int parse_ssize(const char *arg, ssize_t *result)
{
	char *end_arg;
	ssize_t val = strtol(arg, &end_arg, 0);
	if (*arg && !*end_arg) {
		*result = val;
		return 1;
	} else
		return 0;
}

int parse_double(const char *arg, double *result)
{
	char *end_arg;
	double val = strtod(arg, &end_arg);
	if (*arg && !*end_arg) {
		*result = val;
		return 1;
	} else
		return 0;
}

void sleep_for(double seconds)
{
	struct timeval now, delta, end;
	gettimeofday(&now, 0);
	delta.tv_sec = (long) seconds;
	delta.tv_usec = (int) ((seconds - delta.tv_sec) * 1000000);
	timeradd(&now, &delta, &end);

	while (1) {
		gettimeofday(&now, 0);
		if (!timercmp(&end, &now, >))
			break;
		timersub(&end, &now, &delta);
		(void) select(0, 0, 0, 0, &delta);
	}
}

void transfer(int fd1, int fd2, ssize_t size)
{
	char buf[BUFSIZ], *bufptr;

	while (size != 0) {
		ssize_t r = read(fd1, buf, (size > 0 && size < BUFSIZ ? size : BUFSIZ));
		if (r < 0 && (errno == EAGAIN || errno == EINTR))
			continue;
		else if (r < 0) {
			perror("read");
			exit(1);
		} else if (r == 0)
			return;
		else
			size -= r;

		bufptr = buf;
		while (r > 0) {
			ssize_t w = write(fd2, bufptr, r);
			if (w < 0 && (errno == EAGAIN || errno == EINTR))
				continue;
			else if (w < 0 && errno == ENOSPC) /* end of file */
				break;
			else if (w < 0) {
				perror("write");
				exit(1);
			} else
				bufptr += w, r -= w;
		}
	}
}

void transfer_zero(int fd2, ssize_t size)
{
	char buf[BUFSIZ];
	memset(buf, '\0', BUFSIZ);

	while (size != 0) {
		ssize_t w = write(fd2, buf, (size > 0 && size < BUFSIZ ? size : BUFSIZ));
		if (w < 0 && (errno == EAGAIN || errno == EINTR))
			continue;
		else if (w < 0 && errno == ENOSPC) /* end of file */
			break;
		else if (w < 0) {
			perror("write");
			exit(1);
		} else
			size -= w;
	}
}

int main(int argc, char *argv[])
{
	char *newarg;
	int devfd, ofd;
	int i, r, timeout = 0, zero = 0;
	int mode = O_RDONLY, dolock = 0, dotrylock = 0;
	int new_pass = 0;
	ssize_t size = -1;
	ssize_t offset = 0;
	double delay = 0;
	double lock_delay = 0;
	const char *devname = "/dev/osprda";
	char decrypt_pass[MAX_PASS_LEN] = "";
	char new_password[MAX_PASS_LEN] = "";

 flag:
	// Detect a read/write option
	if (argc >= 2 && strcmp(argv[1], "-r") == 0) {
		mode = O_RDONLY;
		argv++, argc--;
		if (argc >= 2 && parse_ssize(argv[1], &size))
			argv++, argc--;
		goto flag;
	} else if (argc >= 2 && strcmp(argv[1], "-w") == 0) {
		mode = O_WRONLY;
		argv++, argc--;
		if (argc >= 2 && parse_ssize(argv[1], &size))
			argv++, argc--;
		goto flag;
	}

	// Detect an offset
	if (argc >= 2 && strcmp(argv[1], "-o") == 0) {
		if (argc < 2 || !parse_ssize(argv[2], &offset))
			usage(1);
		argv += 2, argc -= 2;
		goto flag;
	}

	// Detect a lock option
	if (argc >= 2 && strcmp(argv[1], "-l") == 0) {
		dolock = 1;
		dotrylock = 0;
		argv++, argc--;
		if (argc >= 2 && parse_double(argv[1], &lock_delay))
			argv++, argc--;
		goto flag;
	}

	// Detect an attempt-lock option
	if (argc >= 2 && strcmp(argv[1], "-L") == 0) {
		dotrylock = 1;
		dolock = 0;
		argv++, argc--;
		if (argc >= 2 && parse_double(argv[1], &lock_delay))
			argv++, argc--;
		goto flag;
	}

	// Detect a delay option
	if (argc >= 2 && strcmp(argv[1], "-d") == 0) {
		argv++, argc--;
		if (argc >= 2 && parse_double(argv[1], &delay))
			argv++, argc--;
		goto flag;
	}

	// Detect a zeroes option
	if (argc >= 2 && strcmp(argv[1], "-z") == 0) {
		zero = 1;
		argv++, argc--;
		goto flag;
	}
	
	// Detect the set ramdisk password option
	if(argc >= 2 && strcmp(argv[1], "-p") == 0) {
		
	    //This checks if the next thing is longer than MAX_PASS_LEN
	  //	      or if -p was already given for the same ramdisk
	        new_pass = 1;	        
		argv++, argc--;

	        if(argc < 2 || argv[1][0] == '-' || argv[1][0] == '/')
		   goto flag;  //If no password afterwards, then new
		               //password is "" (empty string)
		else if(strlen(argv[1]) > MAX_PASS_LEN - 1)
		  usage(1);
		else
		{
    		   strncpy(new_password, argv[1], MAX_PASS_LEN-1);
		   fprintf(stderr, "Got password: %s\n", new_password);
		   argv++, argc--;
		}
		  
		goto flag;
	}

	// Detect the password unlock option
	if(argc >= 2 && strcmp(argv[1], "-u") == 0) {		
		argv++, argc--;
	        if(argc < 2 || argv[1][0] == '-' || argv[1][0] == '/')
		   goto flag;
		else if(strlen(argv[1]) > MAX_PASS_LEN - 1)
		  usage(1);
		else
		{
    		   strncpy(decrypt_pass, argv[1], MAX_PASS_LEN-1);
		   argv++, argc--;
		} 
		goto flag;
	}


	// Detect a help option
	if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
		usage(0);

	// Detect a device name
	if (argc >= 2 && argv[1][1] != '-') {
		devname = argv[1];
		argv++, argc--;
	}

	// Open ramdisk file
	devfd = open(devname, mode);
	if (devfd == -1) {
		perror("open");
		exit(1);
	}    

	// Lock, possibly after delay
	if (dolock || dotrylock) {
		if (lock_delay >= 0)
			sleep_for(lock_delay);
		if (dolock
		    && ioctl(devfd, OSPRDIOCACQUIRE, NULL) == -1) {
			perror("ioctl OSPRDIOCACQUIRE");
			exit(1);
		} else if (dotrylock
			   && ioctl(devfd, OSPRDIOCTRYACQUIRE, NULL) == -1) {
			perror("ioctl OSPRDIOCTRYACQUIRE");
			exit(1);
		}
	}

	// Delay
	if (delay >= 0)
		sleep_for(delay);

	// If more arguments, go around for the next ramdisk
	if (argc > 1)
		goto flag;
		// Try decrypting the ramdisk file
	// Seek to offset
	if (lseek(devfd, offset, SEEK_SET) == (off_t) -1) {
		perror("lseek");
		exit(1);
	}

	// Read or write or set password
	if ((mode & O_WRONLY) && zero)
		transfer_zero(devfd, size);
	else if (mode & O_WRONLY)
		transfer(STDIN_FILENO, devfd, size);
	else {
	  if(ioctl(devfd, OSPRDAUTHORIZE, decrypt_pass) < 0)
	      perror("The following error occurred: ");
	  if(new_pass) {
	    if(ioctl(devfd, OSPRDSETPASS, new_password) < 0)
	      perror("The following error occurred: ");
	  }
	  else
	    transfer(devfd, STDOUT_FILENO, size);
	  ioctl(devfd, OSPRDDEAUTHORIZE, NULL);
	}

	exit(0);
}
