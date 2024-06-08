#include <stdio.h>
#include <stdlib.h>
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <string.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "SSD1306_IIC.h"
#include "DevInfo.h"
#include "NetTools.h"
#include "UserCfg.h"

#define TEMP_STR_LEN    64
#define IP_STR_LEN      20
#define TO_GB(b)        ((b) / pow(1024, 3))
#define TO_SEC(h, m)    ((h) * 3600 + (m) * 60)

extern uint8_t WIFI_CONNECT[32];
extern uint8_t ETH_CONNECT[32];
extern uint8_t NET_ERROR[32];

time_t timep;
struct tm* myTm;
uint8_t inited = 1;

#if ENABLE_RUNNING_PERIOD
int BEG_SEC = TO_SEC(BEG_H, BEG_M);
int END_SEC = TO_SEC(END_H, END_M);
int CUR_SEC = 0;
#endif // ENABLE_RUNNING_PERIOD

long int start_rcv_rates = 0;   //保存开始时的流量计数
long int end_rcv_rates = 0;	    //保存结束时的流量计数
long int start_tx_rates = 0;    //保存开始时的流量计数
long int end_tx_rates = 0;
char ipStr[IP_STR_LEN];
char tempStr[TEMP_STR_LEN];
float tx_rates = 0;
float rx_rates = 0;
float cpuUsage;
CPU_OCCUPY cpu_stat1;
CPU_OCCUPY cpu_stat2;
Net_State wlanState;
Net_State ethState;
char netSpeedUnit[4][5] = {
    "B/s",
    "KB/s",
    "MB/s",
    "GB/s",
};

void Work();

int main(int argc, char* argv[])
{
    SSD1306_Init();

    if (!(argc >= 2 && !strcmp(argv[1], "-r")))
    {
        SSD1306_FillRect2(2, 0, 5, 128, White);
        SSD1306_PutString(21, 2, "Panther X2", MF_6x8, White);
        SSD1306_PutString(15, 16, "Husky", MF_16x26, White);
        SSD1306_PutString(90, 25, "", MF_11x18, White);
        SSD1306_PutString(15, 52, "HomeAssistant", MF_6x8, White);
        SSD1306_UpdateScreen();
        sleep(3);
    }

    while (1)
    {
#if ENABLE_RUNNING_PERIOD
        time(&timep);
        myTm = localtime(&timep);
        CUR_SEC = TO_SEC(myTm->tm_hour, myTm->tm_min);

        if (BEG_SEC <= CUR_SEC && CUR_SEC <= END_SEC)
        {
            inited = 1;
        }
        else if (inited)
        {
            inited = 0;
            SSD1306_ClearScreen();
            SSD1306_UpdateScreen();
        }

        if (inited)
        {
#endif // ENABLE_RUNNING_PERIOD
            Work();
            GetCurNetFlow(wlanState == STATE_CONNECT ? WLAN_IF : (ethState == STATE_CONNECT ? ETH_IF : NULL),
                          &start_rcv_rates, &start_tx_rates);
            get_cpuoccupy((CPU_OCCUPY *)&cpu_stat1);
            sleep(REFRESH_TIME);
            get_cpuoccupy((CPU_OCCUPY *)&cpu_stat2);
            GetCurNetFlow(wlanState == STATE_CONNECT ? WLAN_IF : (ethState == STATE_CONNECT ? ETH_IF : NULL),
                          &end_rcv_rates, &end_tx_rates);

            cpuUsage = cal_cpuoccupy((CPU_OCCUPY *)&cpu_stat1, (CPU_OCCUPY *)&cpu_stat2);
            rx_rates = (float)(end_rcv_rates - start_rcv_rates) / REFRESH_TIME;
            tx_rates = (float)(end_tx_rates - start_tx_rates) / REFRESH_TIME;

#if ENABLE_RUNNING_PERIOD
        }
        else
        {
            sleep(10);
        }
#endif // ENABLE_RUNNING_PERIOD

    }
}

void Work()
{
    SSD1306_ClearScreen();

    SSD1306_DrawLine(0, 0, 127, 0, White);
    SSD1306_DrawLine(0, 15, 127, 15, White);
    SSD1306_DrawLine(82, 15, 82, 38, White);
    SSD1306_DrawLine(0, 38, 127, 38, White);
    SSD1306_DrawLine(0, 63, 127, 63, White);
    SSD1306_DrawLine(37, 39, 37, 62, White);

    wlanState = GetWirelessState();
    ethState = GetEthernetState();

    if (wlanState == STATE_CONNECT)
    {
        SSD1306_DrawBitMap(0, 0, WIFI_CONNECT, 16, 16, White);
        memset(ipStr, 0, IP_STR_LEN);
        if (GetLocalIP(WLAN_IF, ipStr) != 0)
            memset(ipStr, 0, IP_STR_LEN);
        SSD1306_PutString(17, 4, ipStr, MF_7x10, White);
    }
    else if (ethState == STATE_CONNECT)
    {
        SSD1306_DrawBitMap(0, 0, ETH_CONNECT, 16, 16, White);
        memset(ipStr, 0, IP_STR_LEN);
        if (GetLocalIP(ETH_IF, ipStr) != 0)
            memset(ipStr, 0, IP_STR_LEN);
        SSD1306_PutString(17, 4, ipStr, MF_7x10, White);
    }
    else
    {
        SSD1306_DrawBitMap(0, 0, NET_ERROR, 16, 16, White);
    }

    memset(tempStr, 0, TEMP_STR_LEN);
    sprintf(tempStr, "CPU: %.1f%%", cpuUsage);
    SSD1306_PutString(0, 17, tempStr, MF_7x10, White);

    memset(tempStr, 0, TEMP_STR_LEN);
    sprintf(tempStr, "Mem: %d%%", GetMemUsage());
    SSD1306_PutString(0, 28, tempStr, MF_7x10, White);

    SSD1306_PutString(91, 17, "Temp", MF_7x10, White);
    sprintf(tempStr, "%.1fC", GetCpuTemp());
    SSD1306_PutString(88, 28, tempStr, MF_7x10, White);

    uint8_t rxUnitLevel = 0;
    uint8_t txUnitLevel = 0;
    while (tx_rates >= 1000)
    {
        tx_rates /= 1000;
        txUnitLevel++;
    }
    while (rx_rates >= 1000)
    {
        rx_rates /= 1000;
        rxUnitLevel++;
    }

    memset(tempStr, 0, TEMP_STR_LEN);
    sprintf(tempStr, " Up :%.1lf %s",
            tx_rates,
            netSpeedUnit[txUnitLevel]);
    SSD1306_PutString(40, 42, tempStr, MF_6x8, White);

    memset(tempStr, 0, TEMP_STR_LEN);
    sprintf(tempStr, "Down:%.1lf %s",
            rx_rates,
            netSpeedUnit[rxUnitLevel]);
    SSD1306_PutString(40, 52, tempStr, MF_6x8, White);

    SSD1306_PutString(7, 42, "Disk", MF_6x8, White);
    memset(tempStr, 0, TEMP_STR_LEN);
    sprintf(tempStr, "%.1f%%", GetDiskUsagePercentage());
    SSD1306_PutString(4, 52, tempStr, MF_6x8, White);

    SSD1306_UpdateScreen();
}
