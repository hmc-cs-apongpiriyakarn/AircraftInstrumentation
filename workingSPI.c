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

double gettime();
int spiSendReceiveBytes(char *data, int count);
void spiSend(char *data, int count);
int readBytes(int handle, char *data, int count);

char data[7];

int main() {
    pioInit();
    spiInit(244000, 0);
    
    int samples = 10;
    int h, bytes;
    int16_t x, y, z;
    int speedSPI = 2000000;
    double tStart, tDuration, t;
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
    
    tStart = time_time();
    for (int i = 0; i < samples; i++) {
        data[0] = DATAX0;
        bytes = readBytes(h, data, 7);
        //bytes = spiSendReceiveBytes(data, 7);
        printf("data[0]: %x \nx0: %x x1: %x \ny0: %x y1: %x \nz0: %x z1: %x \n",
            data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
        if (bytes == 7) {
            x = (data[2]<<8)|data[1];
            y = (data[4]<<8)|data[3];
            z = (data[6]<<8)|data[5];
            t = time_time() - tStart;
            printf("time = %.3f, x = %.3f, y = %.3f, z = %.3f\n",
                   t, x*2*16.0/8192.0, y*2*16.0/8192.0, z*2*16.0/8192.0);
        }
        delayMillis(200);
    }
    tDuration = time_time() - tStart; 
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
