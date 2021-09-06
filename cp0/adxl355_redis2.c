#include "adxl355.h"
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <hiredis.h>

#define QUEUE_MAX 1024

int main(int argc, char const *argv[]) {
  /* code */
  uint16_t clk_div = BCM2835_I2C_CLOCK_DIVIDER_148  ;
  uint8_t slave_address = 0x53;

  int axisX;
  int axisY;
  int axisZ;
  clock_t time;
  FILE *fp;
  redisContext *conn = NULL;
  redisReply *reply   = NULL;
  char    str[100];

  //if ((fp = fopen("data.txt", "w")) == NULL) {
  //    return -1;
  //}

  // 接続
  conn = redisConnect( "localhost" ,  // 接続先redisサーバ
                       6379           // ポート番号
         );
  if( ( NULL != conn ) && conn->err ){
    // error
    printf("error : %s\n" , conn->errstr );
    redisFree( conn );
    exit(-1);
  }else if( NULL == conn ){
    exit(-1);
  }

  if (!bcm2835_init())
  {
    printf("bcm2835_init failed.\n");
    return 1;
  }

  if (!bcm2835_i2c_begin())
  {
    printf("bcm2835_i2c_begin failed.\n");
    return 1;
  }
  bcm2835_i2c_setSlaveAddress(slave_address);
  bcm2835_i2c_setClockDivider(clk_div);
  //bcm2835_i2c_set_baudrate(BAUDRATE);
//  if( AXDXL355_isRunning()){
//      AXDXL355_end();
//  }
  ADXL355_begin();
  ADXL355_setRange(RANGE_4G);
  ADXL355_setLowpassFilter(LOWPASS_FILTER_4000);

  while (1) {
    reply  = (redisReply *)redisCommand( conn , "LLEN fft");
    if( NULL == reply ){
    // error
      redisFree( conn );
      exit(-1);
    }
    if(reply->integer == 0){
      //printf( " %d\n" , reply->integer );
      freeReplyObject(reply);
      break;
    }
    freeReplyObject(reply);
    usleep(1000);
  }
//  printf("Current temperature is %fC",AXDXL355_getTemperature());
//  printf("");
//  printf("Press Ctrl-C to exit");
int t_old = 0;
int count = 0;
while (1) {
  while(1){
          int t = (clock() / 100) % 10;
          if(((t == 0) || (t == 2) || (t == 4) || (t == 6) || (t == 8)) && t != t_old) {
          //if(((t == 0) || (t == 5)) && t != t_old) {
              t_old = t;
              break;
          }
          //usleep(60);
      }
    time = clock();
    ADXL355_getAxes(&axisX,&axisY,&axisZ);
    //printf("%d,%d,%d,%d\n",time,axisX,axisY,axisZ);
    //sprintf(str,"%d,%d,%d,%d",time,axisX,axisY,axisZ);
    //fprintf(fp,"%d,%d,%d,%d\n",time,axisX,axisY,axisZ);
    reply  = (redisReply *)redisCommand( conn , "RPUSH fft %d,%d,%d,%d",time,axisX,axisY,axisZ);
    if( NULL == reply ){
      // error
      redisFree( conn );
      exit(-1);
    }
    freeReplyObject(reply);
    if(count++>=QUEUE_MAX){
      break;
    }
  }

  ADXL355_end();
  bcm2835_i2c_end();
  bcm2835_close();
  // 後片づけ
  redisFree( conn );

  return 0;
}
