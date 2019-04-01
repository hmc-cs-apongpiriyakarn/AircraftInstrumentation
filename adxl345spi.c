// apongpiriyakarn@hmc.edu
// 3/4/19

#include "EasyPIO.h"
// #include <stdio.h>
#include <time.h>
#include <sys/time.h>

#define BANDWIDTH_RATE      0x2C
#define POWER_CONTROL       0x2D
#define DATA_FORMAT         0x31 // register address
#define DATAX0              0x32

#define DATA_FORMAT_BYTES   0x0B // +/-16G range, 13-bit res (p26)
#define FIFO_CTL            0x38 
#define READ_BIT            0x80
#define MULTI_BIT           0x40

int i;

double gettime();
int readBytes(char *data, int count) {
    data[0] |= READ_BIT;
    if (count > 1) data[0] |= MULTI_BIT;
    for(i=0; i<count; i++)
        data[i] = spiSendReceive(data[i]);
    return i;
}    

int writeBytes(char *data, int count) {
    if (count > 1) data[0] |= MULTI_BIT;
    for(i=0; i<count; i++)
        data[i] = spiSendReceive(data[i]);
    return i; 
}    

char data[7];

int main() {

    pioInit();
    // timerInit();

    // set channel, speed
    spiInit(244000, 0);


    char spisend;
    short send, rawx, rawy, rawz;
//     short  testx, testy, testz;
    int samples = 10; // change later
    short x,y,z;
    double t, tstart;

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

    // Configure outout data rate, clock is 1MHz
    data[0] = BANDWIDTH_RATE;              // 0x2C
    data[1] = 0x0F;                     // > 800 Hz
    writeBytes(data, 2);
//     send = (data[0] << 8) | data[1];
//     spiSendReceive16(send);
//     spiSendReceive(data[0]);
//     spiSendReceive(data[1]);

    // Set to full resolution(res increases with g range)
    data[0] = DATA_FORMAT;          // 0x31
    data[1] = 0x0B;    // FULL_RES bit(bit 3), +/-16 G
    writeBytes(data, 2);
//     send = (data[0] << 8) | data[1];
//     spiSendReceive16(send);
//     spiSendReceive(data[0]);
//     spiSendReceive(data[1]);

    // Set the wake up bit
    data[0] = POWER_CONTROL;        // 0x2D
    data[1] = 0x08;                     // bit 3 is wake up bit
    writeBytes(data, 2);
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

    samples=100;
    float accelx, accely, accelz;
    int signx, signy, signz;
    const double accConversion = 2 * 16.0 / 8192.0;  // +/- 16g range, 13-bit resolution
    int16_t testx,testy,testz, lx,ly,lz;
    tstart = gettime();
    int bytes;
    lx=0;
    ly=0;
    lz=0;
    
    for(int j=0; j<samples;j++) {
        data[0] = DATAX0;
        bytes = readBytes(data, 7);
        if (bytes == 7) {
            testx = (data[2]<<8)|data[1];
            testy = (data[4]<<8)|data[3];
            testz = (data[6]<<8)|data[5];
            
//             if((abs(testx-lx)>30) || (abs(testy-ly)>30) || (abs(testz-lz)>30)) {
                printf("new\nx=%d y=%d z=%d\n", testx, testy, testz);
//                 lx = testx;
//                 ly = testy;
//                 lz = testz;
//             }
        }
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
        printf("data[0]: %x \ndata[1]: %x \ndata[2]: %x \ndata[3]: %x \ndata[4]: %x \ndata[5]: %x \ndata[6]: %x \n",
 data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
        
        
        printf("testx = %.3f, testy = %.3f, testz = %.3f\n",
 x * accConversion, y * accConversion, z * accConversion);
        rawx = ((data[2] & 0x0F)<<8)|data[1];
        rawy = ((data[4] & 0x0F)<<8)|data[3];
        rawz = ((data[6] & 0x0F)<<8)|data[5];
        signx = data[2] >> 7;
        signy = data[4] >> 7;
        signz = data[6] >> 7;
        t = gettime();
        accelx = (rawx/256.0) - 16*signx;
        accely = (rawy/256.0) - 16*signy;
        accelz = (rawz/256.0) - 16*signz;
        
        printf("rawx = %x rawy = %x rawz = %x\n", rawx, rawy, rawz);
        printf("time: %.3f, x = %.9f, y = %.9f, z = %.9f\n\n", t, accelx, accely, accelz);
        delayMillis(200);
    }
    
    
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
    printf("%d samples read in %.3f seconds with %.1f Hz\n", samples, t-tstart, samples/(t-tstart));
    return 0;
}

double gettime(void) {
    struct timeval tv;
    double t;

    gettimeofday(&tv, 0);
    t = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);
    return t;
}
