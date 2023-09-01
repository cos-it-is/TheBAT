#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>
typedef WebServer WiFiWebServer;
#include <FS.h>
#include <SPIFFS.h>
#include <AutoConnect.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <JC_Button.h>
#include <Hash.h>
#include "qrcoded.h"
#include "Bitcoin.h"
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#include <Adafruit_Thermal.h>
#include <stdlib.h>
#include "time.h"
#include "logo.h"

//========================================================//
//===================HARDWARE SETTINGS====================//
//========================================================//

//SETUP SCREEN - 7 INCH
#define GRAFFITI 0xFFFF
#define TFT_BL 45
Arduino_ESP32RGBPanel* rgbpanel = new Arduino_ESP32RGBPanel(
  39 /* DE */, 38 /* VSYNC */, 5 /* HSYNC */, 9 /* PCLK */,
  10 /* R0 */, 11 /* R1 */, 12 /* R2 */, 13 /* R3 */, 14 /* R4 */,
  21 /* G0 */, 0 /* G1 */, 46 /* G2 */, 3 /* G3 */, 8 /* G4 */, 18 /* G5 */,
  17 /* B0 */, 16 /* B1 */, 15 /* B2 */, 7 /* B3 */, 6 /* B4 */,
  0 /* hsync_polarity */, 20 /* hsync_front_porch */, 1 /* hsync_pulse_width */, 87 /* hsync_back_porch */,
  0 /* vsync_polarity */, 5 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 31 /* vsync_back_porch */,
  1 /* pclk_active_neg */, 16000000 /* prefer_speed */,
  0 /* de_idle_high */, 0 /* pclk_idle_high */);

Arduino_RGB_Display* gfx = new Arduino_RGB_Display(
  800 /* width */, 480 /* height */, rgbpanel);

//FORMATTING
bool format = false;  // true for formatting memory, use once, then make false and reflash

//BUTTON SETTINGS--.
#define buttonPin 40  //Set to GPIO for signal to button
int debounce = 100;   //Set the debounce time (milliseconds) for the button, default should be 25, but may need modification if issues with sporadic presses are seen.
#define apButton 0

//BILL ACCEPTOR SETTINGS--
#define RXB 4  //define the GPIO connected TO the TX of the bill acceptor
#define TXB 1  //define the GPIO connected TO the RX of the bill acceptor

//COIN ACCEPTOR SETTINGS--
#define RXC 20          //define the GPIO connected TO the SIGNAL/TX of the coin acceptor
#define INHIBITMECH 41  //define the GPIO connected TO the INHIBIT of the coin acceptor

//THERMAL PRINTER SETTINGS--
#define RXP 42  //define the GPIO connected TO the TX of the thermal printer
#define TXP 19  //define the GPIO connected TO the RX of the thermal printer

//RELAY SETTINGS--
#define RELAY_PIN 2

struct Quote {
  String text;
  String author;
};

Quote quotes[] = {
  { "The root problem with conventional currency is all the trust that's required to make it work.", "Satoshi Nakamoto" },
  { "Bitcoin is a remarkable cryptographic achievement and the ability to create something that is not duplicable in the digital world has enormous value.", "Eric Schmidt" },
  { "I do think Bitcoin is the first [encrypted money] that has the potential to do something like change the world.", "Peter Thiel" },
  { "The more commodity use cases we have for Bitcoin, the better money it becomes. Think of Gold.", "Ben Arc" },
  { "Bitcoin will do to banks what email did to the postal industry.", "Rick Falkvinge" },
  { "With e-currency based on cryptographic proof, without the need to trust a third-party middleman, money can be secure and transactions effortless.", "Satoshi Nakamoto" },
  { "Bitcoin may be the TCP/IP of money.", "Paul Buchheit" },
  { "If we remember, 15 years ago if you were doing anything on the internet you were going to make millions. I think it could be the same with Bitcoin.", "Jared Kenna" },
  { "Bitcoin is a very exciting development, it might lead to a world currency.", "Kim Dotcom" },
  { "Bitcoin actually has the balance and incentives right, and that is why it is starting to take off.", "Julian Assange" },
  { "So bitcoin is cyber snob currency...", "William Shatner" },
  { "Virgin Galactic is a bold entrepreneurial technology. It's driving a revolution and Bitcoin is doing just the same when it comes to inventing a new currency.", "Sir Richard Branson" },
  { "I really like Bitcoin. I own Bitcoins. It's a store of value, a distributed ledger. It's a great place to put assets, especially in places like Argentina with 40 percent inflation, where $1 today is worth 60 cents in a year, and a government's currency does not hold value.", "David Marcus" },
  { "I am very intrigued by Bitcoin. It has all the signs. Paradigm shift, hackers love it, yet it's derided as a toy. Just like microcomputers.", "Paul Graham" },
  { "Bitcoin is money 2.0, a huge huge huge deal.", "Chamath Palihapitiya" },
  { "You can't stop things like Bitcoin. It will be everywhere and the world will have to readjust. World governments will have to readjust.", "John McAfee" },
  { "Bitcoin is here to stay.", "Roger Ver" },
  { "Bitcoin seems to be a very promising idea. I like the idea of basing security on the assumption that the CPU power of honest participants outweighs that of the attacker.", "Hal Finney" },
  { "Blockchain is the tech. Bitcoin is merely the first mainstream manifestation of its potential.", "Marc Kenigsberg" },
  { "Bitcoin is a very exciting development, it might lead to a world currency. I think over the next decade it will grow to become one of the most important ways to pay for things and transfer assets.", "Kim Dotcom" }
};

struct Joke {
  String text;
};

Joke jokes[] = {
  { "Why don't Bitcoin holders have a panic room? Because there's no room for panic when you HODL." },
  { "Why was the computer cold? It left its Windows open for Bitcoin mining." },
  { "Why couldn't the Bitcoin blockchain buy a house? It couldn't afford the high 'fees'." },
  { "Why do Bitcoiners want a Lamborghini? Because it's the only car that increases in value faster than their Bitcoin." },
  { "What's the difference between Bitcoin and Las Vegas? What happens in Bitcoin stays on the blockchain." },
  { "Why are Bitcoiners immune to inflation? They take a 'byte' out of it every day." },
  { "Why was the Bitcoiner excited at their birthday party? He thought the candles on the cake were a bullish signal." },
  { "Why don't Bitcoin traders ever sleep? Because they fear they'll wake up to a market crash." },
  { "Why was the computer that was mining Bitcoin happy? It was racking up a lot of 'byte' points." },
  { "Why are Bitcoiners excited about late nights? Because the moon is closer." }
};


//GENERAL SETTINGS--
String fwVersion = "1.0";  //set the version of the firmware release here.
String hwVersion = "1.0";  //set the version of the hardware release here.
#define IMAGE_WIDTH 280
#define IMAGE_HEIGHT 280

//========================================================//
//========================================================//
//========================================================//


///////////////////////////////////////////////
////////OTHER VARIABLES AND DECLARATIONS///////
///////////////////////////////////////////////

bool nativeLang, dualLang, stealthMode, ledOn, liveRate, useGraffiti, acceptCoins, acceptNotes, coinOnly, enableWifi, thermalQR, screenQR, printQuotes, printJokes, triggerAp = false, backupReceipt, balanceCheck;
uint16_t colour1, colour2, colour3, colour4, colour5, background;
String lannN = "", lauN = "", porN = "", resN = "", staN = "", wakN = "", entN = "", buyN = "", bitN = "", herN = "", insN = "", exiN = "", lanN = "", feeN = "", totN = "", scaN = "", indN = "", recN = "", scaQRN = "", valN = "", dayN = "", daysN = "", enjN = "", errN = "", errQRN = "", CountdownN = "", timerExpN = "", walletN = "";
String lannE = "English", porE = "Access Point Launched.", resE = "Restart and launch Access Point to Configure", buyE = "BUY", bitE = "BITCOIN", herE = "HERE", insE = "Insert cash to buy Bitcoin", exiE = "Press button to finish", feeE = "Fee", lanE = "Press button to change language", totE = "Total", scaE = "Scan to receive sats.", walletE = "Funding wallet is low, please top-up and restart.";
String indE = "Inserted", recE = "Please take your receipt.", scaQRE = "Scan this QR code with a lightning wallet to receive your sats!", valE = "This is valid for", dayE = "day only", daysE = "days only", enjE = "Enjoy your Sats!", errE = "Error Printing: Check Printer", errQRE = "QR Code will shortly be displayed on the screen.", CountdownE = "Time Remaining", timerExpE = "Timer expired! Printing instead.";
String lann, por, res, ver, buy, bit, her, ins, exi, lan, fee, tot, sca, ind, rec, scaQR, val, day, days, enj, err, errQR, Countdown, timerExp, wallet, qrData, ssid, apPassword = "thebatatm", wifiPassword, lnurlATM, baseURLATM, secretATM, currencyATM = "", ntpServer, invoice, rateapiEndpoint, walletapiEndpoint, apiKey, coinValues, billValues, tz, url;
int maxAmount, charge, bills, moneyTimer, converted, redemptionPd, timer = 0, screenW = 800, screenH = 480, maxReceipts;
float coins, total;
int billAmountInt[6] = { 5, 10, 20, 50, 100, 500 };
float coinAmountFloat[6] = { 0.05, 0.1, 0.2, 0.5, 1, 2 };
float coinAmountSize = sizeof(coinAmountFloat) / sizeof(float);
int billAmountSize = sizeof(billAmountInt) / sizeof(int);
double exchangeRateBTC = -1, exchangeRateSAT = -1, exchangeRateCUR = -1;
long gmtOffset_sec = 0;
struct tm timeinfo;
// Store the last NTP time in a timestamp
time_t lastSyncNtpTime = 0;
// Variable to store the time of last sync in milliseconds & Interval to check and sync time (12 hours in milliseconds)
unsigned long lastSyncTimeSec = 0, syncInterval, timerDuration, getRateInterval;  // Timer duration in seconds;


