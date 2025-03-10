/* Importing libraries */
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "index_color_tracking.h"

const char* ssid = ""; /* Replace your SSID */
const char* password = ""; /* Replace your Password */


/* Defining pin numbers and channels */
int panServo = 2;
int tiltServo = 12;
int panChannel = 2;
int tiltChannel = 4;
int x_widthMid = 200; /* Screen width mid point --> x */
int y_heightMid = 148; /* Screen height mid point --> y */

int pan_servo_mid = 3250  + ((6500 - 3250) / 2); /* Mid of pan servo 50 hz PWM, 16-bit resolution and range from 3250 to 6500 */
int tilt_servo_mid = 3250  + ((6500 - 3250) / 2); /* Mid of tilt servo 50 hz PWM, 16-bit resolution and range from 3250 to 6500 */

String Feedback = "";
String Command = "", cmd = "", P1 = "", P2 = "", P3 = "", P4 = "", P5 = "", P6 = "", P7 = "", P8 = "", P9 = "";
byte ReceiveState = 0, cmdState = 1, strState = 1, questionstate = 0, equalstate = 0, semicolonstate = 0;

/* AI-Thinker (modelo da câmera)*/
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WiFiServer server(80);

void ExecuteCommand() {
  if (cmd != "colorDetect") {
    //Serial.println("cmd= "+cmd+" ,P1= "+P1+" ,P2= "+P2+" ,P3= "+P3+" ,P4= "+P4+" ,P5= "+P5+" ,P6= "+P6+" ,P7= "+P7+" ,P8= "+P8+" ,P9= "+P9);
    //Serial.println("");
  }

  if (cmd == "resetwifi") {
    WiFi.begin(P1.c_str(), P2.c_str());
    Serial.print("Connecting to ");
    Serial.println(P1);
    long int StartTime = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
      if ((StartTime + 5000) < millis()) break;
    }
    Serial.println("");
    Serial.println("STAIP: " + WiFi.localIP().toString());
    Feedback = "STAIP: " + WiFi.localIP().toString();
  }
  else if (cmd == "restart") {
    ESP.restart();
  }
  else if (cmd == "cm") {
    int XcmVal = P1.toInt();
    int YcmVal = P2.toInt();
    Serial.println("cmd= " + cmd + " ,VALXCM= " + XcmVal);
    Serial.println("cmd= " + cmd + " ,VALYCM= " + YcmVal);
    pan_tilt_Servo(XcmVal, YcmVal); /* Method for pan and tilt servo angle position adjusting
    ( Passing x and y axis co-ordination value of screen ) */

  }
  else if (cmd == "quality") {
    sensor_t * s = esp_camera_sensor_get();
    int val = P1.toInt();
    s->set_quality(s, val);
  }
  else if (cmd == "contrast") {
    sensor_t * s = esp_camera_sensor_get();
    int val = P1.toInt();
    s->set_contrast(s, val);
  }
  else if (cmd == "brightness") {
    sensor_t * s = esp_camera_sensor_get();
    int val = P1.toInt();
    s->set_brightness(s, val);
  }
  else {
    Feedback = "Command is not defined.";
  }
  if (Feedback == "") {
    Feedback = Command;
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  ledcSetup(panChannel, 50, 16); /* (channel, freq, resolution) */ /* 50 hz PWM, 16-bit resolution and range from 3250 to 6500 */
  ledcAttachPin(panServo, panChannel); /* (pin, channel) */
  ledcSetup(tiltChannel, 50, 16); /* (channel, freq, resolution) */ /* 50 hz PWM, 16-bit resolution and range from 3250 to 6500 */
  ledcAttachPin(tiltServo, tiltChannel); /* (pin, channel) */

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  ledcWrite(panChannel, 3250 + ((6500 - 3250) / 2)); /* Setting pan servo to mid point */
  ledcWrite(tiltChannel, 3250 + ((6500 - 3250) / 2)); /* Setting tilt servo to mid point */

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(100);
    ESP.restart();
  }

  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF);  //UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  delay(100);

  long int StartTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    if ((StartTime + 10000) < millis())
      break;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("ESP IP Address: http://");
    Serial.println(WiFi.localIP());
  }
  server.begin();

}

