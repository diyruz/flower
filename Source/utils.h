#ifndef UTILS_H
#define UTILS_H
extern double mapRange(double a1, double a2, double b1, double b2, double s);

extern uint16 adcReadSampled(uint8 channel, uint8 resolution, uint8 reference, uint8 samplesCount);

#endif