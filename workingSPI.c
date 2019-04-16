// David Harris & Aom Pongpiriyakarn 
// harris@hmc.edu, apongpiriyakarn@hmc.edu
// Mar 13, 2019

#include "EasyPIO.h"
#include <time.h>
#include <sys/time.h>
#include <pigpio.h>

#define BANDWIDTH_RATE      0x2C
#define POWER_CONTROL       0x2D
#define DATA_FORMAT         0x31 
#define DATAX0              0x32
#define FIFO_CTL            0x38 
#define READ_BIT            0x80
#define MULTI_BIT           0x40

#define SAMPLESPERSEC 1000
#define MICROSPERSAMPLE (1000000/SAMPLESPERSEC)
#define SECSPERINTERVAL 1
#define SAMPLESPERINTERVAL (SAMPLESPERSEC * SECSPERINTERVAL)
#define STRBUFSIZE 80

// global variables
char tbuf[STRBUFSIZE];
char data[7];
int h;
long samples[SAMPLESPERSEC * SECSPERINTERVAL][4];

double gettime(void);
void spiSend(char *data, int count);
int readBytes(int handle, char *data, int count);
unsigned int micros(void);
void initADXL345(void);
void readADXL345(int sample);
void getDateTime(void);
void logData(void);

void initADXL345(void) {
	int speedSPI = 2000000;
	gpioInitialise();
    	h = spiOpen(0, speedSPI, 3);
    
    	data[0] = BANDWIDTH_RATE;
    	data[1] = 0x0F;
    	spiSend(data, 2);
    
    	data[0] = DATA_FORMAT;
    	data[1] = 0x0B;	 	// +/-16G, 13-bit res
    	spiSend(data, 2);
    
    	data[0] = POWER_CONTROL;
    	data[1] = 0x08;
    	spiSend(data, 2);
}

void readADXL345(int sample) {
    	int bytes;
    	int16_t x, y, z;
	data[0] = DATAX0;
        bytes = readBytes(h, data, 7);
    
        if (bytes == 7) {
            	x = (data[2]<<8)|data[1];
		y = (data[4]<<8)|data[3];
            	z = (data[6]<<8)|data[5];

            	samples[sample][0] = x;
            	samples[sample][1] = y;
            	samples[sample][2] = z;
	    	samples[sample][3] = micros();
	}
}

unsigned int micros(void) {
    return SYS_TIMER_CLO;
}

void getDateTime(void) {
	struct tm ts;

	time_t now = time(NULL); // read seconds since 1970
	ts = *localtime(&now);
	strftime(tbuf, sizeof(tbuf), "%a_%Y_%m_%d_%H_%M_%S_%Z", &ts); 
}

double gettime(void) {
    struct timeval tv;
    double t;

    gettimeofday(&tv, 0);
    t = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);
    return t;
}

void logData(void) {
// 	int low, high;
	char fname[STRBUFSIZE];
	FILE *fptr;
	int sample = 0, sec=0;
	
	// open file with current timestampe
	getDateTime();
	sprintf(fname,"log_%s", tbuf);
	if ((fptr = fopen(fname, "w")) == NULL) {
		printf("Can't write %s\n", fname);
		exit(1);
	}
	while(micros()%1000000);
	unsigned long mic = micros();
	unsigned long tStart = micros();
	int count = 0;
	
	while (1) {
		while (micros()-mic < MICROSPERSAMPLE); // wait until time for next sample
		mic += MICROSPERSAMPLE; // set time for next sample
		
// 		// *** adjust if Need be
// 		low = spiSendReceive(ACCELXH);
// 		high = spiSendReceive(ACCELXL);
// 		samples[sample][0] = low | (high<<8);
// 		low = spiSendReceive(ACCELYH);
// 		high = spiSendReceive(ACCELYL);
// 		samples[sample][1] = low | (high<<8);
// 		low = spiSendReceive(ACCELYH);
// 		high = spiSendReceive(ACCELYL);
// 		samples[sample][2] = low | (high<<8);

		readADXL345(sample);
		printf("sample num: %d, x = %.3f, y = %.3f, z = %.3f, micros: %lu\n\n",
			   count+sample, 
			   samples[sample][0]*2*16.0/8192.0, 
			   samples[sample][1]*2*16.0/8192.0, 
			   samples[sample][2]*2*16.0/8192.0,
			   samples[sample][3]-tStart);
		sample++;
		if (sample % SAMPLESPERSEC == 0) {
			sec++;
		}
		if (sec >= SECSPERINTERVAL) {
			sec = 0;
			sample = 0;

			// time to write to file
			fprintf(fptr, "%s\n", tbuf);
			for(int i=0; i<SAMPLESPERINTERVAL; i++) {
				fprintf(fptr, "x = %.3f, y = %.3f, z = %.3f\n", // t: %f\n", 
					samples[i][0]*2*16.0/8192.0, 
					samples[i][1]*2*16.0/8192.0, 
					samples[i][2]*2*16.0/8192.0);
					//samples[i][3]);
					count++;
				//fprintf(fptr, " t: %lu\n", samples[i][3]);
			}
			fflush(fptr); // make sure write completes
			getDateTime(); // update time for next interval
		}
	}
}

void spiSend(char *data, int count) {
    if (count > 1) data[0] |= MULTI_BIT;
    for(int i=0; i<count; i++)
        data[i] = spiSendReceive(data[i]);
}    

int readBytes(int handle, char *data, int count) {
    data[0] |= READ_BIT;
    if (count > 1) data[0] |= MULTI_BIT;
    return spiXfer(handle, data, data, count);
}

int main() {
    pioInit();
    spiInit(244000, 0); 
    initADXL345();
    logData();
   
    return 0;
}
