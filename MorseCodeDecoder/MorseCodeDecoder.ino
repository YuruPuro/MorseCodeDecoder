/**
 * 「シリウスの心臓」ゴッコ用　モールス信号表示器
 * (C) 2022 yurupuro
 */
#include <M5StickCPlus.h>
#include <Wire.h>
#define SIG_PIN 26
#define M5_PIN 37
#define RST_PIN 39

int sigCount;     // トンツー信号のカウント
int sigLen[9];    // [0]は使わない : ON-OFF-ON-OFFF~の時間
int sigStat;      // 受信中ステータス

static int morseCodeNum = 26 ;
// モールス信号復号用辞書 ・ = B10  － = B11
static struct morseCode_s { uint32_t code ; char str ; } morseCode[] = { 
  0x80000000 , 'E' ,  // 1000 0000
  0xA0000000 , 'I' ,  // 1010 0000
  0xA8000000 , 'S' ,  // 1010 1000
  0xAA000000 , 'H' ,  // 1010 1010
  0xAB000000 , 'V' ,  // 1010 1011
  0xAC000000 , 'U' ,  // 1010 1100
  0xAE000000 , 'F' ,  // 1010 1110
  0xB0000000 , 'A' ,  // 1011 0000
  0xB8000000 , 'R' ,  // 1011 1000
  0xBA000000 , 'L' ,  // 1011 1010
  0xBC000000 , 'W' ,  // 1011 1100
  0xBE000000 , 'P' ,  // 1011 1110
  0xBF000000 , 'J' ,  // 1011 1111
  0xC0000000 , 'T' ,  // 1100 0000
  0xE0000000 , 'N' ,  // 1110 0000
  0xE8000000 , 'D' ,  // 1110 1000
  0xEA000000 , 'B' ,  // 1110 1010
  0xEB000000 , 'X' ,  // 1110 1011
  0xEC000000 , 'K' ,  // 1110 1100
  0xEE000000 , 'C' ,  // 1110 1110
  0xEF000000 , 'Y' ,  // 1110 1111
  0xF0000000 , 'M' ,  // 1111 0000
  0xF8000000 , 'G' ,  // 1111 1000
  0xFA000000 , 'Z' ,  // 1111 1010
  0xFB000000 , 'Q' ,  // 1111 1011
  0xFC000000 , 'O' ,  // 1111 1100
} ;

char dispLine[80] ; // 表示用文字列
int dispLineCur;    // 表示カラム位置

// 信号長さ計測用
unsigned long startTime;
int tempo;
int signalTime;
int blinkTime;
int timeSpan;
bool tempoColor;
int longTempoCount;
bool sendSpace; // 短音ｘ７でスペース判定：現状スペーすは表示していない

// ▲▼▲▼▲▼ Decode ▲▼▲▼▲▼
// 受信した信号をビットパターンに変換
int decodeSignal( ) {
  int sspan = timeSpan * 0.5 ;
  int lspan = timeSpan * 1.5 ;
  int decodeCode = -1 ;

  uint32_t code = 0 ;
  uint32_t codeBit = 0 ;
  for (int i=1;i<=sigCount;i++) {
    if (sspan <= sigLen[i] && sigLen[i] <= lspan) {
      codeBit = 0x80000000 >> (2*(i-1));
    } else {
      codeBit = 0xC0000000 >> (2*(i-1));
    }
    code |= codeBit ;
  }
  char str = '.';
  for (int i=0;i<morseCodeNum;i++) {
    if (morseCode[i].code == code) {
      decodeCode = i ;
      str = morseCode[i].str ; 
      break ;
    }
  }
  return decodeCode ;
}

// ▲▼▲▼▲▼ Display ▲▼▲▼▲▼
// 表示の初期化
void initDisplay( ) {
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.fillCircle( 20 , 13 , 10, WHITE);
  M5.Lcd.fillCircle( 55 , 13 , 10, GREENYELLOW);
  M5.Lcd.fillCircle( 80 , 13 , 10, GREENYELLOW);
  M5.Lcd.fillCircle( 105 , 13 , 10, GREENYELLOW);
  M5.Lcd.setTextFont(4) ;
  M5.Lcd.setTextColor(GREENYELLOW, BLACK) ;
  M5.Lcd.setTextDatum(0) ;
  M5.Lcd.setCursor(130, 3) ;
  M5.Lcd.print("T=") ;
  M5.Lcd.print(tempo) ;
}

