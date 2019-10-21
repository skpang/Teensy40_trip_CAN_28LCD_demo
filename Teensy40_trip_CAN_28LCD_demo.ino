
/*
 * Teensy 4.0 Triple CAN Demo with 2.8" LCD
 * 
 * For use with:
 * http://skpang.co.uk/catalog/teensy-40-triple-can-board-with-teensy-40-and-28-lcd-p-1576.html
 * 
 * can1 and can2 are CAN2.0B
 * can3 is CAN FD
 * 
 * Ensure FlexCAN_T4 is installed first
 * https://github.com/tonton81/FlexCAN_T4
 * 
 * www.skpang.co.uk October 2019 
 * 
 */

#include <FlexCAN_T4.h>

#include "SPI.h"
#include "ILI9341_t3.h"
#include "font_Arial.h"
#include "font_LiberationMono.h"
#include "font_CourierNew.h"
#include "font_LiberationSansBold.h"

FlexCAN_T4FD<CAN3, RX_SIZE_256, TX_SIZE_16> FD;  // can3 port
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;  // can2 port
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;  // can1 port 

#define TFT_DC  9
#define TFT_CS 10

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
IntervalTimer timer;

int led = 13;
uint8_t d=0;

void setup() 
{
  pinMode(led, OUTPUT);   
  digitalWrite(led,HIGH);
  Serial.begin(115200); delay(1000);
  Serial.println("Teensy 4.0 Triple CAN test");
  digitalWrite(led,LOW);

  FD.begin();
  CANFD_timings_t config;
  config.clock = CLK_24MHz;
  config.baudrate =    500000;       // 500kbps arbitration rate
  config.baudrateFD = 2000000;      // 2000kbps data rate
  config.propdelay = 190;
  config.bus_length = 1;
  config.sample = 75;
  FD.setRegions(64);
  FD.setBaudRate(config, 1, 1);
  FD.onReceive(canSniff);
  FD.setMBFilter(ACCEPT_ALL);
  FD.setMBFilter(MB13, 0x1);
  FD.setMBFilter(MB12, 0x1, 0x3);
  FD.setMBFilterRange(MB8, 0x1, 0x04);
  FD.enableMBInterrupt(MB8);
  FD.enableMBInterrupt(MB12);
  FD.enableMBInterrupt(MB13);
  FD.enhanceFilter(MB8);
  FD.enhanceFilter(MB10);
  FD.distribute();
  FD.mailboxStatus();
  
  can2.begin();
  can2.setBaudRate(500000);       // 500kbps data rate
  can2.enableFIFO();
  can2.enableFIFOInterrupt();
  can2.onReceive(FIFO, canSniff20);
  can2.mailboxStatus();
  
  can1.begin();
  can1.setBaudRate(500000);     // 500kbps data rate
  can1.enableFIFO();
  can1.enableFIFOInterrupt();
  can1.onReceive(FIFO, canSniff20);
  can1.mailboxStatus();
  
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setFont(LiberationMono_10);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.println("Teensy 4.0 CAN-Bus Viewer");
  tft.println("SK Pang 2019");
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.setCursor(0, 50);
  
  timer.begin(sendframe, 500000); // Send frame every 500ms 
}  
 
/* From Timer Interrupt */
void sendframe()
{
  
  CAN_message_t msg2;
  msg2.id = 0x401;
  
  for ( uint8_t i = 0; i < 8; i++ ) {
    msg2.buf[i] = i + 1;
  }
  
  msg2.buf[0] = d++;
  msg2.seq = 1;
  can2.write(MB15, msg2); // write to can2

  msg2.id = 0x402;
  msg2.buf[1] = d++;
  can1.write(msg2);       // write to can1
  
  CANFD_message_t msg;
  msg.len = 64;
  msg.id = 0x4fd;
  msg.seq = 1;
  for ( uint8_t i = 0; i < 64; i++ ) {
    msg.buf[i] = i + 1;
  }
  msg.buf[0] = d;
  FD.write( msg);         // write to can3

}

void canSniff(const CANFD_message_t &msg) {
  Serial.print("ISR - MB "); Serial.print(msg.mb);
  Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
  Serial.print("  LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  Serial.print(" ID: "); Serial.print(msg.id, HEX);
  Serial.print(" Buffer: ");
  for ( uint8_t i = 0; i < msg.len; i++ ) {
    Serial.print(msg.buf[i], HEX); Serial.print(" ");
  } Serial.println();
}

void canSniff20(const CAN_message_t &msg) 
{ 
  Serial.print("MB "); Serial.print(msg.mb);
  Serial.print("  LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  Serial.print(" ID: "); Serial.print(msg.id, HEX);
  Serial.print(" Buffer: ");
  for ( uint8_t i = 0; i < msg.len; i++ ) {
    Serial.print(msg.buf[i], HEX); Serial.print(" ");
  } Serial.println();

  String CANStr("");
  CANStr += String(msg.id,HEX); 
  CANStr += (" ") ;
  CANStr += String(msg.len,HEX); 
  CANStr += (" ") ;
     
  for (int i=0; i <msg.len; i++) 
  {     
      if(msg.buf[i] < 16)
         {
          CANStr += ("0") ;
         } 
         CANStr += String(msg.buf[i],HEX).toUpperCase();
         CANStr += (" ") ;
  }
  
  tft.println(CANStr);    // Print data to LCD
  
}

void loop() 
{
 
  CANFD_message_t msg;
  can2.events();
  can1.events();

  FD.events(); /* needed for sequential frame transmit and callback queues */
  if (  FD.readMB(msg) ) {
      Serial.print("MB: "); Serial.print(msg.mb);
      Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
      Serial.print("  ID: 0x"); Serial.print(msg.id, HEX );
      Serial.print("  EXT: "); Serial.print(msg.flags.extended );
      Serial.print("  LEN: "); Serial.print(msg.len);
      Serial.print("  BRS: "); Serial.print(msg.brs);
      Serial.print("  EDL: "); Serial.print(msg.edl);
      Serial.print(" DATA: ");
      for ( uint8_t i = 0; i <msg.len ; i++ ) {
        Serial.print(msg.buf[i]); Serial.print(" ");
      }
      Serial.print("  TS: "); Serial.println(msg.timestamp);
 
      String CANStr("");
      CANStr += String(msg.id,HEX); 
      CANStr += (" ") ;
      CANStr += String(msg.len,HEX); 
      CANStr += (" ") ;
     
      for (int i=0; i <msg.len; i++) 
      {     
        if(msg.buf[i] < 16)
         {
          CANStr += ("0") ;
         } 
         CANStr += String(msg.buf[i],HEX).toUpperCase();
         CANStr += (" ") ;
      }
  
      tft.println(CANStr);    // Print data to LCD
   }

}
