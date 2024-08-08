#include <stdint.h>
#include "hardware.h"
#include "rtc.h"
#include "release.h"

uint8_t rtc_rtcdata[8];
uint8_t rtc_rtcstat=0; //b0=RTC avail b1=RTC write

void rtc_rtcbuild() {
  rtc_rtcdata[D_RTCYLO]=(SYMBOSVM_BUILD_DY)&0xff;
  rtc_rtcdata[D_RTCYHI]=(SYMBOSVM_BUILD_DY)>>8;
  rtc_rtcdata[D_RTCMON]=SYMBOSVM_BUILD_DM;
  rtc_rtcdata[D_RTCDAY]=SYMBOSVM_BUILD_DD;
  rtc_rtcdata[D_RTCHOU]=SYMBOSVM_BUILD_TH;
  rtc_rtcdata[D_RTCMIN]=SYMBOSVM_BUILD_TM;
  rtc_rtcdata[D_RTCSEC]=SYMBOSVM_BUILD_TS;
};

/* vim: set et ts=2 sw=2 :*/
