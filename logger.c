// logger.c
// 27 Feb 2019 David_Harris@hmc.edu
// start of data logger

#include <stdio.h>
#include "EasyPIO.h"
#include <time.h>
#include <pigpio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// Constants
#define SAMPLESPERSEC 1000
#define MICROSPERSAMPLE (1000000/SAMPLESPERSEC)
#define SECSPERINTERVAL 20
#define SAMPLESPERINTERVAL (SAMPLESPERSEC * SECSPERINTERVAL)
#define STRBUFSIZE 80

#define DATA_FORMAT   0x31  // data format register address
#define DATA_FORMAT_B 0x0B  // data format bytes: +/- 16g range, 13-bit resolution (p. 26 of ADXL345 datasheet)
#define READ_BIT      0x80
#define MULTI_BIT     0x40
#define BW_RATE       0x2C
#define POWER_CTL     0x2D
#define DATAX0        0x32

// globals
char tbuf[STRBUFSIZE;]

const char codeVersion[3] = "0.2";  // code version number
const int timeDefault = 5;  // default duration of data stream, seconds
const int freqDefault = 5;  // default sampling rate of data stream, Hz
const int freqMax = 3200;  // maximal allowed cmdline arg sampling rate of data stream, Hz
const int speedSPI = 2000000;  // SPI communication speed, bps
const int freqMaxSPI = 100000;  // maximal possible communication sampling rate through SPI, Hz (assumption)
const int coldStartSamples = 2;  // number of samples to be read before outputting data to console (cold start delays)
const double coldStartDelay = 0.1;  // time delay between cold start reads
const double accConversion = 2 * 16.0 / 8192.0;  // +/- 16g range, 13-bit resolution
const double tStatusReport = 1;  // time period of status report if data read to file, seconds

// write data to disk every INVERVAL

int readBytes(int handle, char *data, int count) {
    data[0] |= READ_BIT;
    if (count > 1) data[0] |= MULTI_BIT;
    return spiXfer(handle, data, data, count);
}

int writeBytes(int handle, char *data, int count) {
    if (count > 1) data[0] |= MULTI_BIT;
    return spiWrite(handle, data, count);
}

void spiInit(void) {
	char vSave[256] = "";
    double vTime = timeDefault;
    double vFreq = freqDefault;
	/// SPI sensor setup
    int samples = vFreq * vTime;
    int samplesMaxSPI = freqMaxSPI * vTime;
    int success = 1;
    int h, bytes;
    char data[7];
    int16_t x, y, z;
    double tStart, tDuration, t;
    if (gpioInitialise() < 0) {
        printf("Failed to initialize GPIO!");
        return 1;
    }
    h = spiOpen(0, speedSPI, 3);
    data[0] = BW_RATE;
    data[1] = 0x0F;
    writeBytes(h, data, 2);
    data[0] = DATA_FORMAT;
    data[1] = DATA_FORMAT_B;
    writeBytes(h, data, 2);
    data[0] = POWER_CTL;
    data[1] = 0x08;
    writeBytes(h, data, 2);

    double delay = 1.0 / vFreq;  // delay between reads in seconds
}

void getDateTime(void) {
	struct tm ts;

	time_t now = time(NULL); // read seconds since 1970
	ts = *localtime(&now);
	strfftime(tbuf, sizeof(buf), "%a_%Y_%m_%d_%H_%M_%S_%Z", &ts); //***understand
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
		low = spiSendReceive(DATAX0);
		high = spiSendReceive(DATAX0);
		samples[sample][0] = low | (high<<8);
		low = spiSendReceive(DATAX0);
		high = spiSendReceive(DATAX0);
		samples[sample][1] = low | (high<<8);
		low = spiSendReceive(DATAX0);
		high = spiSendReceive(DATAX0);
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
			fwrite(samples, sizeof(short), SAMPLESPERINTERFAL, fptr);
			fflush(fptr); // make sure write completes
			getDateTime(); // update time for next interval
		}
	}
}

void main(void) {
	pioInit();
	spiInit();
	logData();
}

