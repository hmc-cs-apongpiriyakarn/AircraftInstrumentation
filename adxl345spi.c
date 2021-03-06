// apongpiriyakarn@hmc.edu
// 3/4/19

#include "EasyPIO.h"
// #include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <pigpio.h>

#define BANDWIDTH_RATE      0x2C
#define BW_RATE       0x2C
#define POWER_CTL     0x2D
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


//     char spisend;
//     short send, rawx, rawy, rawz;
// //     short  testx, testy, testz;
//     int samples = 10; // change later
//     short x,y,z;
//     double t, tstart;

    // static int seconds_last = 99;
    // char TimeString[128];
    // timeval curTime;
    // gettimeofday(&curTime, NULL);
    // if(seconds_last == curtime.tv_sec)
    //     return;
    // seconds_last = curTime.tv_sec;
    // strftime(TimeString, 80, "%Y-%m-%d %H:%M:%S",localtime(&curTime.tv_sec));

    // bit 6 = 1 for multiple consecutive bytes
    // for READ, bit 7 = 1, for WRITE, bit 7 = 0
    // Thus, to read the device ID, the complete address is 0xC0 (0x80 + 0x40 + 0x00)
//     data[0] = 0xC0;
//     data[1] = 0x00;
    //send = (data[0] << 8) | data[1];
    //spiSendReceive16(send);
//     spiSendReceive(data[0]);
//     spiSendReceive(data[1]);

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
//         bytes = readBytes(h, data, 7);
        bytes = spiReceiveM(data, 7);
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
    
//     // Configure outout data rate, clock is 1MHz
//     data[0] = BANDWIDTH_RATE;              // 0x2C
//     data[1] = 0x0F;                     // > 800 Hz
//     spiSendM(data, 2);
//     send = (data[0] << 8) | data[1];
//     spiSendReceive16(send);
//     spiSendReceive(data[0]);
//     spiSendReceive(data[1]);

    // Set to full resolution(res increases with g range)
//     data[0] = DATA_FORMAT;          // 0x31
//     data[1] = 0x0B;    // FULL_RES bit(bit 3), +/-16 G
//     spiSendM(data, 2);
//     send = (data[0] << 8) | data[1];
//     spiSendReceive16(send);
//     spiSendReceive(data[0]);
//     spiSendReceive(data[1]);

//     // Set the wake up bit
//     data[0] = POWER_CONTROL;        // 0x2D
//     data[1] = 0x08;                     // bit 3 is wake up bit
//     spiSendM(data, 2);
//     send = (data[0] << 8) | data[1];
//     spiSendReceive16(send);
//     spiSendReceive(data[0]);
//     spiSendReceive(data[1]);

    // Bypass FIFO mode
//     data[0] = FIFO_CTL_ADR;             // 0x38
//     data[1] = 0x00;                     // disable everything FIFO
// //     send = (data[0] << 8) | data[1];
// //     spiSendReceive16(send);
//     spiSendReceive(data[0]);
//     spiSendReceive(data[1]);

//     samples=100;
//     float accelx, accely, accelz;
//     int signx, signy, signz;
//     const double accConversion = 2 * 16.0 / 8192.0;  // +/- 16g range, 13-bit resolution
//     int16_t testx,testy,testz, lx,ly,lz;
//     tstart = gettime();
//     int bytes;
//     lx=0;
//     ly=0;
//     lz=0;
    
//     for(int j=0; j<samples;j++) {
//         data[0] = DATAX0;
//         bytes = spiReceiveM(data, 7);
//         if (bytes == 7) {
//             testx = (data[2]<<8)|data[1];
//             testy = (data[4]<<8)|data[3];
//             testz = (data[6]<<8)|data[5];
            
//             if((abs(testx-lx)>30) || (abs(testy-ly)>30) || (abs(testz-lz)>30)) {
//                 printf("new\nx=%d y=%d z=%d\n", testx, testy, testz);
//                 lx = testx;
//                 ly = testy;
//                 lz = testz;
//             }
//         }
//         data[0] = spiSendReceive(data[0]);
//         data[0] = 0xF2;
//         data[1] = 0xF2;
//         for(int i = 1; i < 7; i++)
//             data[i] = spiSendReceive(data[i]);
//         data[1] = spiSendReceive(0xF2);
//         data[2] = spiSendReceive(0xF3);
//         data[3] = spiSendReceive(0xF4);
//         data[4] = spiSendReceive(0xF5);
//         data[5] = spiSendReceive(0xF6);
//         data[6] = spiSendReceive(0xF7);
//         printf("data[0]: %x \ndata[1]: %x \ndata[2]: %x \ndata[3]: %x \ndata[4]: %x \ndata[5]: %x \ndata[6]: %x \n",
//  data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
        
        
//         printf("testx = %.3f, testy = %.3f, testz = %.3f\n",
//  x * accConversion, y * accConversion, z * accConversion);
//         rawx = ((data[2] & 0x0F)<<8)|data[1];
//         rawy = ((data[4] & 0x0F)<<8)|data[3];
//         rawz = ((data[6] & 0x0F)<<8)|data[5];
//         signx = data[2] >> 7;
//         signy = data[4] >> 7;
//         signz = data[6] >> 7;
//         t = gettime();
//         accelx = (rawx/256.0) - 16*signx;
//         accely = (rawy/256.0) - 16*signy;
//         accelz = (rawz/256.0) - 16*signz;
        
//         printf("rawx = %x rawy = %x rawz = %x\n", rawx, rawy, rawz);
//         printf("time: %.3f, x = %.9f, y = %.9f, z = %.9f\n\n", t, accelx, accely, accelz);
//         delayMillis(200);
//     }
    
    
    // read data
//     for(int i = 0; i < samples; i++) {

//         // READ(0x80) + Multiple Consecutive(0x40) + DATAX0(0x32)
//         data[0] = 0xF2;
//         data[1] = 0x00;
//         data[2] = 0x00;
//         data[3] = 0x00;
//         data[4] = 0x00;
//         data[5] = 0x00;
//         data[6] = 0x00;
//         for(int k = 0; k<7; k++)
//             spiSendReceive(data[i]);
//         //data[0] = DATAX0_ADR;

//         for(int j = 0; j < 7; j++)
//             data[j] = spiSendReceive(data[j]);
//         x = (data[2] << 8) | data[1];
//         y = (data[4] << 8) | data[3];
//         z = (data[6] << 8) | data[5];
//         t = gettime();
//         printf("time: %.3f, x = %.3f, y = %.3f, z = %.3f\n", t, x*32.0/8192.0, y*32.0/8192.0, z*32.0/8192.0);
//         delayMillis(100);
//     }
//     printf("%d samples read in %.3f seconds with %.1f Hz\n", samples, t-tstart, samples/(t-tstart));


double gettime(void) {
    struct timeval tv;
    double t;

    gettimeofday(&tv, 0);
    t = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);
    return t;
}