fs::SPIFFSFS& FlashFS = SPIFFS;
#define FORMAT_ON_FAIL true
#define ELEMENTS_FILE "/elements.json"
#define LANGUAGE_FILE "/language.json"
JsonObject obj;

struct LookupTable {
  char character;
  byte value;
};

LookupTable PC437Characters[] = {
  { 'Ç', 0x80 }, { 'ü', 0x81 }, { 'é', 0x82 }, { 'â', 0x83 }, { 'ä', 0x84 }, { 'à', 0x85 }, { 'å', 0x86 }, { 'ç', 0x87 }, { 'ê', 0x88 }, { 'ë', 0x89 }, { 'è', 0x8A }, { 'ï', 0x8B }, { 'î', 0x8C }, { 'ì', 0x8D }, { 'Ä', 0x8E }, { 'Å', 0x8F }, { 'É', 0x90 }, { 'æ', 0x91 }, { 'Æ', 0x92 }, { 'ô', 0x93 }, { 'ö', 0x94 }, { 'ò', 0x95 }, { 'û', 0x96 }, { 'ù', 0x97 }, { 'ÿ', 0x98 }, { 'Ö', 0x99 }, { 'Ü', 0x9A }, { '¢', 0x9B }, { '£', 0x9C }, { '¥', 0x9D }, { '₧', 0x9E }, { 'ƒ', 0x9F }, { 'á', 0xA0 }, { 'í', 0xA1 }, { 'ó', 0xA2 }, { 'ú', 0xA3 }, { 'ñ', 0xA4 }, { 'Ñ', 0xA5 }, { 'ª', 0xA6 }, { 'º', 0xA7 }, { '¿', 0xA8 }, { '⌐', 0xA9 }, { '¬', 0xAA }, { '½', 0xAB }, { '¼', 0xAC }, { '¡', 0xAD }, { '«', 0xAE }, { '»', 0xAF }, { 'α', 0xE0 }, { 'ß', 0xE1 }, { 'Γ', 0xE2 }, { 'π', 0xE3 }, { 'Σ', 0xE4 }, { 'σ', 0xE5 }, { 'µ', 0xE6 }, { 'τ', 0xE7 }, { 'Φ', 0xE8 }, { 'Θ', 0xE9 }, { 'Ω', 0xEA }, { 'δ', 0xEB }, { '∞', 0xEC }, { 'φ', 0xED }, { 'ε', 0xEE }, { '∩', 0xEF }, { '≡', 0xF0 }, { '±', 0xF1 }, { '≥', 0xF2 }, { '≤', 0xF3 }, { '⌠', 0xF4 }, { '⌡', 0xF5 }, { '÷', 0xF6 }, { '≈', 0xF7 }, { '°', 0xF8 }, { '∙', 0xF9 }, { '·', 0xFA }, { '√', 0xFB }, { 'ⁿ', 0xFC }, { '²', 0xFD }
};

// Function to lookup byte value of a character in CodePage437
byte lookup(char character) {
  for (unsigned int i = 0; i < sizeof(PC437Characters) / sizeof(LookupTable); i++) {
    if (PC437Characters[i].character == character) {
      return PC437Characters[i].value;
    }
  }

  // Return a default value if character is not found in the lookup table
  return 0x00;
}
/////////////////////////////////////////
////END OF VARIABLES AND DECLARATIONS////m
/////////////////////////////////////////

static const char PAGE_ELEMENTS[] PROGMEM = R"(
{
  "uri": "/elements",
  "title": "Configuration Options",
  "menu": true,
  "element": [
    {
      "name": "tablecss",
      "type": "ACStyle",
      "value": "table{font-family:arial,sans-serif;border-collapse:collapse;width:100%;color:black;}td,th{border:1px solid #dddddd;text-align:center;padding:8px;}tr:nth-child(even){background-color:#dddddd;}"
    },
    {
        "name": "header1",
        "type": "ACText",
        "value": "<h1>General Configuration</h1>",
        "style": "font-family:'Arial',sans-serif;text-align:center;color:#163ff5;"
      },
      {
        "name": "caption1",
        "type": "ACText",
        "value": "Here, you can set all of the customisation settings for your B.A.T. Ensure you click 'Save' at the bottom of this page before restarting.",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#484f59;"
      },
      {
        "name": "newlinegen",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "headergen",
        "type": "ACText",
        "value": "<h3>General</h3>",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#163ff5;"
      },
      {
        "name": "newlinegen1",
        "type": "ACElement",
        "value": "<hr>"
      },
    {
        "name": "appassword",
        "type": "ACInput",
        "label": "Access Point Password:",
        "apply": "password",
        "value": "thebatatm",
        "style": "font-family:'Arial',sans-serif;font-size:18px;font-weight:bold;color:#FF4500;margin-bottom:15px;"
      },
      {
        "name": "lnurl",
        "type": "ACInput",
        "label": "ATM String for LNBits:",
        "apply": "text",
        "value": "",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "rateapiendpoint",
        "type": "ACInput",
        "label": "API Endpoint for live prices ('Live Rate' and WiFi Required):",
        "apply": "text",
        "value": "",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "walletapiendpoint",
        "type": "ACInput",
        "label": "API Endpoint for your funding wallet ('Balance Check' and WiFi Required):",
        "apply": "text",
        "value": "",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "apikey",
        "type": "ACInput",
        "label": "API Key:",
        "apply": "text",
        "value": "",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "newlinefiat",
        "type": "ACElement",
        "value": "<hr>"
      },
       {
        "name": "headerfiat",
        "type": "ACText",
        "value": "<h3>Acceptor and FIAT Settings</h3>",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#163ff5;"
      },
      {
        "name": "newlinefiat1",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "acceptcoins",
        "type": "ACCheckbox",
        "label": "Accept Coins",
        "style": "width:90%;display:block;color:#484f59",
        "checked": true
      },
      {
        "name": "acceptnotes",
        "type": "ACCheckbox",
        "label": "Accept Notes",
        "style": "width:90%;display:block;color:#484f59",
        "checked": true
      },
       {
        "name": "coinonly",
        "type": "ACCheckbox",
        "label": "Coin Edition",
        "style": "width:90%;display:block;color:#484f59",
        "checked": false
      },
      {
        "name": "newlinefiat2",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "coinmech",
        "type": "ACInput",
        "label": "Coin Values (Max 6):",
        "value": "0.05,0.1,0.2,0.5,1,2",
        "apply": "text",
        "placeholder": "e.g. 0.05,0.1,0.2,0.5,1,2",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "billmech",
        "type": "ACInput",
        "label": "Bill Values (Max 6):",
        "value": "5,10,20",
        "apply": "text",
        "placeholder": "e.g. 5,10,20",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "maxamount",
        "type": "ACInput",
        "label": "Maximum Deposit (FIAT):",
        "apply": "number",
        "value": "50",
        "placeholder": "e.g. 100",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "charge",
        "type": "ACInput",
        "label": "Fee (%):",
        "apply": "number",
        "value": "",
        "placeholder": "e.g. 5",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "redemptionpd",
        "type": "ACInput",
        "label": "Redemption Period (days):",
        "apply": "number",
        "value": "",
        "placeholder": "Enter a value between 1 and 90",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "newlinecus",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "headercus",
        "type": "ACText",
        "value": "<h3>Custom Options</h3>",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#163ff5;"
      },
      {
        "name": "newlinecus1",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "ledon",
        "type": "ACCheckbox",
        "label": "Coin LED On",
        "style": "width:90%;display:block;color:#484f59;margin-bottom:15px;",
        "checked": true
      },
      {
        "name": "liverate",
        "type": "ACCheckbox",
        "label": "Live Rate",
        "style": "width:90%;display:block;color:#484f59;margin-bottom:15px;",
        "checked": false
      },
      {
        "name": "balance",
        "type": "ACCheckbox",
        "label": "Balance Check",
        "style": "width:90%;display:block;color:#484f59;margin-bottom:15px;",
        "checked": true
      },
      {
        "name": "newlinebal",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "rateint",
        "type": "ACInput",
        "label": "Live Rate Interval",
        "apply": "number",
        "value": "600",
        "placeholder": "e.g. '600' Seconds = 10 minutes",
        "style": "width:90%;display:block;color:#484f59;margin-bottom:15px;"
      },
      {
        "name": "headerwifi",
        "type": "ACText",
        "value": "<h4>WiFi Connection</h4>",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#163ff5;"
      },
      {
        "name": "newlineqr",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
          "name": "wifissid",
          "type": "ACInput",
          "label": "WiFi SSID:",
          "apply": "text",
          "value": "",
          "placeholder": "This must match your WiFi SSID exactly.",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
        "name": "wifipassword",
        "type": "ACInput",
        "label": "WiFi Password:",
        "apply": "password",
        "placeholder": "This must match your WiFi password exactly.",
        "value": "",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "enablewifi",
        "type": "ACCheckbox",
        "label": "Enable WiFi",
        "style": "width:90%;display:block;color:#484f59;margin-bottom:15px;",
        "checked": false
      },
      {
        "name": "headerqr",
        "type": "ACText",
        "value": "<h4>QR Output</h4>",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#163ff5;"
      },
      {
        "name": "newlineqr",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "qrmode",
        "type": "ACRadio",
        "value": [
          "Screen",
          "Printer",
          "Both"
        ],
        "arrange": "vertical",
        "style": "width:90%;display:block;color:#484f59;margin-bottom:15px;",
        "checked": 2
      },
       {
        "name": "newlineqr2",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "quotemode",
        "type": "ACRadio",
        "label": "Quotes or Jokes on the receipt",
        "value": [
          "Quotes",
          "Jokes",
          "None"
        ],
        "arrange": "vertical",
        "style": "width:90%;display:block;color:#484f59;margin-bottom:15px;",
        "checked": 2
      },
      {
        "name": "newlinejok",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "url",
        "type": "ACInput",
        "label": "Your URL",
        "apply": "text",
        "value": "",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "maxreceipts",
        "type": "ACInput",
        "label": "Maximum Receipts on Reel:",
        "apply": "number",
        "value": "2",
        "placeholder": "e.g. 50",
        "style": "width:90%;display:block;color:#484f59"
      },
       {
        "name": "timer",
        "type": "ACInput",
        "label": "QR Display Timer:",
        "apply": "number",
        "placeholder": "E.g. 60 Seconds",
        "value": "60",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "backup",
        "type": "ACCheckbox",
        "label": "Backup Receipt",
        "style": "width:90%;display:block;color:#484f59",
        "checked": true
      },
      {
        "name": "newlinentp",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "ntp",
        "type": "ACInput",
        "label": "NTP Server:",
        "apply": "text",
        "placeholder": "e.g. 0.uk.pool.ntp.org",
        "value": "0.uk.pool.ntp.org",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "offset",
        "type": "ACInput",
        "label": "Timezone Offset (Seconds):",
        "apply": "number",
        "value": "0",
        "placeholder": "e.g. 7200",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "tz",
        "type": "ACInput",
        "label": "Timezone:",
        "apply": "text",
        "placeholder": "e.g. GMT",
        "value": "GMT",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "sync",
        "type": "ACInput",
        "label": "Sync Interval (Seconds):",
        "apply": "number",
        "placeholder": "E.g. 86400 - 24hrs, and should not need changing",
        "value": "86400",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "headercol",
        "type": "ACText",
        "value": "<h4>Colour Mode</h4>",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#163ff5;"
      },
      {
        "name": "newlinecus2",
        "type": "ACElement",
        "value": "<hr>"
      },
       {
        "name": "colourmode",
        "type": "ACRadio",
        "value": [
          "Graffiti",
          "Stealth",
          "Custom"
        ],
        "arrange": "vertical",
        "style": "width:90%;display:block;color:#484f59;margin-bottom:15px;",
        "checked": 1
      },
      {
        "name": "headercol1",
        "type": "ACText",
        "value": "<h4>Custom Colours</h4>",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#163ff5;"
      },
      {
        "name": "newlinecol",
        "type": "ACElement",
        "value": "<hr>"
      },
       {
        "name": "colour1",
        "type": "ACInput",
        "label": "Colour 1:",
        "apply": "text",
        "placeholder": "In 16-Bit format.",
        "value": "0xf7e6",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "colour2",
        "type": "ACInput",
        "label": "Colour 2:",
        "apply": "text",
        "placeholder": "In 16-Bit format.",
        "value": "0xd89d",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "colour3",
        "type": "ACInput",
        "label": "Colour 3:",
        "apply": "text",
        "placeholder": "In 16-Bit format.",
        "value": "0x1744",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "colour4",
        "type": "ACInput",
        "label": "Colour 4:",
        "apply": "text",
        "placeholder": "In 16-Bit format.",
        "value": "0x11bd",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "colour5",
        "type": "ACInput",
        "label": "Colour 5:",
        "apply": "text",
        "placeholder": "In 16-Bit format.",
        "value": "0xf080",
        "style": "width:90%;display:block;color:#484f59"
      },
      {
        "name": "background",
        "type": "ACInput",
        "label": "Background:",
        "apply": "text",
        "placeholder": "In 16-Bit format.",
        "value": "0x0000",
        "style": "width:90%;display:block;color:#484f59"
      },
    {
      "name": "save",
      "type": "ACSubmit",
      "value": "Save",
      "uri": "/save"
    },
    {
      "name": "adjust_width",
      "type": "ACElement",
      "value": "<script type=\"text/javascript\">window.onload=function(){var t=document.querySelectorAll(\"input[type='text']\");for(i=0;i<t.length;i++){var e=t[i].getAttribute(\"placeholder\");e&&t[i].setAttribute(\"size\",e.length*.8)}};</script>"
    }
  ]
}
)";

