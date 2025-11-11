#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <math.h>

MCUFRIEND_kbv tft;

// ---------------- TOUCH HW ----------------
#define YP A3
#define XM A2
#define YM 9
#define XP 8
#define TS_RESISTANCE 300
TouchScreen ts = TouchScreen(XP, YP, XM, YM, TS_RESISTANCE);

#define MINPRESSURE 200
#define MAXPRESSURE 1000

// calibrazione grezza di base na non serve meglio
#define RAW_X_MIN 120
#define RAW_X_MAX 920
#define RAW_Y_MIN 120
#define RAW_Y_MAX 920

// ---------------- COLORI ----------------
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GRAY    0x8410
#define BLUE    0x001F

// ---------------- DISPLAY ----------------
#define WIDTH   480
#define HEIGHT  320
#define CX      (WIDTH / 2)
#define CY      (HEIGHT - 60)

#define RADIUS     230
#define ANG_START  30
#define ANG_END    150

// ---------------- Pin misura valore del partitore e della Vref ----------------
#define PIN_POWER   A6
#define VREF_USED   5.0
#define MEAS_DIV_RATIO  8.0   // partitore 1:8 → 40V pk → 5V ADC

// ---------------- USCITE ----------------
#define SCALE_OUT A14   // LOW = 10W, HIGH = 100W
#define LOAD_OUT  A15   // LOW = 4Ω,  HIGH = 8Ω

// ---------------- STATO ----------------
int   scaleMode    = 1;    // 1 = 0..100, 0 = 0..10
int   loadMode     = 0;    // 0 = 4Ω, 1 = 8Ω
int   powerMode    = 0;    // 0 = RMS, 1 = PICCO

float loadR        = 4.0;
float currentPower = 0.0;
float lastShownPowerText   = -1;
float lastShownPowerNeedle = -1;

bool  touchIsDown = false;

// tensione ricostruita
float lastVpkMeasured  = 0.0;
float lastShownVoltage = -1000.0;

// ---------------- BOTTONI ----------------
#define BTN_X   10
#define BTN_Y   (HEIGHT - 60)
#define BTN_W   120
#define BTN_H   45

#define BTN_LOAD_X  BTN_X
#define BTN_LOAD_Y  (BTN_Y - BTN_H - 5)

#define BTN_MODE_W  BTN_W
#define BTN_MODE_H  BTN_H
#define BTN_MODE_X  (WIDTH - BTN_W - 10)
#define BTN_MODE_Y  (HEIGHT - 60)

// box scritta centrale
int wattBoxX, wattBoxY, wattBoxW, wattBoxH;

// ---------------- Prototipi delle funzioni ----------------
void drawScale(int mode);
void drawButtonScale();
void drawButtonLoad();
void drawButtonMode();
void drawPowerValue(float pwr);
void drawNeedleForPower(float power, uint16_t color, int shiftY);
float readPowerFromA6();
void updatePowerDisplay(float measP);
void readTouch();
void toggleScale();
void toggleLoad();
void togglePowerMode();
void drawWattLabel();
void drawInputVoltage();

void setup()
{
  Serial.begin(9600);
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(BLACK);

  analogReference(DEFAULT);

  pinMode(SCALE_OUT, OUTPUT);
  pinMode(LOAD_OUT, OUTPUT);
  digitalWrite(SCALE_OUT, HIGH); // 100W
  digitalWrite(LOAD_OUT, LOW);   // 4Ω

  drawScale(scaleMode);
  drawButtonScale();
  drawButtonLoad();
  drawButtonMode();
  drawPowerValue(0);
  drawInputVoltage();

  // disegna la lancetta a 0
  drawNeedleForPower(0.0, RED, -5);
  drawWattLabel();

  currentPower          = 0.0;
  lastShownPowerNeedle  = 0.0;
  lastShownPowerText    = 0.0;
}

void loop()
{
  readTouch();
  float p = readPowerFromA6();
  updatePowerDisplay(p);

  if (fabs(lastVpkMeasured - lastShownVoltage) > 0.05)
  {
    drawInputVoltage();
    lastShownVoltage = lastVpkMeasured;
  }
}

