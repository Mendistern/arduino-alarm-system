#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <ledfunctions.h>
#include <usart.h>
#include <util/delay.h>
#include <potentio.h>
#include <display.h>

#include <extra.h>


int main(void){
  initUSART();

  printf("hello world");

  return 0;
}