static const char PAGE_LANGUAGE[] PROGMEM = R"(
{
  "uri": "/language",
  "title": "Language Options",
  "menu": true,
  "element": [
    {
      "name": "tablecss2",
      "type": "ACStyle",
      "value": "table{font-family:arial,sans-serif;border-collapse:collapse;width:100%;color:black;}td,th{border:1px solid #dddddd;text-align:center;padding:8px;}tr:nth-child(even){background-color:#dddddd;}"
    },
    {
        "name": "header5",
        "type": "ACText",
        "value": "<h1>Language Configuration</h1>",
        "style": "font-family:'Arial',sans-serif;text-align:center;color:#163ff5;"
      },
      {
        "name": "caption3",
        "type": "ACText",
        "value": "Here, you can set all of the language settings for your B.A.T. Ensure you click 'Save' at the bottom of this page before restarting.",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#484f59;"
      },
      {
        "name": "newlinelan4",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "headerlan",
        "type": "ACText",
        "value": "<h3>Language Selection</h3>",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#163ff5;"
      },
      {
        "name": "newlinelan1",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "nativelanguage",
        "type": "ACCheckbox",
        "label": "Native Language",
        "checked": false
        },
        {
        "name": "duallanguage",
        "type": "ACCheckbox",
        "label": "Dual Language",
        "checked": false
        },
        {
        "name": "headerdual",
        "type": "ACText",
        "value": "<h4>Dual Language Translations</h4>",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#163ff5;"
        },
        {
        "name": "captiondual",
        "type": "ACText",
        "value": "When selecting 'Dual Language', you must set the translations for the relevent text.",
        "style": "font-family:'Arial',sans-serif;text-align:left;color:#484f59;"
      },
      {
        "name": "newlinedual",
        "type": "ACElement",
        "value": "<hr>"
      },
        {
        "name": "languagename",
        "type": "ACInput",
        "apply": "text",
        "value": "",
        "placeholder": "Your native language. e.g. Français or Español",
        "style": "width:90%;display:block;color:#484f59"
        },
        {
        "name": "portallaunched",
        "type": "ACInput",
        "apply": "text",
        "value": "",
        "placeholder": "Access point launched.",
        "style": "width:90%;display:block;color:#484f59"
        },
        {
        "name": "restartlaunch",
        "type": "ACInput",
        "apply": "text",
        "value": "",
        "placeholder": "Restart and Launch Access Point to configure.",
        "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "buy",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "BUY",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "bitcoin",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "BITCOIN",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "here",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "HERE",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "insertfiat",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Insert cash to buy Bitcoin",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "exit",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Press button to finish",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "changelanguage",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Press button to change language",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "fee",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Fee",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "total",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Total",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "scan",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Scan to receive Sats.",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "inserted",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Inserted",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "receipt",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Please take your receipt.",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "scanqr",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Scan this QR code with a lightning wallet to receive your sats!",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "valid",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "This is valid for",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "enjoy",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Enjoy your sats!",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "error",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Error Printing: Check Printer",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "errorqr",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "QR Code will shortly be displayed on the screen.",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "day",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "day only",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "days",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "days only",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "countdown",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Time Remaining",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "timerexp",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "The timer has expired! Printing instead.",
          "style": "width:90%;display:block;color:#484f59"
        },
        {
          "name": "wallet",
          "type": "ACInput",
          "apply": "text",
          "value": "",
          "placeholder": "Funding wallet is low, please top-up and restart.",
          "style": "width:90%;display:block;color:#484f59"
        },
    {
      "name": "save2",
      "type": "ACSubmit",
      "value": "Save",
      "uri": "/savelanguage"
    },
    {
      "name": "adjust_width2",
      "type": "ACElement",
      "value": "<script type=\"text/javascript\">window.onload=function(){var t=document.querySelectorAll(\"input[type='text']\");for(i=0;i<t.length;i++){var e=t[i].getAttribute(\"placeholder\");e&&t[i].setAttribute(\"size\",e.length*.8)}};</script>"
    }
  ]
}
)";

static const char PAGE_SAVE[] PROGMEM = R"(
{
  "uri": "/save",
  "title": "Elements",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "format": "Your ATM Configuration has been saved.<br><br>Please restart the ATM from the main menu.<br><br>",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "ok",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/elements"
    }
  ]
}
)";

static const char PAGE_LANGUAGESAVE[] PROGMEM = R"(
{
  "uri": "/savelanguage",
  "title": "Elements",
  "menu": false,
  "element": [
    {
      "name": "caption2",
      "type": "ACText",
      "format": "Your ATM Language Settings have been saved.<br><br>Please restart the ATM from the main menu.<br><br>",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    },
    {
      "name": "ok2",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/language"
    }
  ]
}
)";

WiFiWebServer server;
AutoConnect portal(server);
AutoConnectAux elementsAux;
AutoConnectAux saveAux;
AutoConnectAux languageAux;
AutoConnectAux languagesaveAux;
AutoConnectConfig config;
/////////////////////////////////////////
/////////////END ACCESS POINT////////////
/////////////////////////////////////////


Button BTNA(buttonPin, debounce, false, false);
HardwareSerial SerialPort(0);        //Coin Acceptor
Adafruit_Thermal printer(&Serial1);  //Thermal Printer
HardwareSerial SerialPort2(2);       //Note Acceptor


/////////////////////////////////////////
//////////////USED FUNCTIONS/////////////
/////////////////////////////////////////

void drawRGB565Image(int16_t x, int16_t y, const uint16_t* image, int16_t w, int16_t h) {
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      uint16_t pixel = image[j * w + i];

      // Draw the pixel
      gfx->drawPixel(x + i, y + j, pixel);
    }
  }
}