// --------------------------------------------------
// Gestione del touch
// --------------------------------------------------
void readTouch()
{
  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (p.z < MINPRESSURE || p.z > MAXPRESSURE)
  {
    touchIsDown = false;
    return;
  }
  
  if (touchIsDown) return;
  touchIsDown = true;

  int sx = map(p.y, RAW_X_MAX, RAW_X_MIN, 0, WIDTH);
  int sy = map(p.x, RAW_Y_MIN, RAW_Y_MAX, HEIGHT, 0);

  // LOAD
  if (sx >= BTN_LOAD_X && sx <= BTN_LOAD_X + BTN_W &&
      sy >= BTN_LOAD_Y && sy <= BTN_LOAD_Y + BTN_H)
  {
    toggleLoad();
    return;
  }

  // SCALE
  if (sx >= BTN_X && sx <= BTN_X + BTN_W &&
      sy >= BTN_Y && sy <= BTN_Y + BTN_H)
  {
    toggleScale();
    return;
  }

  // MODE
  if (sx >= BTN_MODE_X && sx <= BTN_MODE_X + BTN_MODE_W &&
      sy >= BTN_MODE_Y && sy <= BTN_MODE_Y + BTN_MODE_H)
 {
    togglePowerMode();
    return;
 }
}

// --------------------------------------------------
// Lettura della potenza
// --------------------------------------------------
float readPowerFromA6()
{
  int raw = analogRead(PIN_POWER);
  
  if (raw < 2)
  {
   lastVpkMeasured = 0.0;
   return 0.0;
  }

  float v_adc = (raw * VREF_USED) / 1023.0;
  float v_pk  = v_adc * MEAS_DIV_RATIO;   // partitore unico
  lastVpkMeasured = v_pk;

  float p;
  
  if (powerMode == 0)
  {
   p = (v_pk * v_pk) / (2.0 * loadR);    // RMS
  } 
  else
  {
   p = (v_pk * v_pk) / (loadR);          // PICCO
  }
  
  if (p < 0) p = 0;
  return p;
}

// --------------------------------------------------
// Aggiornamento display (soglie per scala 10W)
// --------------------------------------------------
void updatePowerDisplay(float measP)
{
  // filtro
  currentPower = currentPower * 0.6 + measP * 0.4;

  // range
  float maxP = (scaleMode == 0) ? 10.0 : 100.0;

  // soglia a zero diversa
  float zeroSnap = (scaleMode == 0) ? 0.02f : 0.15f;   // 20mW vs 150mW
  if (currentPower < zeroSnap) currentPower = 0.0f;
  if (currentPower > maxP)     currentPower = maxP;

  // soglia di ridisegno lancetta
  float redrawEps = (scaleMode == 0) ? 0.02f : 0.20f;

  bool mustRedrawNeedle =
      (fabs(currentPower - lastShownPowerNeedle) > redrawEps) ||
      (currentPower == 0.0f && lastShownPowerNeedle != 0.0f);

  if (mustRedrawNeedle)
  {
    drawNeedleForPower(lastShownPowerNeedle, BLACK, -5);
    drawWattLabel();
    drawNeedleForPower(currentPower, RED, -5);
    drawWattLabel();
    lastShownPowerNeedle = currentPower;
  }

  // testo
  float textEps = (scaleMode == 0) ? 0.05f : 0.10f;
  
  if (fabs(currentPower - lastShownPowerText) > textEps)
  {
    drawPowerValue(currentPower);
    lastShownPowerText = currentPower;
  }
}

