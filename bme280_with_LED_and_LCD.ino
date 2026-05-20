/***************************************************************************
  This application uses: A BME280 temperature, humidity, pressure and altitude sensor,
  a Waveshare LCD 16x2 I2C and an LED.

    Features:
  - Real-time measurements (Temperature, Humidity, Pressure, Altitude)
  - Serial Plotter graphs
  - Mean temperature calculation (100 samples)
  - Min/Max tracking
  - LED warnings for high temperature/humidity
  - Comfort level indicator
  
  I2C address: 0x76 or 0x77
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Waveshare_LCD1602.h>  // Include the Waveshare LCD1602 library

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.3) //sea level pressure in Thessaloniki

Adafruit_BME280 bme; // I2C
Waveshare_LCD1602 lcd(16, 2); //LCD item,set up the LCD's number of columns and rows

unsigned long delayTime;
const int ledPin = 13;
const int temphigh = 27; //Όριο θερμοκρασίας
const int humhigh= 70; // Όριο υγρασίας
const int size=100;
int index;
float m[size];
float minTemp = 999.0;  //Αρχικοποίηση τιμών
float maxTemp = -999.0;


void setup() {
    lcd.init();  // Set up the LCD screen
    lcd.set_brightness(50); // Set the backlight brightness to 50%

    pinMode(ledPin, OUTPUT); //setting up the LED

    Serial.begin(9600);
    while(!Serial);    // time to get serial running
    Serial.println(F("BME280 Weather Station initiated"));
    lcd.setCursor(0,0);
    lcd.send_string("Weather Station ");
    lcd.setCursor(0,1);
    lcd.send_string("   initiated    ");
    delay(1000);

    unsigned status;
    status = bme.begin(0x76);  
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }

    Serial.println();
   
    init_m();   //μηδενισμός πίνακα init
    index=0;
    delayTime = 2000;
}


void loop() { 
    float temp = bme.readTemperature();
    float pres = bme.readPressure()/ 100.0F;
    float hum = bme.readHumidity();
    float alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
    
    m[index]=temp;
    float sum=0;
    float mean=0;

    for(int i=0; i<index; i++)
       sum+=m[i];

    mean=sum/index;

    index++;
    if (index>100) index=0;

    // Update min and max
    if (temp < minTemp) {
     minTemp = temp;
    }
    if (temp > maxTemp) {
     maxTemp = temp;
    }

    String comfort = getComfortLevel(temp, hum);

    //Γραφηκή παράσταση στο Serial Plotter
    Serial.print("Temperature(°C):"); Serial.print(temp, 1); Serial.print("\t"); //(" °C")
    //Serial.print("Max T(°C):"); Serial.print(maxTemp, 1); Serial.print("\t");
    //Serial.print("Min T(°C):"); Serial.print(minTemp, 1); Serial.print("\t");
    Serial.print("Mean_T:");  Serial.println(mean, 1); Serial.print("\t");
    Serial.print("Humidity(%):");  Serial.print(hum, 1); Serial.print("\t");
    Serial.print("Pressure(hPa):"); Serial.print(pres, 1); 
     
    delay(500);
 
    if (temp> temphigh) {
        digitalWrite(ledPin, HIGH); //warning message and LED
        lcd.setCursor(0,0);
        lcd.send_string("! TEMP WARNING !");
        lcd.setCursor(0,1);
        lcd.send_string("Temperature > ");
        lcd.send_string(String(temphigh).c_str());
        lcd.send_string(" C!");
        delay(delayTime);

    } else if(hum > humhigh) {
        digitalWrite(ledPin, LOW);

        lcd.setCursor(0,0);
        lcd.send_string("!HIGH HUMIDITY!");
        lcd.setCursor(0,1);
        lcd.send_string("Humidity > ");
        lcd.send_string(String(hum,1).c_str());
        lcd.send_string("%");
        delay(delayTime);
        
    } else {
        digitalWrite(ledPin, LOW);

        // Οθόνη 1: Θερμοκρασία + Υγρασία
        lcd.setCursor(0,0); //1st row temperature
        lcd.send_string("Temp:  ");
        lcd.send_string(String(temp, 1).c_str());
        lcd.send_string(" oC   ");

        lcd.setCursor(0,1); // 2nd row humidity
        lcd.send_string("Hum:  ");
        lcd.send_string(String(hum, 1).c_str());
        lcd.send_string(" %    ");
        delay(delayTime);

        // Οθόνη 2: Πίεση + Υψόμετρο
        lcd.setCursor(0,0); //1st row pressure
        lcd.send_string("Press: ");
        lcd.send_string(String(pres, 1).c_str());
        lcd.send_string("hPa");

        lcd.setCursor(0,1); // 2nd row altitude
        lcd.send_string("Alt:  ");
        lcd.send_string(String(alt, 1).c_str());
        lcd.send_string(" m   ");
        delay(delayTime);

        // Oθόνη 3: Μέση τιμή
        lcd.setCursor(0,0); //1st row mean
        lcd.send_string("Mean T: ");
        lcd.send_string(String(mean, 1).c_str());
        lcd.send_string("  oC    ");

        lcd.setCursor(0,1); // 2nd row altitude
        lcd.send_string("L: ");
        lcd.send_string(String(minTemp, 1).c_str());
        lcd.send_string("  ");
        lcd.send_string("H: ");
        lcd.send_string(String(maxTemp, 1).c_str());
        lcd.send_string("  ");
        delay(delayTime);

        // Screen 4: Comfort Level
        lcd.setCursor(0, 0);
        lcd.send_string("Comfort Level:  ");
        lcd.setCursor(0, 1);
        lcd.send_string(comfort.c_str());
        // Pad to clear previous text
        for(int i = comfort.length(); i < 16; i++) {
            lcd.send_string(" ");
        }
        delay(delayTime);

    }
   
}


void init_m(){
        for(int i=0; i<size; i++)
        m[i]=0;
}

// Calculate comfort level based on temperature and humidity
String getComfortLevel(float temp, float hum) {
    // Simplified Heat Index
    if (temp < 18) {
        return "Cold";
    } else if (temp >= 18 && temp <= 24 && hum >= 30 && hum <= 60) {
        return "Comfortable";
    } else if (temp > 24 && temp <= 28) {
        if (hum > 60) {
            return "Hot & Humid";
        } else {
            return "Warm";
        }
    } else if (temp > 28) {
        if (hum > 60) {
            return "Very Humid!";
        } else {
            return "Hot";
        }
    }
    return "Moderate";
}