void printText(const char* text, const uint8_t* font = u8g2_font_maniac_tf, uint8_t textSize = 1, uint16_t textColor = GRAFFITI, bool isGraffiti = true, int16_t x = -1, int16_t y = -1) {
  String textString = String(text);

  // Set the text parameters
  gfx->setTextSize(textSize);
  gfx->setFont(font);
  gfx->setUTF8Print(true);

  int16_t x1, y1;
  uint16_t textWidth, textHeight;
  uint16_t charSpacing = 2;
  gfx->getTextBounds(textString, 0, 0, &x1, &y1, &textWidth, &textHeight);

  int16_t centerX = gfx->width() / 2;
  int16_t centerY = gfx->height() / 2;

  if (x == -1) {
    x = centerX - (textWidth / 2) - x1;
  }

  if (y == -1) {
    y = centerY - (textHeight / 2) - y1;
  }

  if (isGraffiti) {
    for (uint16_t i = 0; i < textString.length(); ++i) {
      char currentChar = textString.charAt(i);
      gfx->setCursor(x, y);

      uint16_t randomColor = random(0xFFFF);
      gfx->setTextColor(randomColor);
      gfx->println(currentChar);

      gfx->getTextBounds(&currentChar, 0, 0, &x1, &y1, &textWidth, &textHeight);
      x += textWidth + charSpacing;
    }
  } else {
    gfx->setTextColor(textColor);
    gfx->setCursor(x, y);
    gfx->println(textString);
  }
}


void logo() {
  gfx->fillScreen(BLACK);
  printText("Bitcoin Lightning ATM", u8g2_font_helvB18_te, 2, WHITE, false, -1, 60);
  drawRGB565Image(800 / 2 - IMAGE_WIDTH / 2, 480 / 2 - IMAGE_HEIGHT / 2, logo_bmp, IMAGE_WIDTH, IMAGE_HEIGHT);
  String fwReleaseMoon = ("Firmware Version: " + String(fwVersion.c_str()) + " Moon Edition");
  String hwReleaseMoon = ("Hardware Version: " + String(hwVersion.c_str()) + " Moon Edition");
  String fwReleaseBlock = ("Firmware Version: " + String(fwVersion.c_str()) + " Block Edition");
  String hwReleaseBlock = ("Hardware Version: " + String(hwVersion.c_str()) + " Block Edition");
  if (coinOnly) {
    printText(fwReleaseBlock.c_str(), u8g2_font_helvB18_te, 1, WHITE, false, 10, 430);
    printText(hwReleaseBlock.c_str(), u8g2_font_helvB18_te, 1, WHITE, false, 10, 460);
  } else {
    printText(fwReleaseMoon.c_str(), u8g2_font_helvB18_te, 1, WHITE, false, 10, 430);
    printText(hwReleaseMoon.c_str(), u8g2_font_helvB18_te, 1, WHITE, false, 10, 460);
  }
}

void setLang(const JsonObject& obj) {
  if (!nativeLang) {
    lann = lannE;
    por = porE;
    res = resE;
    buy = buyE;
    bit = bitE;
    her = herE;
    ins = insE;
    exi = exiE;
    fee = feeE;
    lan = lanE;
    tot = totE;
    sca = scaE;
    ind = indE;
    rec = recE;
    scaQR = scaQRE;
    val = valE;
    enj = enjE;
    day = dayE;
    days = daysE;
    err = errE;
    errQR = errQRE;
    Countdown = CountdownE;
    timerExp = timerExpE;
    wallet = walletE;
  } else {
    lann = lannN;
    por = porN;
    res = resN;
    buy = buyN;
    bit = bitN;
    her = herN;
    ins = insN;
    exi = exiN;
    fee = feeN;
    lan = lanN;
    tot = totN;
    sca = scaN;
    ind = indN;
    rec = recN;
    scaQR = scaQRN;
    val = valN;
    enj = enjN;
    day = dayN;
    days = daysN;
    err = errN;
    errQR = errQRN;
    Countdown = CountdownN;
    timerExp = timerExpN;
    wallet = walletN;
  }
}

void getBalance() {
  // Initiate an HTTP client
  HTTPClient http;
  String url = walletapiEndpoint + "?api-key=" + apiKey;
  const char* urlChar = url.c_str();

  // Prepare the URL with the API endpoint
  http.begin(urlChar);
  http.setReuse(true);
  int httpResponseCode = http.GET();
  unsigned long balanceInSats = 0;
  unsigned long maxAmountInSats = maxAmount * exchangeRateSAT;  // Let's assume maxAmount and exchangeRateSAT are globally defined

  // Give some time for the request to be completed
  delay(100);

  if (httpResponseCode == 200) {
    // Get the response string
    String response = http.getString();
    http.end();

    // Parse the response JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
      gfx->fillScreen(WHITE);
      printText(wallet.c_str(), u8g2_font_helvB18_te, 1, RED, false, -1, -1);
      delay(99999999);  // adjust this delay as per your needs
    }

    // Extract the balance in sats
    balanceInSats = doc["balance"].as<unsigned long>() / 1000;  // Adjust the factor as needed
  }

  // Return whether the balance is more than the maxAmount
  if (balanceInSats < maxAmountInSats) {
    gfx->fillScreen(WHITE);
    printText(wallet.c_str(), u8g2_font_helvB18_te, 1, RED, false, -1, -1);
    delay(99999999);  // adjust this delay as per your needs
  }
}


unsigned long getRate() {
  // Make HTTP request to API to get exchange rate
  HTTPClient http;
  http.begin(rateapiEndpoint);
  http.setReuse(true);
  int httpResponseCode = http.GET();
  delay(100);
  if (httpResponseCode == 200) {
    String response = http.getString();
    http.end();
    DynamicJsonDocument ratedoc(1024);
    DeserializationError error = deserializeJson(ratedoc, response);
    if (error) {
      // Set rates to -1 to indicate an error
      exchangeRateBTC = -1;
      exchangeRateCUR = -1;
      exchangeRateSAT = -1;
      return 0;  // Return 0 to indicate an error
    }
    exchangeRateBTC = ratedoc["rate"].as<double>() / 100000000.0;
    exchangeRateCUR = 1.0 / exchangeRateBTC;
    exchangeRateSAT = exchangeRateBTC * 100000000.0;
    // Round the exchange rate to the nearest integer and cast to unsigned long
    unsigned long rate = static_cast<unsigned long>(round(exchangeRateCUR));
    gfx->fillRect(100, 0, 150, 60, background);
    return rate;
  } else {
    http.end();
    // Set rates to -1 to indicate an error
    exchangeRateBTC = -1;
    exchangeRateCUR = -1;
    exchangeRateSAT = -1;
    return 0;  // Return 0 to indicate an error
  }
}

void getRateTimer() {
  static unsigned long lastGetRateTime = 0;  // declare as static variable
  unsigned long currentTime = millis();
  if (currentTime - lastGetRateTime > (getRateInterval * 1000)) {  // Convert interval to seconds
    lastGetRateTime = currentTime;
    if (liveRate && enableWifi && !rateapiEndpoint.isEmpty() && !ssid.isEmpty() && !wifiPassword.isEmpty()) {
      getRate();
    }
  }
}

/////////////////////////////////////////
/////////////END USED FUNCTIONS//////////
/////////////////////////////////////////

/////////////////////////////////////////
//////////PRINTER FUNCTIONS//////////////
/////////////////////////////////////////

void receiptCounter() {
  gfx->fillScreen(background);
  digitalWrite(TFT_BL, LOW);
  delay(500);
  File paramFile = FlashFS.open(ELEMENTS_FILE, "r");

  if (paramFile) {
    DynamicJsonDocument doc(20000);
    DeserializationError error = deserializeJson(doc, paramFile.readString());
    paramFile.close();  // Close the file after reading.

    if (!error) {
      JsonArray root = doc.as<JsonArray>();

      for (JsonVariant variant : root) {
        JsonObject obj = variant.as<JsonObject>();
        String name = obj["name"].as<String>();

        if (name == "maxreceipts") {
          maxReceipts = obj["value"].as<int>();
          // Update maxreceipts
          if (maxReceipts > 0) {
            maxReceipts--;
          }
          obj["value"] = maxReceipts;
        }

        if (maxReceipts == 0 | coinOnly) {
          for (JsonVariant variant : root) {
            JsonObject obj = variant.as<JsonObject>();
            String name = obj["name"].as<String>();

            if (name == "qrmode") {
              obj["checked"] = 1;  // Setting qrMode to 1
              screenQR = true;
              thermalQR = false;
            }
          }
        }
      }

      // Save the updated JSON document to the file
      paramFile = FlashFS.open(ELEMENTS_FILE, "w");
      if (paramFile) {
        serializeJson(doc, paramFile);
        paramFile.close();
      }
    }
  }
  delay(500);
  digitalWrite(TFT_BL, HIGH);
}

bool checkForError() {
  // Transmit the real-time transmission status command for the specified status
  const byte transmitStatusCommand[] = { 0x10, 0x04, 0x01 };
  Serial1.write(transmitStatusCommand, sizeof(transmitStatusCommand));

  delay(50);  // Adjust the delay as needed to allow time for the response

  if (Serial1.available()) {
    byte response = Serial1.read();

    // Check if there is an error condition based on the response value
    if (response != 0x12) {
      // Error condition detected
      return true;
    }
  }
  return false;  // No error condition detected
}

String wrapText(String input, int lineLength) {
  String output = "";
  int curLineStart = 0;
  int curLineEnd = lineLength;

  while (curLineStart < input.length()) {
    if (curLineEnd < input.length()) {
      int lastSpaceInLine = input.lastIndexOf(' ', curLineEnd);
      if (lastSpaceInLine > curLineStart) {
        curLineEnd = lastSpaceInLine;
      }
    } else {
      curLineEnd = input.length();
    }

    output += input.substring(curLineStart, curLineEnd);
    output += "\n";

    curLineStart = curLineEnd + 1;
    curLineEnd = curLineStart + lineLength;
  }

  return output;
}

