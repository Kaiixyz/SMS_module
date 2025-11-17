#include <SoftwareSerial.h>

#define GSM_RX 2
#define GSM_TX 3
#define RFID_RX 4
#define RFID_TX 5
#define GSM_BAUD 9600
#define RFID_BAUD 9600

SoftwareSerial gsmSerial(GSM_RX, GSM_TX);
SoftwareSerial rfidSerial(RFID_RX, RFID_TX);

bool debugMode = true;

struct Student {
  String id, name, phone, status;
};

Student students[] = {
  {"1234567890", "Alice Johnson", "+1234567890", ""},
  {"0987654321", "Bob Smith", "+0987654321", ""},
  {"1122334455", "Charlie Brown", "+1122334455", ""},
  {"5566778899", "Diana Prince", "+5566778899", ""}
};
const int numStudents = sizeof(students) / sizeof(students[0]);

void setup() {
  Serial.begin(9600);
  gsmSerial.begin(GSM_BAUD);
  rfidSerial.begin(RFID_BAUD);
  debugPrint("System initializing...");
  if (checkGSM()) debugPrint("GSM ready."); else debugPrint("GSM failed.");
  if (checkRFID()) debugPrint("RFID ready."); else debugPrint("RFID failed.");
  debugPrint("Setup complete.");
}

void loop() {
  String tagID = readRFIDTag();
  if (tagID != "") {
    debugPrint("Tag: " + tagID);
    String studentData = lookupStudent(tagID);
    if (studentData != "Unknown") {
      debugPrint("Student: " + studentData);
      String status = updateStatus(tagID);
      debugPrint("Status: " + status);
      String timestamp = getTimestamp();
      String message = "Student " + studentData + " " + status + " at " + timestamp;
      if (status == "IN" && isLate()) message = "Late: " + message;
      sendSMS(getPhone(tagID), message);
      debugPrint("SMS sent.");
    } else debugPrint("Unknown tag.");
  }
  delay(2000);
}

void debugPrint(String msg) { if (debugMode) Serial.println(msg); }

bool checkGSM() {
  gsmSerial.println("AT");
  delay(1000);
  return gsmSerial.available() && gsmSerial.readString().indexOf("OK") != -1;
}

bool checkRFID() {
  rfidSerial.println("AT");
  delay(1000);
  return rfidSerial.available();
}

String readRFIDTag() {
  for (int i = 0; i < 3; i++) {
    rfidSerial.println("READ_TAG");
    delay(1000);
    if (rfidSerial.available()) {
      String tag = rfidSerial.readStringUntil('\n');
      tag.trim();
      if (tag != "") return tag;
    }
  }
  resetRFID();
  return "";
}

String lookupStudent(String id) {
  for (int i = 0; i < numStudents; i++) if (students[i].id == id) return students[i].name;
  return "Unknown";
}

String getPhone(String id) {
  for (int i = 0; i < numStudents; i++) if (students[i].id == id) return students[i].phone;
  return "";
}

String updateStatus(String id) {
  for (int i = 0; i < numStudents; i++) {
    if (students[i].id == id) {
      students[i].status = (students[i].status == "IN") ? "OUT" : "IN";
      return students[i].status;
    }
  }
  return "";
}

String getTimestamp() {
  unsigned long ms = millis(), s = ms / 1000, m = s / 60, h = m / 60, d = h / 24;
  int year = 2023 + (d / 365), dayOfYear = d % 365;
  int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int month = 1, day = 1;
  for (int i = 0; i < 12; i++) {
    if (dayOfYear < monthDays[i]) {
      month = i + 1;
      day = dayOfYear + 1;
      break;
    }
    dayOfYear -= monthDays[i];
  }
  int hour = h % 24, minute = m % 60;
  char buf[20];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
  return String(buf);
}

bool isLate() {
  String ts = getTimestamp();
  int hour = ts.substring(11,13).toInt(), minute = ts.substring(14,16).toInt();
  return hour > 8 || (hour == 8 && minute > 0);
}

void sendSMS(String phone, String msg) {
  for (int i = 0; i < 3; i++) {
    if (!checkGSM()) continue;
    gsmSerial.println("AT+CMGF=1");
    delay(1000);
    if (!gsmSerial.available() || gsmSerial.readString().indexOf("OK") == -1) continue;
    gsmSerial.print("AT+CMGS=\"");
    gsmSerial.print(phone);
    gsmSerial.println("\"");
    delay(1000);
    gsmSerial.print(msg);
    delay(100);
    gsmSerial.write(26);
    delay(5000);
    if (gsmSerial.available()) {
      String resp = gsmSerial.readString();
      if (resp.indexOf("OK") != -1 || resp.indexOf("+CMGS") != -1) return;
    }
  }
  resetGSM();
}

void resetRFID() {
  debugPrint("Resetting RFID...");
  rfidSerial.println("RESET");
  delay(2000);
  rfidSerial.begin(RFID_BAUD);
}

void resetGSM() {
  debugPrint("Resetting GSM...");
  gsmSerial.println("AT+CFUN=1,1");
  delay(5000);
  gsmSerial.begin(GSM_BAUD);
}
