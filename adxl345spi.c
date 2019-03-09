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

double gettime();

int main() {

    pioInit();
    // timerInit();

    // set channel, speed
    spiInit(244000, 0);


    char data[7];
    short send;
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

    data[0] = BW_RATE_ADR;
    data[1] = 0x0F;
    send = (data[0] << 8) | data[1];
    spiSendReceive16(send);

    data[0] = DATA_FORMAT_ADR;
    data[1] = DATA_FORMAT_BYTES_ADR; //fix
    send = (data[0] << 8) | data[1];
    spiSendReceive16(send);

    data[0] = POWER_CONTROL_ADR;
    data[1] = 0x08;
    send = (data[0] << 8) | data[1];
    spiSendReceive16(send);

    for(int i = 0; i < samples; i++) {
        data[0] = DATAX0_ADR;

        for(int j = 0; j < 7; j++)
            data[j] = spiSendReceive(data[j]);
        x = (data[2] << 8) | data[1];
        y = (data[4] << 8) | data[3];
        z = (data[6] << 8) | data[5];
        t = gettime();
        printf("time: %.3f, x = %.3f, y = %.3f, z = %.3f\n", t, x*32.0/8192.0, y*32.0/8192.0, z*32.0/8192.0);
        delayMillis(100);
    }
    printf("%d samples read in %.3f seconds with %.1f Hz\n", samples, t, samples/t);
    return 0;
}

double gettime(void) {
    struct timeval tv;
    double t;

    gettimeofday(&tv, 0);
    t = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);
    return t;
}
