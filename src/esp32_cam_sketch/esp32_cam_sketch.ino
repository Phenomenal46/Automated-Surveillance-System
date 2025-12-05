/**
 * Minimal Debug Version of ESP32-CAM Code
 * The ESP32-CAM waits for a UART command from the Arduino Nano.
 * When a line exactly equal to "CAPTURE" is received, it captures an image
 * and sends it via email.
 * Debug outputs are minimized to avoid interference.
 */

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#endif
#include <ESP_Mail_Client.h>
#include "esp_camera.h"

// ===================
// Select camera model
// ===================
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"

// WiFi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// SMTP settings
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT esp_mail_smtp_port_587

#define AUTHOR_EMAIL "your.email@example.com"
#define AUTHOR_PASSWORD "<12 digit App-specific-password>"  // use an app password, NOT your main password
#define RECIPIENT_EMAIL "recipient@example.com"
// Uncomment to enable debug prints (if needed)
// #define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x) // Debug prints disabled
#endif

// Global SMTP session object.
SMTPSession smtp;

// Minimal SMTP callback.
void smtpCallback(SMTP_Status status) {
  // Keep empty to avoid extra output.
}

/**
 * @brief Captures an image from the camera, converts it to JPEG, and sends
 * it via email.
 */
void captureAndSendEmail() {
  DEBUG_PRINT("Capture command received.");

  // Capture image from the camera.
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    return;
  }

  // Ensure image is in the expected format; if not, return.
  if (fb->format != PIXFORMAT_RGB565) {
    esp_camera_fb_return(fb);
    return;
  }

  // Convert the image from RGB565 to JPEG.
  uint8_t *jpg_buf = NULL;
  size_t jpg_size = 0;
  if (!frame2jpg(fb, 80, &jpg_buf, &jpg_size)) {
    esp_camera_fb_return(fb);
    return;
  }

  // Prepare the email message with the captured image.
  SMTP_Attachment att;
  SMTP_Message message;

  message.enable.chunking = true;
  message.sender.name = F("ESP Mail");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Camera Image Capture");
  message.addRecipient(F("user1"), RECIPIENT_EMAIL);
  message.html.content = F("<span style=\"color:#ff0000;\">Camera image:</span><br/><br/><img src=\"cid:image-001\" alt=\"esp32 cam image\">");
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  message.html.charSet = F("utf-8");

  att.descr.filename = F("camera.jpg");
  att.descr.mime = F("image/jpeg");
  att.blob.data = jpg_buf;
  att.blob.size = jpg_size;
  att.descr.content_id = F("image-001");
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  message.addInlineImage(att);

  // Setup SMTP configuration.
  Session_Config config;
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = F("127.0.0.1");
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  if (!smtp.connect(&config)) {
    esp_camera_fb_return(fb);
    if (jpg_buf) free(jpg_buf);
    return;
  }

  if (!smtp.isLoggedIn()) {
    // No verbose output.
  }

  // Send the email.
  MailClient.sendMail(&smtp, &message, true);

  // Cleanup.
  esp_camera_fb_return(fb);
  if (jpg_buf) free(jpg_buf);
}

/**
 * Setup: initializes the camera, WiFi, and SMTP configuration.
 */
void setup() {
  Serial.begin(115200);
  // Optionally print startup only if debugging.
  DEBUG_PRINT("ESP32-CAM started.");

  // Configure the camera.
  camera_config_t camCfg;
  camCfg.ledc_channel = LEDC_CHANNEL_0;
  camCfg.ledc_timer = LEDC_TIMER_0;
  camCfg.pin_d0 = Y2_GPIO_NUM;
  camCfg.pin_d1 = Y3_GPIO_NUM;
  camCfg.pin_d2 = Y4_GPIO_NUM;
  camCfg.pin_d3 = Y5_GPIO_NUM;
  camCfg.pin_d4 = Y6_GPIO_NUM;
  camCfg.pin_d5 = Y7_GPIO_NUM;
  camCfg.pin_d6 = Y8_GPIO_NUM;
  camCfg.pin_d7 = Y9_GPIO_NUM;
  camCfg.pin_xclk = XCLK_GPIO_NUM;
  camCfg.pin_pclk = PCLK_GPIO_NUM;
  camCfg.pin_vsync = VSYNC_GPIO_NUM;
  camCfg.pin_href = HREF_GPIO_NUM;
  camCfg.pin_sscb_sda = SIOD_GPIO_NUM;
  camCfg.pin_sscb_scl = SIOC_GPIO_NUM;
  camCfg.pin_pwdn = PWDN_GPIO_NUM;
  camCfg.pin_reset = RESET_GPIO_NUM;
  camCfg.xclk_freq_hz = 8000000;
  camCfg.pixel_format = PIXFORMAT_RGB565;
  camCfg.frame_size = FRAMESIZE_SVGA;
  camCfg.jpeg_quality = 12; // Do not change this.
  camCfg.fb_count = 1;

  if (esp_camera_init(&camCfg) != ESP_OK) {
    return;
  }

  // Connect to WiFi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }

  // Setup SMTP; disable extra debug.
  MailClient.networkReconnect(true);
  smtp.debug(0);
  smtp.callback(smtpCallback);
}

/**
 * Main loop: reads complete lines from Serial and triggers capture only if
 * the line equals "CAPTURE".
 */
void loop() {
  if (Serial.available()) {
    // Read until a newline character.
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equals("CAPTURE")) {
      captureAndSendEmail();
      delay(500);
    }
    // Ignore any other commands.
  }
  delay(10);
}