void loop() {

  Feedback = ""; Command = ""; cmd = ""; P1 = ""; P2 = ""; P3 = ""; P4 = ""; P5 = ""; P6 = ""; P7 = ""; P8 = ""; P9 = "";
  ReceiveState = 0, cmdState = 1, strState = 1, questionstate = 0, equalstate = 0, semicolonstate = 0;

  WiFiClient client = server.available();

  if (client) {
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        getCommand(c);

        if (c == '\n') {
          if (currentLine.length() == 0) {

            if (cmd == "colorDetect") {
              camera_fb_t * fb = NULL;
              fb = esp_camera_fb_get();
              if (!fb) {
                Serial.println("Camera capture failed");
                delay(100);
                ESP.restart();
              }

              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: image/jpeg");
              client.println("Content-Disposition: form-data; name=\"imageFile\"; filename=\"picture.jpg\"");
              client.println("Content-Length: " + String(fb->len));
              client.println("Connection: close");
              client.println();

              uint8_t *fbBuf = fb->buf;
              size_t fbLen = fb->len;
              for (size_t n = 0; n < fbLen; n = n + 1024) {
                if (n + 1024 < fbLen) {
                  client.write(fbBuf, 1024);
                  fbBuf += 1024;
                }
                else if (fbLen % 1024 > 0) {
                  size_t remainder = fbLen % 1024;
                  client.write(fbBuf, remainder);
                }
              }
              esp_camera_fb_return(fb);
            }
            else {
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: text/html; charset=utf-8");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Connection: close");
              client.println();
              String Data = "";
              if (cmd != "")
                Data = Feedback;
              else {
                Data = String((const char *)INDEX_HTML);
              }
              int Index;
              for (Index = 0; Index < Data.length(); Index = Index + 1000) {
                client.print(Data.substring(Index, Index + 1000));
              }
              client.println();
            }

            Feedback = "";
            break;
          } else {
            currentLine = "";
          }
        }
        else if (c != '\r') {
          currentLine += c;
        }
        if ((currentLine.indexOf("/?") != -1) && (currentLine.indexOf(" HTTP") != -1)) {
          if (Command.indexOf("stop") != -1) {
            client.println();
            client.println();
            client.stop();
          }
          currentLine = "";
          Feedback = "";
          ExecuteCommand();
        }
      }
    }
    delay(1);
    client.stop();
  }
}

void getCommand(char c) {
  if (c == '?') ReceiveState = 1;
  if ((c == ' ') || (c == '\r') || (c == '\n')) ReceiveState = 0;

  if (ReceiveState == 1) {
    Command = Command + String(c);
    if (c == '=') cmdState = 0;
    if (c == ';') strState++;
    if ((cmdState == 1) && ((c != '?') || (questionstate == 1))) cmd = cmd + String(c);
    if ((cmdState == 0) && (strState == 1) && ((c != '=') || (equalstate == 1))) P1 = P1 + String(c);
    if ((cmdState == 0) && (strState == 2) && (c != ';')) P2 = P2 + String(c);
    if ((cmdState == 0) && (strState == 3) && (c != ';')) P3 = P3 + String(c);
    if ((cmdState == 0) && (strState == 4) && (c != ';')) P4 = P4 + String(c);
    if ((cmdState == 0) && (strState == 5) && (c != ';')) P5 = P5 + String(c);
    if ((cmdState == 0) && (strState == 6) && (c != ';')) P6 = P6 + String(c);
    if ((cmdState == 0) && (strState == 7) && (c != ';')) P7 = P7 + String(c);
    if ((cmdState == 0) && (strState == 8) && (c != ';')) P8 = P8 + String(c);
    if ((cmdState == 0) && (strState >= 9) && ((c != ';') || (semicolonstate == 1))) P9 = P9 + String(c);
    if (c == '?') questionstate = 1;
    if (c == '=') equalstate = 1;
    if ((strState >= 9) && (c == ';')) semicolonstate = 1;
  }
}


void pan_tilt_Servo(int xVal, int yVal) {

  /* Adjusting pan servo angle */
  if (xVal < (x_widthMid - 25)) {
    pan_servo_mid += 200; /* Increasing the pan servo channel PWM */
    if (pan_servo_mid > 6500)
      pan_servo_mid = 6500;

    ledcWrite(panChannel, pan_servo_mid);
  }
  if (xVal > (x_widthMid + 25)) {
    pan_servo_mid -= 200; /* Decreasing the pan servo channel PWM */

    if (pan_servo_mid < 3250)
      pan_servo_mid = 3250;

    ledcWrite(panChannel, pan_servo_mid);
  }

  /* Adjusting tilt servo angle */
  if (yVal < (y_heightMid + 25)) {
    tilt_servo_mid += 200; /* Increasing the tilt servo channel PWM */
    if (tilt_servo_mid > 6500)
      tilt_servo_mid = 6500;

    ledcWrite(tiltChannel, tilt_servo_mid);
  }
  if (yVal > (y_heightMid - 25)) {
    tilt_servo_mid -= 200;/* Decreasing the tilt servo channel PWM */

    if (tilt_servo_mid < 3250)
      tilt_servo_mid = 3250;

    ledcWrite(tiltChannel, tilt_servo_mid);
  }

}