void printLogo() {
  const byte centerJustificationCommand[] = { 0x1B, 0x61, 0x01 };  // Set center justification
  Serial1.write(centerJustificationCommand, sizeof(centerJustificationCommand));
  const byte printCommand[] = { 0x1C, 0x70, 0x01, 0x00 };  // FS p n m
  Serial1.write(printCommand, sizeof(printCommand));
}

void getDatetime() {
  static long my_gmtOffset_sec = gmtOffset_sec;
  time_t currentTime;
  if ((millis() / 1000) - lastSyncTimeSec >= syncInterval) {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), wifiPassword.c_str());
      delay(3000);
    }

    // Sync the time
    if (WiFi.status() == WL_CONNECTED) {
      configTime(gmtOffset_sec, 0, ntpServer.c_str());
      delay(500);  // Allow time for the sync to happen
      getLocalTime(&timeinfo);

      // Sync the time
      if (my_gmtOffset_sec != 0) {
        configTime(my_gmtOffset_sec, 0, ntpServer.c_str());
        my_gmtOffset_sec = 0;
        delay(500);  // Allow time for the sync to happen
        getLocalTime(&timeinfo);
      }

      // Update last sync time
      lastSyncNtpTime = mktime(&timeinfo);  // Now this should have the correct value
      lastSyncTimeSec = millis() / 1000;    // lastSyncTime is now in seconds
      currentTime = lastSyncNtpTime;
    }
  } else {
    // Calculate the current time based on the seconds count.
    currentTime = (millis() / 1000) - lastSyncTimeSec + lastSyncNtpTime;
  }

  // Convert the current timestamp back to a struct tm
  gmtime_r(&currentTime, &timeinfo);
}

void syncTimeNow() {
  // Ensure WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), wifiPassword.c_str());

    // Wait until WiFi is connected
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }

  // Sync the time
  configTime(gmtOffset_sec, 0, ntpServer.c_str());
  delay(2000);
  // Get the local time
  getLocalTime(&timeinfo);

  // Update last sync time
  lastSyncNtpTime = mktime(&timeinfo);
  lastSyncTimeSec = millis() / 1000;  // lastSyncTime is now in seconds
}

void printQRcode(String qrData) {
  const byte modelCommand[] = { 0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x43, 0x05 };  // Changed the module size to 6
  const byte eccCommand[] = { 0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x45, 0x33 };
  const byte printCommand[] = { 0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x51, 0x30 };
  Serial1.write(modelCommand, sizeof(modelCommand));
  Serial1.write(eccCommand, sizeof(eccCommand));

  int len = qrData.length() + 3;
  byte dataCommand[] = { 0x1D, 0x28, 0x6B, (byte)len, 0x00, 0x31, 0x50, 0x30 };
  Serial1.write(dataCommand, sizeof(dataCommand));
  Serial1.print(qrData);

  Serial1.write(printCommand, sizeof(printCommand));
}

void printEncodedString(const String& str) {
  for (int i = 0; i < str.length(); i++) {
    char character = str.charAt(i);
    if (isAscii(character)) {
      printer.write(character);
    } else {
      byte encodedCharacter = lookup(character);
      printer.write(encodedCharacter);
    }
  }
}


void printReceipt(String qrData) {

  int jokeIndex = random(10);   // re-randomize jokeIndex
  int quoteIndex = random(20);  // re-randomize quoteIndex

  printLogo();
  // Large centered text "Bitcoin Lighting ATM"
  printer.setSize('L');  // Set type size to large
  printer.justify('C');  // Center align text
  printer.feed(3);
  printer.boldOn();
  printer.println("Bitcoin");
  printer.println("Lightning ATM");
  printer.boldOff();

  // Line space
  printer.feed(1);

  // Medium text "Scan this QR code with a lightning wallet to receive your sats!"
  printer.setSize('S');
  String scaQRText = scaQR;
  scaQRText = wrapText(scaQRText, 25);
  printEncodedString(scaQRText);  // Using encoded print

  // Line space
  printer.feed(1);

  // Text "This is valid for X days only"
  printer.underlineOn();
  if (redemptionPd > 1) {
    printEncodedString(val + " " + String(redemptionPd) + " " + days);  // Using encoded print
  } else if (redemptionPd = 1) {
    printEncodedString(val + " " + String(redemptionPd) + " " + day);  // Using encoded print
  }
  printer.underlineOff();
  // 2 Line spaces
  printer.feed(2);
  printQRcode(qrData);
  printer.feed(2);

  // Large centered text "Enjoy your sats!"
  printer.setSize('M');
  printEncodedString(enj);  // Using encoded print
  printer.setSize('S');

  if (!url.isEmpty()) {
    printer.feed(2);
    printer.println(url);
  }
  if (enableWifi && !ssid.isEmpty() && !wifiPassword.isEmpty() && !ntpServer.isEmpty()) {
    printer.feed(1);
    printer.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    printer.println(tz);
  }

  // Select a random quote or joke
  String text;
  if (printJokes) {
    text = jokes[jokeIndex].text;
  } else if (printQuotes) {
    text = quotes[quoteIndex].text + "\n- " + quotes[quoteIndex].author;
  }

  // Print the quote or joke
  text = wrapText(text, 32);  // Wrap the text to a line length of 32 characters
  if (printJokes || printQuotes) {
    printer.feed(2);
    printer.println(text);
  }

  // Few line feeds for easy ripping off
  printer.feed(2);
  const byte reset[] = { 0x1B, 0x40 };
  Serial.write(reset, sizeof(reset));
}


/////////////////////////////////////////
/////////////END PRINTER FUNCTIONS///////
/////////////////////////////////////////


/////////////////////////////////////////
//////////////LOOP FUNCTIONS/////////////
/////////////////////////////////////////

void feedmefiat() {
  getRateTimer();
  if (stealthMode) {
    //debounce = 10;
    printText(buy.c_str(), u8g2_font_maniac_tf, 4, WHITE, false, -1, 155);
    printText(bit.c_str(), u8g2_font_maniac_tf, 4, WHITE, false, -1, -1);
    printText(her.c_str(), u8g2_font_maniac_tf, 4, WHITE, false, -1, 415);
    printText((fee + ": " + String(charge) + "%").c_str(), u8g2_font_helvB18_te, 1, WHITE, false, 600, 40);
    if (exchangeRateCUR != -1) {
      printText((String("1 BTC = ~") + String((int)exchangeRateCUR) + String(" ") + String(currencyATM)).c_str(), u8g2_font_helvB18_te, 1, WHITE, false, 10, 25);
    }
    if (exchangeRateSAT != -1) {
      printText((String("1 ") + String(currencyATM) + String(" = ~") + String((int)exchangeRateSAT) + String(" Sats")).c_str(), u8g2_font_helvB18_te, 1, WHITE, false, 10, 55);
    }
    printText(ins.c_str(), u8g2_font_helvB18_te, 1, WHITE, false, 10, 440);
    if (dualLang) {
      printText((lan + ": " + lannE + "/" + lannN).c_str(), u8g2_font_helvB18_te, 1, WHITE, false, 10, 475);
    }
  }
  if (useGraffiti && !stealthMode) {
    printText(buy.c_str(), u8g2_font_maniac_tf, 4, GRAFFITI, true, -1, 155);
    printText(bit.c_str(), u8g2_font_maniac_tf, 4, GRAFFITI, true, -1, -1);
    printText(her.c_str(), u8g2_font_maniac_tf, 4, GRAFFITI, true, -1, 415);
    printText((fee + ": " + String(charge) + "%").c_str(), u8g2_font_helvB18_te, 1, colour5, false, 600, 40);
    if (exchangeRateCUR != -1) {
      printText((String("1 BTC = ~") + String((int)exchangeRateCUR) + String(" ") + String(currencyATM)).c_str(), u8g2_font_helvB18_te, 1, colour4, false, 10, 25);
    }
    if (exchangeRateSAT != -1) {
      printText((String("1 ") + String(currencyATM) + String(" = ~") + String((int)exchangeRateSAT) + String(" Sats")).c_str(), u8g2_font_helvB18_te, 1, colour4, false, 10, 55);
    }
    printText(ins.c_str(), u8g2_font_helvB18_te, 1, colour4, false, 10, 440);
    if (dualLang) {
      printText((lan + ": " + lannE + "/" + lannN).c_str(), u8g2_font_helvB18_te, 1, colour5, false, 10, 475);
    }
    delay(200);
  }
  if (!useGraffiti && !stealthMode) {
    printText(buy.c_str(), u8g2_font_maniac_tf, 4, colour1, false, -1, 155);
    printText(bit.c_str(), u8g2_font_maniac_tf, 4, colour1, false, -1, -1);
    printText(her.c_str(), u8g2_font_maniac_tf, 4, colour1, false, -1, 415);
    delay(150);
    printText(buy.c_str(), u8g2_font_maniac_tf, 4, colour2, false, -1, 155);
    printText(bit.c_str(), u8g2_font_maniac_tf, 4, colour2, false, -1, -1);
    printText(her.c_str(), u8g2_font_maniac_tf, 4, colour2, false, -1, 415);
    delay(150);
    printText(buy.c_str(), u8g2_font_maniac_tf, 4, colour3, false, -1, 155);
    printText(bit.c_str(), u8g2_font_maniac_tf, 4, colour3, false, -1, -1);
    printText(her.c_str(), u8g2_font_maniac_tf, 4, colour3, false, -1, 415);
    delay(150);
    printText((fee + ": " + String(charge) + "%").c_str(), u8g2_font_helvB18_te, 1, colour5, false, 600, 40);
    if (exchangeRateCUR != -1) {
      printText((String("1 BTC = ~") + String((int)exchangeRateCUR) + String(" ") + String(currencyATM)).c_str(), u8g2_font_helvB18_te, 1, colour4, false, 10, 25);
    }
    if (exchangeRateSAT != -1) {
      printText((String("1 ") + String(currencyATM) + String(" = ~") + String((int)exchangeRateSAT) + String(" Sats")).c_str(), u8g2_font_helvB18_te, 1, colour4, false, 10, 55);
    }
    printText(ins.c_str(), u8g2_font_helvB18_te, 1, colour4, false, 10, 440);
    if (dualLang) {
      printText((lan + ": " + lannE + "/" + lannN).c_str(), u8g2_font_helvB18_te, 1, colour5, false, 10, 475);
    }
  }
}