// --------------------------------------------------
// Disegno della scala in modo caruccio
// --------------------------------------------------
void drawScale(int mode)
{
  const int SHIFT = -5;
  const int PIVOT_X = CX;
  const int PIVOT_Y = CY - 5;

  tft.fillRect(0, 0, WIDTH, 200, BLACK);

  // arco blu
  for (float a = ANG_START; a <= ANG_END; a += 1)
  {
    float rad = radians(a);
    int x1 = CX + cos(rad) * (RADIUS - 2);
    int y1 = CY - sin(rad) * (RADIUS - 2) + SHIFT;
    int x2 = CX + cos(rad) * RADIUS;
    int y2 = CY - sin(rad) * RADIUS + SHIFT;
    tft.drawLine(x1, y1, x2, y2, BLUE);
  }

  if (mode == 1)
  {
    // scala 0..100
    for (int v = 0; v <= 100; v += 10)
    {
      float t = v / 100.0;
      float ang = radians(ANG_END - t * (ANG_END - ANG_START));
      float ex = PIVOT_X + cos(ang) * RADIUS;
      float ey = PIVOT_Y - sin(ang) * RADIUS;
      float sx = PIVOT_X + cos(ang) * (RADIUS - 17);
      float sy = PIVOT_Y - sin(ang) * (RADIUS - 17);
      tft.drawLine(sx, sy, ex, ey, WHITE);

      float lx = CX + cos(ang) * (RADIUS + 25);
      float ly = (CY + 10) - sin(ang) * (RADIUS + 25);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(1);
      tft.setCursor(lx - 6, ly - 8);
      tft.print(v);
    }
  }
  else
  {
    // scala 0..10
    for (int v = 0; v <= 10; v++)
    {
      float t = v / 10.0;
      float ang = radians(ANG_END - t * (ANG_END - ANG_START));
      float ex = PIVOT_X + cos(ang) * RADIUS;
      float ey = PIVOT_Y - sin(ang) * RADIUS;
      float sx = PIVOT_X + cos(ang) * (RADIUS - 17);
      float sy = PIVOT_Y - sin(ang) * (RADIUS - 17);
      tft.drawLine(sx, sy, ex, ey, WHITE);

      float lx = CX + cos(ang) * (RADIUS + 25);
      float ly = (CY + 10) - sin(ang) * (RADIUS + 25);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(1);
      tft.setCursor(lx - 4, ly - 8);
      tft.print(v);
    }
  }

  drawWattLabel();
}

// --------------------------------------------------
// Scritta centro scala 
// --------------------------------------------------
void drawWattLabel()
{
  const char *label = (powerMode == 0) ? "Watt RMS" : "Watt Peak";

  int textY = CY - sin(radians(ANG_START)) * (RADIUS - 15) - 5;
  int boxW = 120;
  int textX = CX - (boxW / 2);

  wattBoxX = textX - 2;
  wattBoxY = textY - 2;
  wattBoxW = boxW + 4;
  wattBoxH = 22;

  tft.fillRect(wattBoxX, wattBoxY, wattBoxW, wattBoxH, BLACK);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.setCursor(textX, textY);
  tft.print(label);
}

// --------------------------------------------------
// Valore numerico tensione (font x2)
// --------------------------------------------------
void drawInputVoltage()
{
  int vx = BTN_MODE_X;
  int vy = BTN_MODE_Y - 28;
  int vw = BTN_MODE_W;
  int vh = 26;

  tft.fillRect(vx, vy, vw, vh, BLACK);

  float vpk = lastVpkMeasured;
  float vshow;
  const char *prefix;

  if (powerMode == 0)
  {
   vshow = vpk * 0.7071;
   prefix = "Vrms=";
  }
  else
  {
   vshow = vpk;
   prefix = "Vp=";
  }

  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.setCursor(vx + 2, vy + 4);
  tft.print(prefix);
  tft.print(vshow, 1);
  tft.print("V");
}

// --------------------------------------------------
// Bottoni touch
// --------------------------------------------------
void drawButtonScale()
{
  tft.fillRect(BTN_X, BTN_Y, BTN_W, BTN_H, GRAY);
  tft.drawRect(BTN_X, BTN_Y, BTN_W, BTN_H, WHITE);
  tft.setTextColor(WHITE, GRAY);
  tft.setTextSize(2);
  tft.setCursor(BTN_X + 6, BTN_Y + 15);
  tft.print("Scale");
  tft.setTextSize(2);
  tft.setCursor(BTN_X + 70, BTN_Y + 15);
  tft.setTextColor(BLUE, GRAY);
  
  if (scaleMode == 0) tft.print("10");
  else                tft.print("100");
}

void drawButtonLoad()
{
  tft.fillRect(BTN_LOAD_X, BTN_LOAD_Y, BTN_W, BTN_H, GRAY);
  tft.drawRect(BTN_LOAD_X, BTN_LOAD_Y, BTN_W, BTN_H, WHITE);
  tft.setTextColor(WHITE, GRAY);
  tft.setTextSize(2);
  tft.setCursor(BTN_LOAD_X + 6, BTN_LOAD_Y + 17);
  tft.print("Load");
  tft.setTextSize(2);
  tft.setCursor(BTN_LOAD_X + 62, BTN_LOAD_Y + 17);
  tft.setTextColor(BLUE, GRAY);
  
  if (loadMode == 0) tft.print("4ohm");
  else               tft.print("8ohm");
}

