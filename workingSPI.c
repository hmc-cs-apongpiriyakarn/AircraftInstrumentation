// Aom Pongpiriyakarn
// apongpiriyakarn@hmc.edu

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

// Constants
#define SAMPLESPERSEC 2
#define MICROSPERSAMPLE (1000000/SAMPLESPERSEC)
#define SECSPERINTERVAL 1
#define SAMPLESPERINTERVAL (SAMPLESPERSEC * SECSPERINTERVAL)
#define STRBUFSIZE 80

// globals
char tbuf[STRBUFSIZE];
char data[7];
int h;
short samples[SAMPLESPERSEC * SECSPERINTERVAL][3];

double gettime();
void spiSend(char *data, int count);
int readBytes(int handle, char *data, int count);

void initADXL345(void) {
    int speedSPI = 2000000;
    gpioInitialise();
    h = spiOpen(0, speedSPI, 3);
    
    data[0] = BANDWIDTH_RATE;
    data[1] = 0x0F;
    spiSend(data, 2);
    
    data[0] = DATA_FORMAT;
    data[1] = 0x0B; // +/-16G, 13-bit res
    spiSend(data, 2);
    
    data[0] = POWER_CONTROL;
    data[1] = 0x08;
    spiSend(data, 2);
}

void readADXL345(int sample) {
    int bytes;
    int16_t x, y, z;
//     double tStart, tDuration, t;
//     for (int i = 0; i < samples; i++) {
        data[0] = DATAX0;
        bytes = readBytes(h, data, 7);
        //bytes = spiSendReceiveBytes(data, 7);
//         printf("data[0]: %x \nx0: %x \tx1: %x \ny0: %x \ty1: %x \nz0: %x \tz1: %x \n",
//             data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
        
//     tStart = time_time();
    
        if (bytes == 7) {
            x = (data[2]<<8)|data[1];
            y = (data[4]<<8)|data[3];
            z = (data[6]<<8)|data[5];
//             t = time_time() - tStart;
//             printf(" x = %.3f, y = %.3f, z = %.3f\n",
//                    x*2*16.0/8192.0, y*2*16.0/8192.0, z*2*16.0/8192.0);
            samples[sample][0] = x;
            samples[sample][1] = y;
            samples[sample][2] = z;
// 		printf("sample num: %d, x = %.3f, y = %.3f, z = %.3f\n",
// 		   sample, 
// 		       samples[sample][0]*2*16.0/8192.0, 
// 		   samples[sample][1]*2*16.0/8192.0, 
// 		       samples[sample][2]*2*16.0/8192.0);
	}
//         delayMillis(200);
//     }
//     tDuration = time_time() - tStart; 
//     printf("%d samples read in %.2f seconds with sampling rate %.1f Hz\n\n", samples, tDuration, samples/tDuration); 
}

unsigned long micros(void) {
    return SYS_TIMER_CLO;
}

void getDateTime(void) {
	struct tm ts;

	time_t now = time(NULL); // read seconds since 1970
	ts = *localtime(&now);
	strftime(tbuf, sizeof(tbuf), "%a_%Y_%m_%d_%H_%M_%S_%Z", &ts); //***understand
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
	
	unsigned long mic = micros();
	
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
	printf("logdata\n");
	printf("sample num: %d, x = %.3f, y = %.3f, z = %.3f\n\n",
		   sample, 
		       samples[sample][0]*2*16.0/8192.0, 
		   samples[sample][1]*2*16.0/8192.0, 
		       samples[sample][2]*2*16.0/8192.0);
		
		sample++;
		if (sample % SAMPLESPERSEC == 0) {
			sec++;
		}
		if (sec >= SECSPERINTERVAL) {
			sec = 0;
			sample = 0;
// 			printf("time = %.3f, x = %.3f, y = %.3f, z = %.3f\n",
//                    		t, x*2*16.0/8192.0, y*2*16.0/8192.0, z*2*16.0/8192.0);
			// time to write to file
// 			fwrite(tbuf, sizeof(char), STRBUFSIZE, fptr);
			fprintf(fptr, "%s\n", tbuf);
			for(int i=0; i<SAMPLESPERINTERVAL; i++) {
				fprintf(fptr, "x = %.3f, y = %.3f, z = %.3f\n", 
					samples[i][0]*2*16.0/8192.0, 
					samples[i][1]*2*16.0/8192.0, 
					samples[i][2]*2*16.0/8192.0);
			}
// 			fwrite(samples, sizeof(short), SAMPLESPERINTERVAL, fptr);
			fflush(fptr); // make sure write completes
			getDateTime(); // update time for next interval
		}
	}
}

double gettime(void) {
    struct timeval tv;
    double t;

    gettimeofday(&tv, 0);
    t = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);
    return t;
}

// TO DO: look at the order of bytes sending
int spiSendReceiveBytes(char *data, int count) {
    int i;
    data[0] |= READ_BIT;
    if (count > 1) data[0] |= MULTI_BIT;
    for(i=0; i<count; i++)
        data[i] = spiSendReceive(data[i]);
    return i;
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
//     readADXL345(sample);
    logData();
   
    return 0;
}