void qrShowCodeLNURL() {
  gfx->fillScreen(WHITE);
  qrData.toUpperCase();
  const char* qrDataChar = qrData.c_str();
  QRCode qrcoded;
  uint8_t qrcodeData[qrcode_getBufferSize(20)];
  qrcode_initText(&qrcoded, qrcodeData, 11, 0, qrDataChar);

  int qrSize = qrcoded.size * 6;  // Change the multiplier to 9 for a larger QR code
  int offsetX = (screenW - qrSize) / 2;
  int offsetY = (screenH - qrSize) / 2;

  for (uint8_t y = 0; y < qrcoded.size; y++) {
    for (uint8_t x = 0; x < qrcoded.size; x++) {
      if (qrcode_getModule(&qrcoded, x, y)) {
        gfx->fillRect(offsetX + 6 * x, offsetY + 6 * y, 6, 6, BLACK);
      } else {
        gfx->fillRect(offsetX + 6 * x, offsetY + 6 * y, 6, 6, WHITE);
      }
    }
  }
  if (!screenQR && thermalQR) {
    printText(rec.c_str(), u8g2_font_helvB18_te, 1, colour4, false, -1, 35);
    printText(exi.c_str(), u8g2_font_helvB18_te, 1, colour5, false, 10, 470);
  }
  if (screenQR && !thermalQR) {
    printText(sca.c_str(), u8g2_font_helvB18_te, 1, colour4, false, -1, 35);
    printText(exi.c_str(), u8g2_font_helvB18_te, 1, colour5, false, 10, 470);
  }
  if (screenQR && thermalQR) {
    printText((String(sca.c_str()) + " " + String(rec.c_str())).c_str(), u8g2_font_helvB18_te, 1, colour4, false, -1, 35);
    printText(exi.c_str(), u8g2_font_helvB18_te, 1, colour5, false, 10, 470);
  }

  unsigned long startTime = millis();
  unsigned long elapsedTime = 0;
  bool buttonPressed = false;  // Flag variable to track button press

  while (elapsedTime < timerDuration * 1000 && !buttonPressed) {
    BTNA.read();
    if (BTNA.wasReleased()) {
      buttonPressed = true;  // Set the flag when button is pressed
    }

    elapsedTime = millis() - startTime;                                  // Update the elapsed time
    unsigned long remainingTime = (timerDuration - elapsedTime / 1000);  // Calculate remaining time in seconds
                                                                         /*
    // Display the timer on the screen
    gfx->setTextColor(RED);
    gfx->setTextSize(1);
    gfx->setCursor(500, 470);
    gfx->printf("%s: %lu", Countdown.c_str(), remainingTime);
    delay(125);  // Delay for a short period
    gfx->fillRect(700, 450, 50, 60, WHITE);*/

    gfx->setTextColor(RED);
    gfx->setTextSize(1);
    gfx->setCursor(450, 470);
    gfx->printf("%s:", Countdown.c_str());
    gfx->setCursor(700, 470);
    gfx->printf("%lu", remainingTime);
    delay(125);
    gfx->fillRect(695, 450, 100, 100, WHITE);
  }

  if (elapsedTime >= timerDuration * 1000 && backupReceipt && !thermalQR && maxReceipts > 0) {
    gfx->fillScreen(WHITE);
    printText(timerExp.c_str(), u8g2_font_helvB18_te, 1, RED, false, -1, 80);
    printText(rec.c_str(), u8g2_font_helvB18_te, 2, RED, false, -1, 200);
    delay(3000);
    printReceipt(qrData);  // Timer expired, call printReceipt()
    delay(3000);
    printText(exi.c_str(), u8g2_font_helvB18_te, 1, RED, false, 10, 470);
  }
}

void moneyTimerFun() {
  bool waitForTap = true;
  bool langChange = true;
  coins = 0;
  bills = 0;
  total = 0;

  unsigned long startTime = millis();  // store the start time
  while (waitForTap || total == 0) {
    if (total == 0) {
      feedmefiat();
      BTNA.read();
      if (dualLang && BTNA.wasReleased() && total == 0) {
        nativeLang = !nativeLang;
        setLang(obj);
        gfx->fillScreen(background);
        langChange = false;
      }
    }
    if (SerialPort2.available()) {
      int x = SerialPort2.read();
      for (int i = 0; i < billAmountSize; i++) {
        if ((i + 1) == x) {
          bills = bills + billAmountInt[i];
          total = (coins + bills);
          gfx->fillScreen(background);
          printText((String(billAmountInt[i]) + " " + currencyATM + " " + ind).c_str(), u8g2_font_helvB18_te, 2, colour5, false, -1, 100);
          printText((tot + ": " + String(total) + " " + currencyATM).c_str(), u8g2_font_helvB18_te, 3, colour4, false, -1, -1);
          printText(exi.c_str(), u8g2_font_helvB18_te, 1, colour5, false, 10, 460);
        }
      }
    }
    if (SerialPort.available()) {
      int x = SerialPort.read();
      for (int i = 0; i < coinAmountSize; i++) {
        if ((i + 1) == x) {
          coins = coins + coinAmountFloat[i];
          total = (coins + bills);
          gfx->fillScreen(background);
          printText((String(coinAmountFloat[i], 2) + " " + currencyATM + " " + ind).c_str(), u8g2_font_helvB18_te, 2, colour5, false, -1, 100);
          printText((tot + ": " + String(total) + " " + currencyATM).c_str(), u8g2_font_helvB18_te, 3, colour4, false, -1, -1);
          printText(exi.c_str(), u8g2_font_helvB18_te, 1, colour5, false, 10, 460);
        }
      }
    }
    BTNA.read();
    if (BTNA.wasReleased() && (total > 0 || total == maxAmount)) {
      waitForTap = false;
    }
  }

  total = (coins + bills) * 100;

  // Turn off machines
  SerialPort2.write(185);
  digitalWrite(INHIBITMECH, LOW);
}

/////////////////////////////////////////
/////////////END LOOP FUNCTIONS//////////
/////////////////////////////////////////

/////////////////////////////////////////
/////////////////UTILITY/////////////////
/////////////////////////////////////////
void checkWiFiConnection() {

  if (enableWifi) {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid.c_str(), wifiPassword.c_str());
    }
    unsigned long startTime = millis();
    // Wait for connection or timeout after 5 seconds
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(100);
    }
  }
}

void exitOTAStart() {
  digitalWrite(TFT_BL, LOW);
}

void exitOTAProgress(unsigned int amount, unsigned int sz) {
  Serial.printf("OTA in progress: received %d bytes, total %d bytes\n", sz, amount);
}

void exitOTAEnd() {
  digitalWrite(TFT_BL, HIGH);
  gfx->setCursor(30, 30);
  gfx->fillScreen(BLACK);
  gfx->setTextColor(WHITE);
  gfx->println("Updated, Rebooting");
}

