/* -*- mode: c; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
Copyright (c) 2014, Pure Engineering LLC
Copyright (c) 2015, Andrew Berezovskyi
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <limits.h>

#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/ipc.h>

#define ROWSZ 80
#define COLSZ 60
#define SHMSZ ROWSZ*COLSZ*2
//#define DEBUG
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static char *device = NULL;
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 16000000;
static uint16_t delay;

#define VOSPI_FRAME_SIZE (164)
uint8_t lepton_frame_packet[VOSPI_FRAME_SIZE];
uint16_t* lepton_image;

static void save_pgm_file(void)
{
	int i;
	unsigned int maxval = 0;
	unsigned int minval = UINT_MAX;

	FILE *f = fopen("image.pgm", "w");
	if (f == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	printf("Calculating min/max values for proper scaling...\n");
	for(i=0;i<COLSZ*ROWSZ;i++)
	{
		if (lepton_image[i] > maxval) {
			maxval = lepton_image[i];
		}
		if (lepton_image[i] < minval) {
			minval = lepton_image[i];
		}
	}
	printf("maxval = %u\n",maxval);
	printf("minval = %u\n",minval);
	
	fprintf(f,"P2\n80 60\n%u\n",maxval-minval);
	for(i=0;i<ROWSZ*COLSZ;i++)
	{
		fprintf(f,"%d ", lepton_image[i] - minval);
		fprintf(f,"\n");
	}
	fprintf(f,"\n\n");

	fclose(f);
}

int transfer(int fd)
{
	int ret;
	int i;
	int frame_number = -1;
	uint8_t tx[VOSPI_FRAME_SIZE] = {0, };
	uint8_t* src_row_data;
	uint8_t* dst_row_data;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)lepton_frame_packet,
		.len = VOSPI_FRAME_SIZE,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	if(((lepton_frame_packet[0]&0xf) != 0x0f))
	{
#ifdef DEBUG
		printf(".");
#endif
		frame_number = lepton_frame_packet[1];

		src_row_data = lepton_frame_packet + 4;
		dst_row_data = lepton_image + frame_number * ROWSZ;
		if(frame_number < COLSZ )
		{
			memcpy(dst_row_data, src_row_data, ROWSZ * 2);
		}
	}
	return frame_number;
}

void* open_shm(void)
{
	void* mem;
	int fd = shm_open("lepton_data", O_RDWR | O_CREAT, 0);
	if (fd < 0)
		pabort("can't open shared memory region");

	int res = ftruncate(fd, SHMSZ);
	if (res < 0)
		pabort("can't ftruncate shared memory fd");

	mem = mmap(NULL, SHMSZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED)
		pabort("can't mmap shared memory region");

	return mem;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;

	device = getenv("SPIDEV");
	if (!device) {
		device = "/dev/spidev1.0";
	}

	fd = open(device, O_RDWR);
	if (fd < 0)
	{
		pabort("can't open device");
	}

	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
	{
		pabort("can't set spi mode");
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
	{
		pabort("can't get spi mode");
	}

	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		pabort("can't set bits per word");
	}

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		pabort("can't get bits per word");
	}

	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		pabort("can't set max speed hz");
	}

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		pabort("can't get max speed hz");
	}

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	lepton_image = (uint16_t*) open_shm();
        while(1)
	{
		int row;
		while ((row=transfer(fd)) != COLSZ - 1) {
#ifdef DEBUG
			if (row == -1) {
				fprintf(stderr, "Incorrect row returned, retrying...\n");
				//abort();
			}
#endif
		}
	msync(lepton_image,SHMSZ,MS_SYNC);
	sleep(1);
        //close_shm();
	}

	close(fd);
	//save_pgm_file();

	return ret;
}
