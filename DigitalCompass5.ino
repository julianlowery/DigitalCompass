#include <Wire.h>

// device address
#define DEVICE_ADDRESS B0001101

// define pi
#define M_PI 3.14159265359

// register numbers from data sheet
// two bytes of data for each axis (stored in 2 regisers)
#define X_LSB 0
#define X_MSB 1
#define Y_LSB 2
#define Y_MSB 3
#define Z_LSB 4
#define Z_MSB 5
#define STATUS 6
#define TEMPERATURE_LSB 7
#define TEMPERATURE_MSB 8
#define CONFIG 9
#define CONFIG2 10
#define RESET 11
#define RESERVED 12
#define CHIP_ID 13

//the 16 pins for led matrix stored in arrays
int rows[8] = {2, 14, 4, 15, 9, 5, 11, 6};
int cols[8] = {10, 0, 1, 7, 3, 8, 12, 13};

// 2D array to store the state of LED matrix
int pixels[8][8];

// low and high values used for compass calibration
int x_low, x_high, y_low, y_high = 0;

void setup()
{
  // all LED matrix pins set to output
  for (int i = 0; i < 16; i++)
    pinMode(i, OUTPUT);

  // set all collumn pins (cathodes) to HIGH to ensure LEDs are off
  for (int i = 0; i < 8; i++)
  {
    digitalWrite(cols[i], HIGH);
  }
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++)
      pixels[y][x] = HIGH;
  }

  // Set centre LEDs on permanently
  pixels[3][3] = LOW;
  pixels[3][4] = LOW;
  pixels[4][3] = LOW;
  pixels[4][4] = LOW;
}


// ----------------------------------------------------LED Matrix Functions----------------------------------------------------

void refreshScreen()
{
  // loop over the rows
  for (int thisRow = 0; thisRow < 8; thisRow++) {
    // set the row (anodes) we are at from low to high
    digitalWrite(rows[thisRow], HIGH);
    // loop over the column places of the row we are at
    for (int thisCol = 0; thisCol < 8; thisCol++) {
      // save the state of the pixel we are at (from the array of pixels) in a variable
      int pixelState = pixels[thisRow][thisCol];
      // set the column to either low or high depending on the pixel state (if set to low, pixel will light up)
      digitalWrite(cols[thisCol], pixelState);
      // if the pixel was turned on, turn it off again
      if (pixelState == LOW)
        digitalWrite(cols[thisCol], HIGH);
    }
    // set the row back to low
    digitalWrite(rows[thisRow], LOW);
  }
}

void clearScreen()
{
  // loop through all x and y values of pixels array and set to high
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      pixels[y][x] = HIGH;
    }
  }
  // apply to screen
  refreshScreen();
}
void fillScreen()
{
  // loop through all x and y values of pixels array and set to low
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      pixels[y][x] = LOW;
      int count = 0;
      while (count < 30) {
        refreshScreen();
        count ++;
      }
    }
  }
  delayScreen(300);
}

void delayScreen(int counts)
{
  int count = 0;
  while (count < counts) {
    refreshScreen();
    count ++;
  }
}

void refreshPixels()
{
  // loop through all values of pixels and set to high
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      pixels[y][x] = HIGH;
    }
  }
}




// -------------------------------------------------------------- I2C FUNCTIONS -------------------------------------------------------------

// function to check if specified number of bytes are ready to be read a from a specified register
// BE CAREFUL WITH THE BYTES... SIZE MUST MATCH
int register_select(int address, int reg, int bytes)
{
  // begin transmission at device
  Wire.beginTransmission(address);
  // queue bytes for transmission to the slave (writing desired register)
  Wire.write(reg);
  // end transmission with the slave and and transmit the bytes queued in write()
  Wire.endTransmission();

  // request bytes from the slave
  Wire.requestFrom(address, bytes);
  // check how many bytes are available for retrieval
  int bytes_available = Wire.available();
  // check if desired amount of bytes are available, if not return 0
  if (bytes_available != bytes)
    return 0;
  return bytes_available;
}

// function to check if heading data is ready to be read
int read_heading_data_ready()
{
  // request from the status register and check if data is available
  if (!register_select(DEVICE_ADDRESS, STATUS, 1))
    return 0;
  // read data from status register
  uint8_t stat = Wire.read();
  // return only the DRDY bit at bit 0 (LSB) (DRDY is Data Ready, 0: no new data, 1: new data is ready)
  return stat & 1;
}

// function to read heading data
int read_heading_data(int &x, int &y, int &z)
{
  // wait until 3 axes registers have all data available
  while(!read_heading_data_ready())
    {}
  // 
  if (!register_select(DEVICE_ADDRESS, X_LSB, 6))
    return 0;
  // read data from registers, shift
  x = Wire.read() | Wire.read() << 8;
  y = Wire.read() | Wire.read() << 8;
  z = Wire.read() | Wire.read() << 8;
  return 1;
}

