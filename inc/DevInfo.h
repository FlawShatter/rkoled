// DevInfo.h

#ifndef DEVINFO_H_
#define DEVINFO_H_

typedef struct {
    char name[32];
    unsigned int user;
    unsigned int nice;
    unsigned int system;
    unsigned int idle;
} CPU_OCCUPY;

float GetCpuUsage();
float GetCpuTemp();
int GetMemUsage();
float GetDiskUsagePercentage();

#endif /* DEVINFO_H_ */