void drawButtonMode()
{
  tft.fillRect(BTN_MODE_X, BTN_MODE_Y, BTN_MODE_W, BTN_MODE_H, GRAY);
  tft.drawRect(BTN_MODE_X, BTN_MODE_Y, BTN_MODE_W, BTN_MODE_H, WHITE);

  tft.setTextColor(WHITE, GRAY);
  tft.setTextSize(2);
  tft.setCursor(BTN_MODE_X + 8, BTN_MODE_Y + 15);
  tft.print("Mode");

  tft.setTextSize(2);
  tft.setTextColor(BLUE, GRAY);
  tft.setCursor(BTN_MODE_X + 78, BTN_MODE_Y + 15);
  
  if (powerMode == 0)
    tft.print("RMS");
  else
    tft.print("PK");
}

// --------------------------------------------------
// Commuta scala
// --------------------------------------------------
void toggleScale()
{
  float p = currentPower;
  if (p < 0.15) p = 0.0;    // snap a zero

  // cancella la lancetta vecchia
  drawNeedleForPower(p, BLACK, -5);
  // cancella area perno
  tft.fillRect(CX - 70, CY - 70, 140, 140, BLACK);

  // cambia scala
  scaleMode = !scaleMode;
  if (scaleMode == 1) digitalWrite(SCALE_OUT, HIGH);
  else                digitalWrite(SCALE_OUT, LOW);

  // ridisegna
  drawScale(scaleMode);
  drawButtonScale();
  drawButtonLoad();
  drawButtonMode();
  drawInputVoltage();

  // ridisegna lancetta con lo stesso p
  drawNeedleForPower(p, RED, -5);
  drawWattLabel();

  currentPower         = p;
  lastShownPowerNeedle = p;
  lastShownPowerText   = -1;
}

// --------------------------------------------------
// Commuta carico
// --------------------------------------------------
void toggleLoad()
{
  loadMode = !loadMode;
  
  if (loadMode == 0)
  {
    loadR = 4.0;
    digitalWrite(LOAD_OUT, LOW);
  }
  else
  {
    loadR = 8.0;
    digitalWrite(LOAD_OUT, HIGH);
  }
  
  drawButtonLoad();
  drawInputVoltage();
  lastShownPowerText = -1;
}

// --------------------------------------------------
// Commuta modo
// --------------------------------------------------
void togglePowerMode()
{
  powerMode = !powerMode;
  drawButtonMode();
  drawWattLabel();
  drawInputVoltage();
  lastShownPowerText   = -1;
  lastShownPowerNeedle = -1;
}

// --------------------------------------------------
// Lettura digitale (numerica)
// --------------------------------------------------
void drawPowerValue(float pwr)
{
  tft.fillRect(CX - 90, CY + 5, 180, 50, BLACK);

  int ip = (int)pwr;
  int dp = (int)((pwr - ip) * 10.0 + 0.5);

  int digits = 1;
  if (ip >= 100) digits = 3;
  else if (ip >= 10) digits = 2;

  int charW = 18;
  int totalWidth = digits * charW + charW + charW + charW + charW;

  int startX = CX - totalWidth / 2;
  int startY = CY + 18;

  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(3);
  tft.setCursor(startX, startY);
  tft.print(ip);
  tft.print('.');
  tft.print(dp);
  tft.print(" W");
}

// --------------------------------------------------
// Lancetta (spessa 2 px)
// --------------------------------------------------
void drawNeedleForPower(float power, uint16_t color, int shiftY)
{
  float maxP = (scaleMode == 0) ? 10.0 : 100.0;
  if (power < 0) power = 0;
  if (power > maxP) power = maxP;

  float ratio = (maxP > 0) ? (power / maxP) : 0;
  float angle = radians(ANG_END - ratio * (ANG_END - ANG_START));

  int pivotX = CX;
  int pivotY = CY + shiftY;

  float tipX = CX + cos(angle) * (RADIUS - 17);
  float tipY = CY - sin(angle) * (RADIUS - 17) + shiftY;

  float px = -sin(angle);
  float py = -cos(angle);

  tft.drawLine(pivotX, pivotY, tipX, tipY, color);
  tft.drawLine(pivotX + px, pivotY + py, tipX + px, tipY + py, color);
  tft.fillCircle(pivotX, pivotY, 6, color);
}