void exitOTAError(uint8_t err) {
  digitalWrite(TFT_BL, HIGH);
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  const int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void to_upper(char* arr) {
  for (size_t i = 0; i < strlen(arr); i++) {
    if (arr[i] >= 'a' && arr[i] <= 'z') {
      arr[i] = arr[i] - 'a' + 'A';
    }
  }
}

/////////////////////////////////////////
//////////////END UTILITY////////////////
/////////////////////////////////////////

/////////////////////////////////////////
/////////////////LNURL///////////////////
/////////////////////////////////////////


int xor_encrypt(uint8_t* output, size_t outlen, uint8_t* key, size_t keylen, uint8_t* nonce, size_t nonce_len, uint64_t pin, uint64_t amount_in_cents) {
  // check we have space for all the data:
  // <variant_byte><len|nonce><len|payload:{pin}{amount}><hmac>
  if (outlen < 2 + nonce_len + 1 + lenVarInt(pin) + 1 + lenVarInt(amount_in_cents) + 8) {
    return 0;
  }

  int cur = 0;
  output[cur] = 1;  // variant: XOR encryption
  cur++;

  // nonce_len | nonce
  output[cur] = nonce_len;
  cur++;
  memcpy(output + cur, nonce, nonce_len);
  cur += nonce_len;

  // payload, unxored first - <pin><currency byte><amount>
  int payload_len = lenVarInt(pin) + 1 + lenVarInt(amount_in_cents);
  output[cur] = (uint8_t)payload_len;
  cur++;
  uint8_t* payload = output + cur;                                  // pointer to the start of the payload
  cur += writeVarInt(pin, output + cur, outlen - cur);              // pin code
  cur += writeVarInt(amount_in_cents, output + cur, outlen - cur);  // amount
  cur++;

  // xor it with round key
  uint8_t hmacresult[32];
  SHA256 h;
  h.beginHMAC(key, keylen);
  h.write((uint8_t*)"Round secret:", 13);
  h.write(nonce, nonce_len);
  h.endHMAC(hmacresult);
  for (int i = 0; i < payload_len; i++) {
    payload[i] = payload[i] ^ hmacresult[i];
  }

  // add hmac to authenticate
  h.beginHMAC(key, keylen);
  h.write((uint8_t*)"Data:", 5);
  h.write(output, cur);
  h.endHMAC(hmacresult);
  memcpy(output + cur, hmacresult, 8);
  cur += 8;

  // return number of bytes written to the output
  return cur;
}

void makeLNURL() {
  int randomPin = random(1000, 9999);
  byte nonce[8];
  for (int i = 0; i < 8; i++) {
    nonce[i] = random(256);
  }

  byte payload[51];  // 51 bytes is max one can get with xor-encryption

  size_t payload_len = xor_encrypt(payload, sizeof(payload), (uint8_t*)secretATM.c_str(), secretATM.length(), nonce, sizeof(nonce), randomPin, float(total));
  String preparedURL = baseURLATM + "?atm=1&p=";
  preparedURL += toBase64(payload, payload_len, BASE64_URLSAFE | BASE64_NOPADDING);

  Serial.println(preparedURL);  // REMOVE FOR SECURITY??
  char Buf[200];
  preparedURL.toCharArray(Buf, 200);
  char* url = Buf;
  byte* data = (byte*)calloc(strlen(url) * 2, sizeof(byte));
  size_t len = 0;
  int res = convert_bits(data, &len, 5, (byte*)url, strlen(url), 8, 1);
  char* charLnurl = (char*)calloc(strlen(url) * 2, sizeof(byte));
  bech32_encode(charLnurl, "lnurl", data, len);
  to_upper(charLnurl);
  qrData = charLnurl;
}
/////////////////////////////////////////
///////////////END LNURL/////////////////
/////////////////////////////////////////

void loadConfig() {
  const char* files[] = { ELEMENTS_FILE, LANGUAGE_FILE };

  for (int i = 0; i < 2; i++) {
    File paramFile = FlashFS.open(files[i], "r");
    if (paramFile) {
      DynamicJsonDocument doc(20000);  // important to set the correct size or data is not saved!
      DeserializationError error = deserializeJson(doc, paramFile.readString());
      JsonArray root = doc.as<JsonArray>();

      for (JsonVariant variant : root) {
        obj = variant.as<JsonObject>();
        String name = obj["name"].as<String>();

        if (name == "appassword") {
          apPassword = obj["value"].as<String>();
        } else if (name == "lnurl") {
          lnurlATM = obj["value"].as<String>();
          baseURLATM = getValue(lnurlATM, ',', 0);
          secretATM = getValue(lnurlATM, ',', 1);
          currencyATM = getValue(lnurlATM, ',', 2);
        } else if (name == "rateapiendpoint") {
          rateapiEndpoint = obj["value"].as<String>();
        } else if (name == "walletapiendpoint") {
          walletapiEndpoint = obj["value"].as<String>();
        } else if (name == "apikey") {
          apiKey = obj["value"].as<String>();
        } else if (name == "coinmech") {
          coinValues = obj["value"].as<String>();
          if (!coinValues.isEmpty()) {
            for (int j = 0; j < 6; j++) {
              coinAmountFloat[j] = getValue(coinValues, ',', j).toFloat();
            }
          }
        } else if (name == "billmech") {
          billValues = obj["value"].as<String>();
          if (!billValues.isEmpty()) {
            for (int j = 0; j < 6; j++) {
              billAmountInt[j] = getValue(billValues, ',', j).toInt();
            }
          }
        } else if (name == "maxamount") {
          maxAmount = obj["value"].as<int>();
        } else if (name == "charge") {
          charge = obj["value"].as<int>();
        } else if (name == "colourmode") {
          int colourMode = obj["checked"].as<int>();
          if (colourMode == 1) {  // "Graffiti"
            useGraffiti = true;
            stealthMode = false;
          } else if (colourMode == 2) {  // "Stealth"
            stealthMode = true;
            useGraffiti = false;
          } else if (colourMode == 3) {  // "Custom"
            stealthMode = false;
            useGraffiti = false;
          }
        } else if (name == "ledon") {
          ledOn = obj["checked"].as<bool>();
          digitalWrite(RELAY_PIN, ledOn ? LOW : HIGH);
        } else if (name == "liverate") {
          liveRate = obj["checked"].as<bool>();
        } else if (name == "balance") {
          balanceCheck = obj["checked"].as<bool>();
        } else if (name == "rateint") {
          getRateInterval = obj["value"].as<unsigned long>();
        } else if (name == "wifissid") {
          ssid = obj["value"].as<String>();
        } else if (name == "wifipassword") {
          wifiPassword = obj["value"].as<String>();
        } else if (name == "enablewifi") {
          enableWifi = obj["checked"].as<bool>();
        } else if (name == "qrmode") {
          int qrMode = obj["checked"].as<int>();
          if (qrMode == 1) {  // "Screen"
            screenQR = true;
            thermalQR = false;
          } else if (qrMode == 2) {  // "Printer"
            screenQR = false;
            thermalQR = true;
          } else if (qrMode == 3) {  // "Both"
            screenQR = true;
            thermalQR = true;
          }
        } else if (name == "quotemode") {
          int quoteMode = obj["checked"].as<int>();
          if (quoteMode == 1) {  // "Quotes"
            printQuotes = true;
            printJokes = false;
          } else if (quoteMode == 2) {  // "Jokes"
            printJokes = true;
            printQuotes = false;
          } else if (quoteMode == 3) {  // "Jokes"
            printJokes = false;
            printQuotes = false;
          }
        } else if (name == "maxreceipts") {
          maxReceipts = obj["value"].as<int>();
        } else if (name == "url") {
          url = obj["value"].as<String>();
        } else if (name == "ntp") {
          ntpServer = obj["value"].as<String>();
        } else if (name == "tz") {
          tz = obj["value"].as<String>();
        } else if (name == "offset") {
          gmtOffset_sec = obj["value"].as<long>();
        } else if (name == "sync") {
          syncInterval = obj["value"].as<unsigned long>();
        } else if (name == "timer") {
          timerDuration = obj["value"].as<unsigned long>();
        } else if (name == "redemptionpd") {
          redemptionPd = obj["value"].as<int>();
        } else if (name == "backup") {
          backupReceipt = obj["checked"].as<bool>();
        } else if (name == "acceptcoins") {
          acceptCoins = obj["checked"].as<bool>();
        } else if (name == "acceptnotes") {
          acceptNotes = obj["checked"].as<bool>();
        } else if (name == "coinonly") {
          coinOnly = obj["checked"].as<bool>();
        } else if (name.startsWith("colour")) {
          String colorString = obj["value"].as<String>();
          long colorLong = strtol(colorString.c_str(), NULL, 16);
          uint16_t color = (uint16_t)colorLong;
          int index = name.substring(6).toInt();
          if (index >= 1 && index <= 5) {
            switch (index) {
              case 1:
                colour1 = color;
                break;
              case 2:
                colour2 = color;
                break;
              case 3:
                colour3 = color;
                break;
              case 4:
                colour4 = color;
                break;
              case 5:
                colour5 = color;
                break;
            }
          }
        } else if (name == "background") {
          if (stealthMode) {
            background = BLACK;
          } else {
            String colorStringbg = obj["value"].as<String>();
            long colorLongbg = strtol(colorStringbg.c_str(), NULL, 16);
            background = (uint16_t)colorLongbg;
          }
        } else if (name == "nativelanguage") {
          nativeLang = obj["checked"].as<bool>();
        } else if (name == "duallanguage") {
          dualLang = obj["checked"].as<bool>();
        } else if (name == "languagename") {
          lannN = obj["value"].as<String>();
        } else if (name == "portallaunched") {
          porN = obj["value"].as<String>();
        } else if (name == "restartlaunch") {
          resN = obj["value"].as<String>();
        } else if (name == "buy") {
          buyN = obj["value"].as<String>();
        } else if (name == "bitcoin") {
          bitN = obj["value"].as<String>();
        } else if (name == "here") {
          herN = obj["value"].as<String>();
        } else if (name == "insertfiat") {
          insN = obj["value"].as<String>();
        } else if (name == "exit") {
          exiN = obj["value"].as<String>();
        } else if (name == "changelanguage") {
          lanN = obj["value"].as<String>();
        } else if (name == "fee") {
          feeN = obj["value"].as<String>();
        } else if (name == "total") {
          totN = obj["value"].as<String>();
        } else if (name == "scan") {
          scaN = obj["value"].as<String>();
        } else if (name == "inserted") {
          indN = obj["value"].as<String>();
        } else if (name == "receipt") {
          recN = obj["value"].as<String>();
        } else if (name == "scanqr") {
          scaQRN = obj["value"].as<String>();
        } else if (name == "valid") {
          valN = obj["value"].as<String>();
        } else if (name == "day") {
          dayN = obj["value"].as<String>();
        } else if (name == "days") {
          daysN = obj["value"].as<String>();
        } else if (name == "enjoy") {
          enjN = obj["value"].as<String>();
        } else if (name == "error") {
          errN = obj["value"].as<String>();
        } else if (name == "errorqr") {
          errQRN = obj["value"].as<String>();
        } else if (name == "countdown") {
          CountdownN = obj["value"].as<String>();
        } else if (name == "timerexp") {
          timerExp = obj["value"].as<String>();
        }
      }
    } else {
      triggerAp = true;
    }
    paramFile.close();
  }
}



/////////////////////////////////////////
/////////////////SETUP///////////////////
/////////////////////////////////////////

void setup() {

  // Responder of root page handled directly from WebServer class.
  // Setup screen and button
  gfx->begin();
  gfx->setFont(u8g2_font_helvB18_te);
  gfx->setUTF8Print(true);
  gfx->setCursor(30, 30);
  delay(100);
  pinMode(apButton, INPUT_PULLUP);
  pinMode(TFT_BL, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  BTNA.begin();
  digitalWrite(TFT_BL, HIGH);
  setLang(obj);

  gfx->setCursor(30, 30);
  gfx->fillScreen(BLACK);
  gfx->setTextColor(WHITE);
  gfx->println("Press the BOOT button to launch Access Point");
  delay(100);
  while (timer < 2000) {
    if (digitalRead(apButton) == LOW) {  // Use boot button to launch AP
      timer = 5000;
      triggerAp = true;
    }
    timer = timer + 100;
    delay(100);
  }
  delay(2000);

  FlashFS.begin(FORMAT_ON_FAIL);
  server.on("/", []() {
    String content = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
Place the root page with the sketch application.&ensp;
__AC_LINK__
</body>
</html>
    )";
    content.replace("__AC_LINK__", String(AUTOCONNECT_LINK(COG_16)));
    server.send(200, "text/html", content);
  });



  loadConfig();

  elementsAux.load(FPSTR(PAGE_ELEMENTS));
  elementsAux.on([](AutoConnectAux& aux, PageArgument& arg) {
    File param = FlashFS.open(ELEMENTS_FILE, "r");
    if (param) {
      aux.loadElement(param, { "appassword", "lnurl", "apikey", "rateapiendpoint", "walletapiendpoint", "balance", "ledon", "coinmech", "billmech", "maxamount", "charge", "liverate", "redemptionpd", "qrmode", "quotemode", "colourmode", "wifissid", "wifipassword", "enablewifi", "colour1", "colour2", "colour3", "colour4", "colour5", "background", "acceptcoins", "acceptnotes", "coinonly", "ntp", "offset", "tz", "sync", "url", "timer", "backup", "rateint", "maxreceipts" });
      param.close();
    }
    return String();
  });

  saveAux.load(FPSTR(PAGE_SAVE));
  saveAux.on([](AutoConnectAux& aux, PageArgument& arg) {
    File param = FlashFS.open(ELEMENTS_FILE, "w");
    if (param) {
      elementsAux.saveElement(param, { "appassword", "lnurl", "apikey", "rateapiendpoint", "walletapiendpoint", "balance", "ledon", "coinmech", "billmech", "maxamount", "charge", "liverate", "redemptionpd", "qrmode", "quotemode", "colourmode", "wifissid", "wifipassword", "enablewifi", "colour1", "colour2", "colour3", "colour4", "colour5", "background", "acceptcoins", "acceptnotes", "coinonly", "ntp", "offset", "tz", "sync", "url", "timer", "backup", "rateint", "maxreceipts" });
      param.close();
    }
    return String();
  });

  //SAVE LANGUAGE OPTIONS
  languageAux.load(FPSTR(PAGE_LANGUAGE));
  languageAux.on([](AutoConnectAux& aux, PageArgument& arg) {
    File param = FlashFS.open(LANGUAGE_FILE, "r");
    if (param) {
      aux.loadElement(param, { "nativelanguage", "duallanguage", "languagename", "portallaunched", "restartlaunch", "buy", "bitcoin", "buy", "bitcoin", "here", "insertfiat", "exit", "changelanguage", "fee", "total", "scan", "inserted", "receipt", "scanqr", "valid", "enjoy", "error", "errorqr", "countdown", "timerexp", "wallet", "day", "days" });
      param.close();
    }
    return String();
  });

  languagesaveAux.load(FPSTR(PAGE_LANGUAGESAVE));
  languagesaveAux.on([](AutoConnectAux& aux, PageArgument& arg) {
    File param = FlashFS.open(LANGUAGE_FILE, "w");
    if (param) {
      languageAux.saveElement(param, { "nativelanguage", "duallanguage", "languagename", "portallaunched", "restartlaunch", "buy", "bitcoin", "buy", "bitcoin", "here", "insertfiat", "exit", "changelanguage", "fee", "total", "scan", "inserted", "receipt", "scanqr", "valid", "enjoy", "error", "errorqr", "countdown", "timerexp", "wallet", "day", "days" });
      param.close();
    }
    return String();
  });

  if (triggerAp == true) {
    gfx->setCursor(30, 30);
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->println(por);
    String macAddress = String((uint64_t)ESP.getEfuseMac(), HEX);
    config.apid = "The B.A.T - " + macAddress;
    config.psk = apPassword;
    config.ota = AC_OTA_BUILTIN;
    config.menuItems = AC_MENUITEM_RESET | AC_MENUITEM_UPDATE;
    config.title = "ATM Configuration Portal";
    config.immediateStart = true;
    portal.join({ elementsAux, saveAux });
    portal.join({ languageAux, languagesaveAux });
    portal.config(config);
    portal.onOTAStart(exitOTAStart);
    portal.onOTAEnd(exitOTAEnd);
    portal.onOTAProgress(exitOTAProgress);
    portal.onOTAError(exitOTAError);
    portal.begin();
    while (true) {
      portal.handleClient();
    }
    timer = 2000;
  }

  if (enableWifi) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), wifiPassword.c_str());

    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setCursor(30, 30);

    for (int attempt = 0; attempt < 5; attempt++) {
      if (WiFi.status() == WL_CONNECTED) {
        break;  // If connected to WiFi, break the loop
      }
      gfx->fillScreen(BLACK);
      gfx->setTextColor(WHITE);
      gfx->setCursor(30, 30);
      gfx->println("Connecting to WiFi...");
      delay(3000);
    }

    if (WiFi.status() == WL_CONNECTED) {
      gfx->fillScreen(BLACK);
      gfx->setTextColor(WHITE);
      gfx->setCursor(30, 30);
      gfx->println("Connected to WiFi");
      delay(2000);
    } else {
      gfx->println("Failed to connect to WiFi.");
      delay(5000);
    }
  }

  if (currencyATM == "" || (liveRate && WiFi.status() != WL_CONNECTED)) {
    gfx->setCursor(30, 30);
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->println(res);
    delay(99999999);
  }

  // Start and setup devices
  pinMode(INHIBITMECH, OUTPUT);
  SerialPort.begin(4800, SERIAL_8N1, RXC);
  Serial1.begin(19200, SERIAL_8N1, RXP, TXP);
  SerialPort2.begin(300, SERIAL_8N2, RXB, TXB);
  setLang(obj);
  //check edition
  if (coinOnly) {
    digitalWrite(INHIBITMECH, HIGH);
    SerialPort2.write(185);
    maxReceipts == 0;
    backupReceipt == false;
    ntpServer == "";
    thermalQR = false;
    screenQR = true;
  }
  logo();
  delay(3000);
  if (WiFi.status() == WL_CONNECTED && !ntpServer.isEmpty()) {
    syncTimeNow();
  }
  gfx->fillScreen(background);
  randomSeed(analogRead(A0));  // for random jokes on receipt

  if (liveRate && enableWifi && !rateapiEndpoint.isEmpty() && !ssid.isEmpty() && !wifiPassword.isEmpty()) {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setCursor(30, 30);
    gfx->println("Getting Bitcoin Rate...");
    delay(2000);
    getRate();
  }
  if (liveRate && enableWifi && !walletapiEndpoint.isEmpty() && !ssid.isEmpty() && !wifiPassword.isEmpty() && balanceCheck) {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setCursor(30, 30);
    gfx->println("Checking Wallet Balance...");
    delay(2000);
    getBalance();
  }

  // Turn on setup machines
  if (acceptCoins) {
    digitalWrite(INHIBITMECH, HIGH);
  } else {
    digitalWrite(INHIBITMECH, LOW);
  }
  if (acceptNotes) {
    SerialPort2.write(184);
  } else {
    SerialPort2.write(185);
  }
}

