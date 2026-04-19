#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <FS.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// ---------------- DISPLAY ----------------
#define EPD_CS   5
#define EPD_DC   17
#define EPD_RST  16
#define EPD_BUSY 4

GxEPD2_3C<GxEPD2_420c_GDEY042Z98,
          GxEPD2_420c_GDEY042Z98::HEIGHT> display(
  GxEPD2_420c_GDEY042Z98(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// ---------------- WIFI ----------------
WebServer server(80);
const char* AP_SSID = "ESP32-BOOK";
const char* AP_PASS = "12345678";

unsigned long wifiStartTime = 0;
const unsigned long WIFI_TIMEOUT = 300000; // 5 min
bool wifiEnabled = false;

// ---------------- BUTTONS ----------------
const int PIN_PREV  = 33;
const int PIN_NEXT  = 25;
const int PIN_POWER = 26;

// ---------------- STATE ----------------
String currentPath = "/book.txt";
File uploadFile;
bool screenOn = true;

size_t currentOffset = 0;
size_t nextOffset = 0;

// ---------------- UI ----------------
const int margin = 10;
const int lineHeight = 16;

// ---------------- HTML ----------------
String htmlHome() {
  return R"HTML(
<h2>ESP32 ePaper Book</h2>
<form method="POST" action="/upload" enctype="multipart/form-data">
<input type="file" name="file">
<button type="submit">Upload</button>
</form>
<hr>
<a href="/list">List Files</a>
)HTML";
}

// ---------------- WIFI CONTROL ----------------
void disableWiFi() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiEnabled = false;
  Serial.println("WiFi OFF");
}

void enableWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  wifiEnabled = true;
  wifiStartTime = millis();
  Serial.println("WiFi ON");
}

// ---------------- SAVE / LOAD ----------------
void saveState() {
  File f = LittleFS.open("/state.dat", "w");
  if (!f) return;
  f.println(currentPath);
  f.println(currentOffset);
  f.println(nextOffset);
  f.close();
}

void loadState() {
  if (!LittleFS.exists("/state.dat")) return;
  File f = LittleFS.open("/state.dat", "r");
  if (!f) return;

  currentPath = f.readStringUntil('\n');
  currentPath.trim();
  currentOffset = f.readStringUntil('\n').toInt();
  nextOffset    = f.readStringUntil('\n').toInt();
  f.close();
}

// ---------------- PAGE BUILDER ----------------
size_t buildPage(File &f, size_t startOffset, std::vector<String> &linesOut) {
  f.seek(startOffset);
  size_t pos = startOffset;

  String line = "";
  String word = "";

  int maxWidth = display.width() - 2 * margin;
  int maxLines = (display.height() - 2 * margin) / lineHeight;

  auto lineWidth = [&](const String &txt) -> int {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
    return w;
  };

  auto pushLine = [&](String &ln) {
    if (ln.length() > 0 || linesOut.size() == 0) {
      linesOut.push_back(ln);
    }
    ln = "";
  };

  auto flushWord = [&]() {
    if (word.length() == 0) return;

    String test = (line.length() ? line + " " : "") + word;

    if (lineWidth(test) > maxWidth) {
      pushLine(line);
      if (linesOut.size() >= maxLines) return;
      line = word;
    } else {
      if (line.length()) line += " ";
      line += word;
    }
    word = "";
  };

  while (f.available() && linesOut.size() < maxLines) {
    char c = f.read();
    pos++;

    if (c == '\r') continue;

    if (c == '\n') {
      flushWord();
      pushLine(line);
      continue;
    }

    if (c == ' ') {
      flushWord();
      continue;
    }

    if (c >= 32) {
      word += c;
    }
  }

  flushWord();
  if (line.length() && linesOut.size() < maxLines) {
    pushLine(line);
  }

  return pos;
}