int get_heading()
{
  // create variables to hold raw x, y, z data
  int16_t x, y, z;

  // attempt to read data, if not, return 0
  if (!read_heading_data(x, y, z))
    return 0;

  // update callibration
  if(x < x_low)
    x_low = x;
  if(x > x_high)
    x_high = x;
  if(y < y_low)
    y_low = y;
  if(y > y_high)
    y_high = y;

  // do not calculate if there is not enough data yet
  if(x_low == x_high || y_low == y_high)
    return 0;

// recenter the data by subtracting the average
  x -= (x_high - x_low) / 2;
  y -= (y_high - y_low) / 2;

// rescale the data 
  float fx = (float)x / (x_high - x_low);
  float fy = (float)y / (y_high - y_low);

  int heading = atan2(y, x)*180/M_PI;
  if(heading <= 0)
    heading += 360;

  return heading;
}



void loop()
{
  int heading = get_heading();

  int LEDheading = map(heading, 1, 360, 1, 28);

  switch (LEDheading)
  {
    case 1:
      pixels[0][3] = LOW;
      delayScreen(200);
      pixels[0][3] = HIGH;
      break;
    case 2:
      pixels[0][4] = LOW;
      delayScreen(200);
      pixels[0][4] = HIGH;
      break;
    case 3:
      pixels[0][5] = LOW;
      delayScreen(200);
      pixels[0][5] = HIGH;
      break;
    case 4:
      pixels[0][6] = LOW;
      delayScreen(200);
      pixels[0][6] = HIGH;
      break;
    case 5:
      pixels[0][7] = LOW;
      delayScreen(200);
      pixels[0][7] = HIGH;
      break;
    case 6:
      pixels[1][7] = LOW;
      delayScreen(200);
      pixels[1][7] = HIGH;
      break;
    case 7:
      pixels[2][7] = LOW;
      delayScreen(200);
      pixels[2][7] = HIGH;
      break;
    case 8:
      pixels[3][7] = LOW;
      delayScreen(200);
      pixels[3][7] = HIGH;
      break;
    case 9:
      pixels[4][7] = LOW;
      delayScreen(200);
      pixels[4][7] = HIGH;
      break;
    case 10:
      pixels[5][7] = LOW;
      delayScreen(200);
      pixels[5][7] = HIGH;
      break;
    case 11:
      pixels[6][7] = LOW;
      delayScreen(200);
      pixels[6][7] = HIGH;
      break;
    case 12:
      pixels[7][7] = LOW;
      delayScreen(200);
      pixels[7][7] = HIGH;
      break;
    case 13:
      pixels[7][6] = LOW;
      delayScreen(200);
      pixels[7][6] = HIGH;
      break;
    case 14:
      pixels[7][5] = LOW;
      delayScreen(200);
      pixels[7][5] = HIGH;
      break;
    case 15:
      pixels[7][4] = LOW;
      delayScreen(200);
      pixels[7][4] = HIGH;
      break;
    case 16:
      pixels[7][3] = LOW;
      delayScreen(200);
      pixels[7][3] = HIGH;
      break;
    case 17:
      pixels[7][2] = LOW;
      delayScreen(200);
      pixels[7][2] = HIGH;
      break;
    case 18:
      pixels[7][1] = LOW;
      delayScreen(200);
      pixels[7][1] = HIGH;
      break;
    case 19:
      pixels[7][0] = LOW;
      delayScreen(200);
      pixels[7][0] = HIGH;
      break;
    case 20:
      pixels[6][0] = LOW;
      delayScreen(200);
      pixels[6][0] = HIGH;
      break;
    case 21:
      pixels[5][0] = LOW;
      delayScreen(200);
      pixels[5][0] = HIGH;
      break;
    case 22:
      pixels[4][0] = LOW;
      delayScreen(200);
      pixels[4][0] = HIGH;
      break;
    case 23:
      pixels[3][0] = LOW;
      delayScreen(200);
      pixels[3][0] = HIGH;
      break;
    case 24:
      pixels[2][0] = LOW;
      delayScreen(200);
      pixels[2][0] = HIGH;
      break;
    case 25:
      pixels[1][0] = LOW;
      delayScreen(200);
      pixels[1][0] = HIGH;
      break;
    case 26:
      pixels[0][0] = LOW;
      delayScreen(200);
      pixels[0][0] = HIGH;
      break;
    case 27:
      pixels[0][1] = LOW;
      delayScreen(200);
      pixels[0][1] = HIGH;
      break;
    case 28:
      pixels[0][2] = LOW;
      delayScreen(200);
      pixels[0][2] = HIGH;
      break;
    default:
      clearScreen();
      break;
  }
}








