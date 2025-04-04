#include <Wire.h>
#include <LiquidCrystalTr_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust I2C address if needed

// Display states
enum DisplayState {
  WAITING_FOR_PROGRAM,
  WAITING_FOR_SPOTIFY,
  SHOWING_TRACK_INFO,
  SHOWING_PAUSED,
  SHOWING_DISCONNECTED
};

// Scrolling settings
struct ScrollInfo {
  unsigned long previousMillis;
  int position;
  bool needsScroll;
  bool isPaused;
  bool direction; // true = right to left
  unsigned long pauseStartTime;
  String content;
};

const long SCROLL_INTERVAL = 1000;    // Scroll speed (ms)
const int INITIAL_PAUSE = 2000;      // Pause before scrolling (ms)
String prefixes[2] = {"Now: ", "By: "};
ScrollInfo scrolls[2] = {
  {0, 0, false, true, true, 0, ""},
  {0, 0, false, true, true, 0, ""}
};

DisplayState currentState = WAITING_FOR_PROGRAM;
unsigned long lastStateChange = 0;
bool firstConnection = true;

void setup() {
  Serial.begin(9600);
  lcd.begin();
  lcd.backlight();
  
  showWaitingForProgram();
}

void showWaitingForProgram() {
  currentState = WAITING_FOR_PROGRAM;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for");
  lcd.setCursor(0, 1);
  lcd.print("Python program...");
}

void showWaitingForSpotify() {
  currentState = WAITING_FOR_SPOTIFY;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("Spotify...");
}

void showConnectedMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Spotify");
  lcd.setCursor(0, 1);
  lcd.print("Connected!");
  delay(2000);
  lcd.clear();
}

void showDisconnectedMessage() {
  currentState = SHOWING_DISCONNECTED;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Spotify");
  lcd.setCursor(0, 1);
  lcd.print("Disconnected!");
}

void showPausedMessage() {
  currentState = SHOWING_PAUSED;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Playback Paused");
  scrolls[0].content = "";
  scrolls[1].content = "";
  updateDisplay(0);
  updateDisplay(1);
}

void updateDisplay(int row) {
  lcd.setCursor(0, row);
  
  if (row < 2 && currentState == SHOWING_TRACK_INFO) {
    lcd.print(prefixes[row]);
    
    int availableSpace = 16 - prefixes[row].length();
    scrolls[row].position = constrain(scrolls[row].position, 0, 
                                   max(0, scrolls[row].content.length() - availableSpace));
    
    String displayText = scrolls[row].content.substring(scrolls[row].position, 
                                  min(scrolls[row].position + availableSpace, scrolls[row].content.length()));
    
    lcd.print(displayText);
    
    // Clear remaining space
    for (int i = displayText.length(); i < availableSpace; i++) {
      lcd.print(" ");
    }
  }
}

void handleScrolling(int row) {
  if (!scrolls[row].needsScroll || scrolls[row].content.length() == 0) return;

  unsigned long currentMillis = millis();
  
  if (currentMillis - scrolls[row].previousMillis >= SCROLL_INTERVAL) {
    scrolls[row].previousMillis = currentMillis;
    
    if (!scrolls[row].isPaused) {
      int availableSpace = 16 - prefixes[row].length();
      
      if (scrolls[row].direction) { // Right to left
        if (scrolls[row].position + availableSpace < scrolls[row].content.length()) {
          scrolls[row].position++;
        } else {
          scrolls[row].isPaused = true;
          scrolls[row].pauseStartTime = currentMillis;
          scrolls[row].direction = false;
        }
      } else { // Left to right
        if (scrolls[row].position > 0) {
          scrolls[row].position--;
        } else {
          scrolls[row].isPaused = true;
          scrolls[row].pauseStartTime = currentMillis;
          scrolls[row].direction = true;
        }
      }
      updateDisplay(row);
    } 
    else if (currentMillis - scrolls[row].pauseStartTime > INITIAL_PAUSE) {
      scrolls[row].isPaused = false;
    }
  }
}

void processSerialData() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim();
    
    if (data == "Python Started") {
      showWaitingForSpotify();
    }
    else if (data == "Spotify Connected") {
      if (firstConnection) {
        showConnectedMessage();
        firstConnection = false;
      }
      currentState = SHOWING_TRACK_INFO;
    }
    else if (data == "Spotify Disconnected") {
      showDisconnectedMessage();
    }
    else if (data == "Playback Paused") {
      showPausedMessage();
    }
    else if (data == "Playback Started") {
      currentState = SHOWING_TRACK_INFO;
      lcd.clear();
    }
    else if (data.indexOf('|') != -1) {
      int separator = data.indexOf('|');
      String track = data.substring(0, separator);
      String artist = data.substring(separator + 1);
      
      scrolls[0].content = track;
      scrolls[1].content = artist;
      
      for (int i = 0; i < 2; i++) {
        int availableSpace = 16 - prefixes[i].length();
        scrolls[i].needsScroll = (scrolls[i].content.length() > availableSpace);
        scrolls[i].position = 0;
        scrolls[i].isPaused = true;
        scrolls[i].pauseStartTime = millis();
        scrolls[i].direction = true;
      }
      
      currentState = SHOWING_TRACK_INFO;
      updateDisplay(0);
      updateDisplay(1);
    }
  }
}

void loop() {
  processSerialData();
  
  if (currentState == SHOWING_TRACK_INFO) {
    handleScrolling(0);
    handleScrolling(1);
  }
  else if (currentState == WAITING_FOR_PROGRAM || currentState == WAITING_FOR_SPOTIFY) {
    // Blink waiting message every second
    if (millis() - lastStateChange >= 1000) {
      lastStateChange = millis();
      lcd.clear();
      if (currentState == WAITING_FOR_PROGRAM) {
        lcd.setCursor(0, 0);
        lcd.print("Waiting for");
        lcd.setCursor(0, 1);
        lcd.print("Python program...");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("Connecting to");
        lcd.setCursor(0, 1);
        lcd.print("Spotify...");
      }
    }
  }
  else if (currentState == SHOWING_DISCONNECTED) {
    // Blink disconnected message every second
    if (millis() - lastStateChange >= 1000) {
      lastStateChange = millis();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Spotify");
      lcd.setCursor(0, 1);
      lcd.print("Disconnected!");
    }
  }
}