// ---------------- DRAW PAGE ----------------
void drawPage(size_t offset) {
  File f = LittleFS.open(currentPath, "r");
  if (!f) return;
  if (offset >= f.size()) offset = 0;

  std::vector<String> lines;
  nextOffset = buildPage(f, offset, lines);
  f.close();

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    int y = margin + 20;
    for (auto &ln : lines) {
      display.setCursor(margin, y);
      display.print(ln);
      y += lineHeight;
    }

  } while (display.nextPage());

  currentOffset = offset;
  saveState();
}

// ---------------- NAVIGATION ----------------
void goNextPage() {
  drawPage(nextOffset);
}

void goPrevPage() {
  File f = LittleFS.open(currentPath, "r");
  if (!f) return;

  size_t pos = 0;
  size_t prevOffset = 0;
  std::vector<String> dummy;

  while (pos < currentOffset && f.available()) {
    prevOffset = pos;
    dummy.clear();
    pos = buildPage(f, pos, dummy);
  }
  f.close();

  drawPage(prevOffset);
}

// ---------------- POWER ----------------
void togglePower() {
  screenOn = !screenOn;
  if (!screenOn) {
    display.powerOff();
  } else {
    display.init(115200);
    display.setRotation(1);
    enableWiFi();
    drawPage(currentOffset);
  }
}

// ---------------- WEB ----------------
void handleRoot() { server.send(200, "text/html", htmlHome()); }

void handleList() {
  String out = "<h3>Files</h3><ul>";
  File root = LittleFS.open("/");
  File f = root.openNextFile();

  while (f) {
    String name = f.name();
    out += "<li><a href='/set?name=" + name + "'>" + name + "</a></li>";
    f = root.openNextFile();
  }

  out += "</ul>";
  server.send(200, "text/html", out);
}

void handleSet() {
  String name = server.arg("name");
  server.send(200, "text/plain", "Opening: " + name);
  delay(200);

  currentPath = name;
  currentOffset = 0;
  nextOffset = 0;

  saveState();
  drawPage(currentOffset);
}

// ---------------- UPLOAD ----------------
void handleUploadStream() {
  HTTPUpload& up = server.upload();

  if (up.status == UPLOAD_FILE_START) {
    String filename = "/" + up.filename;
    uploadFile = LittleFS.open(filename, "w");
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
    String filename = "/" + up.filename;
    server.send(200, "text/plain", "Upload OK, opening...");
    delay(300);

    currentPath = filename;
    currentOffset = 0;
    nextOffset = 0;

    saveState();
    drawPage(currentOffset);
  }
}

void handleUploadDone() {
  server.send(200, "text/plain", "Upload finished");
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_NEXT, INPUT_PULLUP);
  pinMode(PIN_PREV, INPUT_PULLUP);
  pinMode(PIN_POWER, INPUT_PULLUP);

  display.init(115200);
  display.setRotation(1);

  LittleFS.begin(false);
  enableWiFi();

  server.on("/", handleRoot);
  server.on("/list", handleList);
  server.on("/set", handleSet);
  server.on("/upload", HTTP_POST, handleUploadDone, handleUploadStream);
  server.begin();

  if (!LittleFS.exists("/book.txt")) {
    File f = LittleFS.open("/book.txt", "w");
    f.println("Hello ePaper Reader!");
    f.close();
  }

  loadState();

  if (!LittleFS.exists(currentPath)) {
    currentPath = "/book.txt";
    currentOffset = 0;
    nextOffset = 0;
  }

  drawPage(currentOffset);
}

// ---------------- LOOP ----------------
void loop() {
  server.handleClient();

  if (wifiEnabled && millis() - wifiStartTime > WIFI_TIMEOUT) {
    disableWiFi();
  }

  if (!screenOn) return;

  if (digitalRead(PIN_NEXT) == LOW) { delay(200); goNextPage(); }
  if (digitalRead(PIN_PREV) == LOW) { delay(200); goPrevPage(); }
  if (digitalRead(PIN_POWER) == LOW) { delay(200); togglePower(); }

  delay(10);
}