
#define WII_IDENT_LEN ((byte)6)
#define WII_TELEGRAM_LEN ((byte)6)
#define WII_NUNCHUCK_TWI_ADR ((byte)0x52)

#include <Wire.h>
#include <string.h>

uint8_t outbuf[WII_TELEGRAM_LEN];		// array to store arduino output
int cnt = 0;
  int analogMagnitude;
  float analogAngle;
uint8_t nunchuck_joy_x, nunchuck_joy_y;
int  nunchuck_c_button, nunchuck_z_button, nunchuck_accel_x, nunchuck_accel_y, nunchuck_accel_z;
uint8_t error_cycles = 0;

static byte readControllerIdent(byte* pIdent)
{
  static byte rc = 1;

  // read identification
  Wire.beginTransmission (WII_NUNCHUCK_TWI_ADR);	// transmit to device 0x52
  Wire.write (0xFA);		// sends memory address of ident in controller
  if (Wire.endTransmission () == 0)	// stop transmitting
  {
    static byte i;
    Wire.requestFrom (WII_NUNCHUCK_TWI_ADR, WII_TELEGRAM_LEN);	// request data from nunchuck
    for (i = 0; (i < WII_TELEGRAM_LEN) && Wire.available (); i++)     pIdent[i] = Wire.read();	// receive byte as an integer
    if (i == WII_TELEGRAM_LEN)     rc = 0;
  }
  return rc;
}

static byte nunchuck_init (unsigned short timeout)
{
  static byte rc = 1;
  unsigned long time = millis();
  do
  {
    Wire.beginTransmission (WII_NUNCHUCK_TWI_ADR);	// transmit to device 0x52
    Wire.write (0xF0);		// sends memory address Snakebyte Wireless Motion XS controller
    Wire.write (0x55);		// sends data.
    Serial.println("Handshake 1/2....");
    if (Wire.endTransmission() == 0) { // stop transmitting
      Wire.beginTransmission (WII_NUNCHUCK_TWI_ADR);	// transmit to device 0x52
      Wire.write (0xA5);		// sends memory address for Snakebyte Wireless Motion XS controller
      Wire.write (0x00);		// sends sent a zero.
      Serial.println("Handshake 2/2....");
      if (Wire.endTransmission () == 0) {
        rc = 0;
      }
    }
  }
  while (rc != 0 && (!timeout || ((millis() - time) < timeout)));
  Serial.println("OK.");
  return rc;
}


static void nunchuck_pre_init()
{
  Wire.begin(); // initialize i2c
  // we need to switch the TWI speed, because the nunchuck uses Fast-TWI
  // normally set in hardware\libraries\Wire\utility\twi.c twi_init()
  // this is the way of doing it without modifying the original files
#define TWI_FREQ_NUNCHUCK 400000L
  TWBR = ((CPU_FREQ / TWI_FREQ_NUNCHUCK) - 16) / 2;

  nunchuck_init(0); // send the initialization handshake

}


static void nunchuck_get_data()
{

  for (byte i = 0; i < 3; i++)
  {
    Wire.beginTransmission (WII_NUNCHUCK_TWI_ADR);  // transmit to device 0x52
    Wire.write (0x00);    // sends one byte
    Wire.endTransmission ();  // stop transmitting
    delay(5);
  }
  uint8_t errors = 0;
  Wire.requestFrom (WII_NUNCHUCK_TWI_ADR, WII_TELEGRAM_LEN);	// request data from nunchuck

  for (cnt = 0; (cnt < WII_TELEGRAM_LEN) && Wire.available (); cnt++)
  {
    outbuf[cnt] = Wire.read();
    if (outbuf[cnt] == 0xFF || outbuf[cnt] == 0x00) errors++;
  }

  if (errors >= 6)    error_cycles++;
  else    error_cycles = 0;


  if (error_cycles >= 12)   nunchuck_init(0);

  // clear the receive buffer from any partial data
  while ( Wire.available ())  Wire.read ();

  // If we recieved the 6 bytes, then go print them
  if (cnt >= WII_TELEGRAM_LEN)
  {
    nunchuck_joy_x = outbuf[0];
    nunchuck_joy_y = outbuf[1];
    nunchuck_accel_x = (outbuf[2] << 2) | ((outbuf[5] >> 2) & 3);
    nunchuck_accel_y = (outbuf[3] << 2) | ((outbuf[5] >> 4) & 3);
    nunchuck_accel_z = (outbuf[4] << 2) | ((outbuf[5] >> 6) & 3);
    nunchuck_z_button = !((outbuf[5] >> 0) & 1);
    nunchuck_c_button = !((outbuf[5] >> 1) & 1);
    analogMagnitude = sqrt(pow((127 - nunchuck_joy_x), 2) + pow((127 - nunchuck_joy_y), 2));
    analogAngle = (atan2((int)127 - (int)nunchuck_joy_y, (int)127 - (int)nunchuck_joy_x) * 57.2957);


  }

}