// ▲▼▲▼▲▼ Display ▲▼▲▼▲▼
// でコードしたビットパターンから文字を表示
void dispCode(int dispCode) {
  uint32_t codeBit = 0 ;
  char str = ' ' ;
  if (dispCode >= 0) {
    codeBit = morseCode[dispCode].code ;
    str = morseCode[dispCode].str ;
  }
  M5.Lcd.fillRect(0,25,240,30,BLACK) ;
  int x = 50 ;
  for (;codeBit != 0;) {
    if ((codeBit & 0xC0000000) == 0x80000000) {
      M5.Lcd.fillRect(x,35,10,10 , WHITE) ;
      x += 20 ;
    } else {
      M5.Lcd.fillRect(x,35,30,10 , WHITE) ;
      x += 40 ;
    }
    codeBit <<= 2 ;
  }

  M5.Lcd.setTextColor(WHITE, BLACK) ;
  M5.Lcd.setCursor(10, 30) ;
  M5.Lcd.print(str) ;

  dispLine[dispLineCur] = str ;
  M5.Lcd.setTextColor(GREENYELLOW, BLACK) ;
  M5.Lcd.setCursor(10 + dispLineCur * 26 , 110) ;
  M5.Lcd.print(str) ;
  
  dispLineCur ++ ;
  if (dispLineCur > 8) {
    M5.Lcd.fillRect(0,80 ,240, 60 ,BLACK) ;
    M5.Lcd.setTextColor(GREENYELLOW, BLACK) ;
    for (int i=0;i<dispLineCur;i++) {
      M5.Lcd.setCursor(10 + i * 26 , 80) ;
      M5.Lcd.print(dispLine[i]) ;
    }
    dispLineCur = 0 ;
  }
}

// ▲▼▲▼▲▼ Setup ▲▼▲▼▲▼
void setup() {
  M5.begin(); // 電源管理がメンドクサイから入れてる
  M5.Axp.ScreenBreath(10);

  pinMode(SIG_PIN,INPUT) ;
  pinMode(M5_PIN,INPUT) ;
  pinMode(RST_PIN,INPUT) ;

  sigCount = 0 ;
  sigStat = LOW ;
  dispLineCur = 0 ;

  tempo  = 300;
  timeSpan = 60 * 1000 / tempo / 2 ;
  longTempoCount = 0 ;
  sendSpace = true ;

  initDisplay( ) ;
  
  tempoColor = true ;
  startTime = millis();
  signalTime = startTime ;
  blinkTime = startTime ;
}

// ▲▼▲▼▲▼ Loop ▲▼▲▼▲▼
void loop() {
  long int newTime = millis();
  int btn = digitalRead(SIG_PIN) ;
 
  if (btn == HIGH) {
    // 打鍵状態の処理
    if (sigStat == LOW) {
      signalTime = newTime ;
      sigStat = HIGH ;
      sigCount ++ ;
    }
    sigLen[sigCount] = newTime - signalTime ;

    switch(longTempoCount) {
      case 1:
        M5.Lcd.fillCircle( 55 , 13 , 10, BLUE);
        break ;
      case 2:
        M5.Lcd.fillCircle( 80 , 13 , 10, BLUE);
        break ;
      case 3:
        M5.Lcd.fillCircle(105 , 13 , 10, BLUE);
        break ;
      default:
        M5.Lcd.fillCircle( 55 , 13 , 10, GREEN);
        M5.Lcd.fillCircle( 80 , 13 , 10, GREEN);
        M5.Lcd.fillCircle( 105 , 13 , 10, GREEN);
    }
  } else {
    // 解放状態の処理
    if (sigStat == HIGH) {
      signalTime = newTime ;
      sigStat = LOW;
    }
    int blankTime = newTime - signalTime ;
    if ((sigCount > 0 && blankTime >= timeSpan * 2.5) || sigCount == 4) {
      // Term
      int decodeCode = decodeSignal( ) ;
      dispCode(decodeCode) ;
      sigCount = 0;
      sendSpace = false ;
    }
    if (!sendSpace && blankTime > timeSpan * 7 && sigCount ==0) {
      // SPACE
      sendSpace = true ;
    }
    
    M5.Lcd.fillCircle( 55 , 13 , 10, GREENYELLOW);
    M5.Lcd.fillCircle( 80 , 13 , 10, GREENYELLOW);
    M5.Lcd.fillCircle( 105 , 13 , 10, GREENYELLOW);
    longTempoCount = 0 ;
  }

  // テンポ表示
  if (newTime - blinkTime >= timeSpan) {
    blinkTime = newTime ;
    tempoColor = tempoColor ? false : true ;
    if (tempoColor) {
      M5.Lcd.fillCircle( 20 , 13 , 10, WHITE);
    } else {
      M5.Lcd.fillCircle( 20 , 13 , 10, ORANGE);
    }
    longTempoCount ++ ;
  }

  // 右側のボタンをクリックでテンポを変える
  if (digitalRead(RST_PIN) == LOW) {
    sigCount = 0 ;
    sigStat = LOW ;
    dispLineCur = 0 ;

    switch(tempo) {
      case 300:  tempo  = 110; break ;
      case 110:  tempo  = 200; break ;
      case 200:  tempo  = 300; break ;
    }
    timeSpan = 60 * 1000 / tempo / 2 ;
    longTempoCount = 0 ;
    sendSpace = true ;
  
    initDisplay( ) ;
    delay(500) ;
  }

  // M5ボタンをクリックで表示を初期状態に戻す
  if (digitalRead(M5_PIN) == LOW) {
    sigCount = 0 ;
    sigStat = LOW ;
    dispLineCur = 0 ;
  
    tempo  = 300;
    timeSpan = 60 * 1000 / tempo / 2 ;
    longTempoCount = 0 ;
    sendSpace = true ;
  
    initDisplay( ) ;
    delay(500) ;
  }
}