/////////////////////////////////////////
//////////////END SETUP//////////////////
/////////////////////////////////////////

void loop() {
  gfx->fillScreen(background);

  // Accept FIAT
  moneyTimerFun();
  getDatetime();
  makeLNURL();
  checkForError();
  bool isError = checkForError();



  if (screenQR && !thermalQR) {
    qrShowCodeLNURL();
    if (enableWifi && !ssid.isEmpty() && !wifiPassword.isEmpty() && balanceCheck && !walletapiEndpoint.isEmpty()) {
      getBalance();
    }
  }

  if (thermalQR && !screenQR) {
    if (isError) {
      gfx->fillScreen(WHITE);
      printText(err.c_str(), u8g2_font_helvB18_te, 2, RED, false, -1, 80);
      printText(errQR.c_str(), u8g2_font_helvB18_te, 1, RED, false, -1, 200);
      delay(5000);
      qrShowCodeLNURL();
      if (enableWifi && !ssid.isEmpty() && !wifiPassword.isEmpty() && balanceCheck && !walletapiEndpoint.isEmpty()) {
        getBalance();
      }
    } else {
      gfx->fillScreen(WHITE);
      printText(rec.c_str(), u8g2_font_helvB18_te, 2, colour4, false, -1, -1);
      printReceipt(qrData);
      if (enableWifi && !ssid.isEmpty() && !wifiPassword.isEmpty() && balanceCheck && !walletapiEndpoint.isEmpty()) {
        getBalance();
      }
      delay(5000);
      receiptCounter();
    }
  }

  if (screenQR && thermalQR) {
    if (isError) {
      gfx->fillScreen(WHITE);
      printText(err.c_str(), u8g2_font_helvB18_te, 2, RED, false, -1, 80);
      printText(errQR.c_str(), u8g2_font_helvB18_te, 1, RED, false, -1, 200);
      delay(5000);
      qrShowCodeLNURL();
      if (enableWifi && !ssid.isEmpty() && !wifiPassword.isEmpty() && balanceCheck && !walletapiEndpoint.isEmpty()) {
        getBalance();
      }
    } else {
      printReceipt(qrData);
      delay(500);
      qrShowCodeLNURL();
      receiptCounter();
      if (enableWifi && !ssid.isEmpty() && !wifiPassword.isEmpty() && balanceCheck && !walletapiEndpoint.isEmpty()) {
        getBalance();
      }
    }
  }
  // check machines
  if (acceptCoins) {
    digitalWrite(INHIBITMECH, HIGH);
  } else {
    digitalWrite(INHIBITMECH, LOW);
  }
  if (acceptNotes) {
    SerialPort2.write(184);
  } else {
    SerialPort2.write(185);
  }
}
