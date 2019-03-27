// logger.c
// 27 Feb 2019 David_Harris@hmc.edu
// start of data logger

#include <stdio.h>
#include "EasyPIO.h"
#include <time.h>
#include <sys/time.h>

// Constants
#define SAMPLESPERSEC 1000
#define MICROSPERSAMPLE (1000000/SAMPLESPERSEC)
#define SECSPERINTERVAL 20
#define SAMPLESPERINTERVAL (SAMPLESPERSEC * SECSPERINTERVAL)
#define STRBUFSIZE 80

#define BW_RATE_ADR             0x2C
#define DATA_FORMAT_ADR         0x31 // register address
#define DATA_FORMAT_BYTES_ADR   0x0B // +/-16G range, 13-bit res (p26)
#define POWER_CONTROL_ADR       0x2D
#define DATAX0_ADR              0x32
#define FIFO_CTL_ADR            0x38 

double gettime();

// globals
char tbuf[STRBUFSIZE];

void adxl345Init(void) {
	
    char data[7];
    short send;
	
    // bit 6 = 1 for multiple consecutive bytes
    // for READ, bit 7 = 1, for WRITE, bit 7 = 0
    // Thus, to read the device ID, the complete address is 0xC0 (0x80 + 0x40 + 0x00)
    data[0] = 0xC0;
    data[1] = 0x00;
    send = (data[0] << 8) | data[1];
    spiSendReceive16(send);

    // Configure outout data rate, clock is 1MHz
    data[0] = BW_RATE_ADR;              // 0x2C
    data[1] = 0x0F;                     // > 800 Hz
    send = (data[0] << 8) | data[1];
    spiSendReceive16(send);

    // Set to full resolution(res increases with g range)
    data[0] = DATA_FORMAT_ADR;          // 0x31
    data[1] = DATA_FORMAT_BYTES_ADR;    // FULL_RES bit(bit 3), +/-16 G
    send = (data[0] << 8) | data[1];
    spiSendReceive16(send);

    // Set the wake up bit
    data[0] = POWER_CONTROL_ADR;        // 0x2D
    data[1] = 0x08;                     // bit 3 is wake up bit
    send = (data[0] << 8) | data[1];
    spiSendReceive16(send);

    // Bypass FIFO mode
    data[0] = FIFO_CTL_ADR;             // 0x38
    data[1] = 0x00;                     // disable everything FIFO
    send = (data[0] << 8) | data[1];
    spiSendReceive16(send);

}

void getDateTime(void) {
	struct tm ts;

	time_t now = time(NULL); // read seconds since 1970
	ts = *localtime(&now);
	strftime(tbuf, sizeof(tbuf), "%a_%Y_%m_%d_%H_%M_%S_%Z", &ts); //***understand
}

void logData(void) {
	short samples[SAMPLESPERSEC * SECSPERINTERVAL][3];
	int low, high;
	char fname[STRBUFSIZE];
	FILE *fptr;
	int sample = 0, sec=0;
	
	// open file with current timestampe
	getDateTime();
	sprintf(fname,"log_%s", tbuf);
	if ((fptr = fopen(fname, "w")) == NULL) {
		print("Can't write %s\n", fname);
		exit(1);
	}
	
	unsigned long mic = micros();
	
	while (1) {
		while (micros()-mic < MICROSPERSAMPLE); // wait until time for next sample
		mic += MICROSPERSAMPLE; // set time for next sample
		
		// *** adjust if Need be
		low = spiSendReceive(DATAX0_ADR);
		high = spiSendReceive(DATAX0_ADR);
		samples[sample][0] = low | (high<<8);
		low = spiSendReceive(DATAX0_ADR);
		high = spiSendReceive(DATAX0_ADR);
		samples[sample][1] = low | (high<<8);
		low = spiSendReceive(DATAX0_ADR);
		high = spiSendReceive(DATAX0_ADR);
		samples[sample][2] = low | (high<<8);
		
		sample++;
		if (sample % SAMPLESPERSEC == 0) {
			sec++;
		}
		if (sec >= SECSPERINTERVAL) {
			sec = 0;
			sample = 0;
			// time to write to file
			fwrite(tbuf, sizeof(char), STRBUFSIZE, fptr);
			fwrite(samples, sizeof(short), SAMPLESPERINTERVAL, fptr);
			fflush(fptr); // make sure write completes
			getDateTime(); // update time for next interval
		}
	}
}

void main(void) {
	pioInit();
	spiInit(244000, 0);
	adxl345Init();
	logData();
}

