// Aom Pongpiriyakarn
// apongpiriyakarn@hmc.edu

#include "EasyPIO.h"
#include <time.h>
#include <sys/time.h>
#include <pigpio.h>

#define BANDWIDTH_RATE      0x2C
#define BW_RATE             0x2C
#define POWER_CTL           0x2D
#define POWER_CONTROL       0x2D
#define DATA_FORMAT         0x31 // register address
#define DATAX0              0x32

#define DATA_FORMAT_BYTES   0x0B // +/-16G range, 13-bit res (p26)
#define DATA_FORMAT_B 0x0B
#define FIFO_CTL            0x38 
#define READ_BIT            0x80
#define MULTI_BIT           0x40

int i;

double gettime();
int spiReceiveM(char *data, int count);
int spiSendM(char *data, int count);
int readBytes(int handle, char *data, int count);
int writeBytes(int handle, char *data, int count);

char data[7];
const int timeDefault = 5;  // default duration of data stream, seconds
const int freqDefault = 5;  // default sampling rate of data stream, Hz
const int freqMax = 3200;  // maximal allowed cmdline arg sampling rate of data stream, Hz
const int speedSPI = 2000000;  // SPI communication speed, bps
const int freqMaxSPI = 100000;  // maximal possible communication sampling rate through SPI, Hz (assumption)
const int coldStartSamples = 2;  // number of samples to be read before outputting data to console (cold start delays)
const double coldStartDelay = 0.1;  // time delay between cold start reads
const double accConversion = 2 * 16.0 / 8192.0;  // +/- 16g range, 13-bit resolution
const double tStatusReport = 1;  // time period of status report if data read to file, seconds

int main() {

    pioInit();
    // timerInit();

    // set channel, speed
    spiInit(244000, 0);
     double vTime = timeDefault;
    double vFreq = freqDefault;
    int samples = vFreq * vTime;
    int samplesMaxSPI = freqMaxSPI * vTime;
    int success = 1;
    int h, bytes;
    char data[7];
    int16_t x, y, z;
    double tStart, tDuration, t;
    double delay = 1.0 / vFreq;
    gpioInitialise();
    h = spiOpen(0, speedSPI, 3);
    
    data[0] = BW_RATE;
    data[1] = 0x0F;
    //writeBytes(h, data, 2);
    spiSendM(data, 2);
    data[0] = DATA_FORMAT;
    data[1] = DATA_FORMAT_B;
//     writeBytes(h, data, 2);
    spiSendM(data, 2);
    data[0] = POWER_CTL;
    data[1] = 0x08;
//     writeBytes(h, data, 2);
    spiSendM(data, 2);
    
    for (i = 0; i < coldStartSamples; i++) {
        data[0] = DATAX0;
        bytes = readBytes(h, data, 7);
        if (bytes != 7) {
            success = 0;
        }
        time_sleep(coldStartDelay);
    }
    // real reads happen here
    tStart = time_time();
    for (i = 0; i < samples; i++) {
        data[0] = DATAX0;
        bytes = readBytes(h, data, 7);
        //bytes = spiReceiveM(data, 7);
        if (bytes == 7) {
            x = (data[2]<<8)|data[1];
            y = (data[4]<<8)|data[3];
            z = (data[6]<<8)|data[5];
            t = time_time() - tStart;
            printf("time = %.3f, x = %.3f, y = %.3f, z = %.3f\n",
                   t, x * accConversion, y * accConversion, z * accConversion);
        }
        else {
            success = 0;
        }
        time_sleep(delay);  // pigpio sleep is accurate enough for console output, not necessary to use nanosleep
    }
    tDuration = time_time() - tStart;  // need to update current time to give a closer estimate of sampling rate
    printf("%d samples read in %.2f seconds with sampling rate %.1f Hz\n", samples, tDuration, samples/tDuration);
    return 0;
}

double gettime(void) {
    struct timeval tv;
    double t;

    gettimeofday(&tv, 0);
    t = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);
    return t;
}

int spiReceiveM(char *data, int count) {
    data[0] |= READ_BIT;
    if (count > 1) data[0] |= MULTI_BIT;
    for(i=0; i<count; i++)
        data[6-i] = spiSendReceive(data[i]);
    return i;
}    

int spiSendM(char *data, int count) {
    if (count > 1) data[0] |= MULTI_BIT;
    for(i=0; i<count; i++)
        data[i] = spiSendReceive(data[i]);
    return i; 
}    

int readBytes(int handle, char *data, int count) {
    data[0] |= READ_BIT;
    if (count > 1) data[0] |= MULTI_BIT;
    return spiXfer(handle, data, data, count);
}

int writeBytes(int handle, char *data, int count) {
    if (count > 1) data[0] |= MULTI_BIT;
    return spiWrite(handle, data, count);
}
