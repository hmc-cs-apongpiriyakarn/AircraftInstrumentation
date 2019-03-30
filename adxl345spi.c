// apongpiriyakarn@hmc.edu
// 3/4/19

#include "EasyPIO.h"
#include <time.h>
#include <sys/time.h>

#define BW_RATE_ADR             0x2C
#define DATA_FORMAT_ADR         0x31 // register address
#define DATA_FORMAT_BYTES_ADR   0x0B // +/-16G range, 13-bit res (p26)
#define POWER_CONTROL_ADR       0x2D
#define DATAX0_ADR              0x32
#define FIFO_CTL_ADR            0x38 

double gettime();

int main() {

    pioInit();
    // timerInit();

    // set channel, speed
    spiInit(244000, 0);


    char data[7],spisend;
    short send, rawx, rawy, rawz;
    int samples = 10; // change later
    short x,y,z;
    double t;

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
    data[0] = 0xC0;
    data[1] = 0x00;
    //send = (data[0] << 8) | data[1];
    //spiSendReceive16(send);
    spiSendReceive(data[0]);
    spiSendReceive(data[1]);

    // Configure outout data rate, clock is 1MHz
    data[0] = BW_RATE_ADR;              // 0x2C
    data[1] = 0x0F;                     // > 800 Hz
//     send = (data[0] << 8) | data[1];
//     spiSendReceive16(send);
    spiSendReceive(data[0]);
    spiSendReceive(data[1]);

    // Set to full resolution(res increases with g range)
    data[0] = DATA_FORMAT_ADR;          // 0x31
    data[1] = DATA_FORMAT_BYTES_ADR;    // FULL_RES bit(bit 3), +/-16 G
//     send = (data[0] << 8) | data[1];
//     spiSendReceive16(send);
    spiSendReceive(data[0]);
    spiSendReceive(data[1]);

    // Set the wake up bit
    data[0] = POWER_CONTROL_ADR;        // 0x2D
    data[1] = 0x08;                     // bit 3 is wake up bit
//     send = (data[0] << 8) | data[1];
//     spiSendReceive16(send);
    spiSendReceive(data[0]);
    spiSendReceive(data[1]);

    // Bypass FIFO mode
//     data[0] = FIFO_CTL_ADR;             // 0x38
//     data[1] = 0x00;                     // disable everything FIFO
// //     send = (data[0] << 8) | data[1];
// //     spiSendReceive16(send);
//     spiSendReceive(data[0]);
//     spiSendReceive(data[1]);

    samples=10;
    tstart = gettime();
    for(int j=0; j<samples;j++) {
        spisend = DATAX0_ADR;
        for(int i = 0; i < 7; i++)
            data[i] = spiSendReceive(spisend);
        rawx = (data[2]<<8)|data[1];
        rawy = (data[4]<<8)|data[3];
        rawz = (data[6]<<8)|data[5];
        t = gettime();
        printf("time: %.3f, x = %.3f, y = %.3f, z = %.3f\n", t, x*32.0/8192.0, y*32.0/8192.0, z*32.0/8192.0);
        delayMillis(50);
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
