#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Arduino_GFX_Library.h>

// DC, CS, SCK, MOSI, MISO
Arduino_DataBus *bus = new Arduino_ESP32SPI(10, 1, 4, 3, GFX_NOT_DEFINED);
Arduino_GFX *tft = new Arduino_GC9A01(bus, 0, 0, true);

#define SH 30
Arduino_Canvas *g = new Arduino_Canvas(240, SH, tft);

WebServer server(80);

#define C 120
#define K 3.0f
#define TY 70
#define TH 100
#define TW 36

const uint16_t SKY = RGB565(40, 120, 220);
const uint16_t GND = RGB565(130, 80, 30);
const uint16_t WHT = RGB565(255, 255, 255);
const uint16_t ORG = RGB565(255, 160, 0);
const uint16_t PNL = RGB565(25, 25, 25);
const uint16_t BLK = RGB565(0, 0, 0);

float pitch = 0, roll = 0, spd = 120, alt = 1500, hdg = 0;
bool demo = true;
int yo = 0;

void LN(float x0, float y0, float x1, float y1, uint16_t c) { g->drawLine(x0, y0 - yo, x1, y1 - yo, c); }
void HL(int x, int y, int w, uint16_t c) { g->drawFastHLine(x, y - yo, w, c); }
void VL(int x, int y, int h, uint16_t c) { g->drawFastVLine(x, y - yo, h, c); }
void FR(int x, int y, int w, int h, uint16_t c) { g->fillRect(x, y - yo, w, h, c); }
void DR(int x, int y, int w, int h, uint16_t c) { g->drawRect(x, y - yo, w, h, c); }
void FC(int x, int y, int r, uint16_t c) { g->fillCircle(x, y - yo, r, c); }
void FT(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
  g->fillTriangle(x0, y0 - yo, x1, y1 - yo, x2, y2 - yo, c);
}
void CUR(int x, int y) { g->setCursor(x, y - yo); }

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<style>
*{box-sizing:border-box;-webkit-user-select:none;user-select:none}
body{margin:0;background:#0b0d10;color:#ddd;font-family:monospace;touch-action:none}
#p{max-width:480px;margin:auto;padding:12px;background:linear-gradient(#1a1d22,#101215)}
.ro{display:flex;gap:6px}
.ro div{flex:1;background:#000;border:1px solid #333;border-radius:6px;text-align:center;padding:6px 0;color:#4f4;font-size:20px}
.ro small{display:block;color:#888;font-size:10px}
.row{display:flex;align-items:center;justify-content:space-between;margin:14px 0}
.lv{width:56px;text-align:center;font-size:11px;color:#aaa}
.tr{position:relative;height:220px;width:30px;margin:6px auto;background:#000;border:2px solid #444;border-radius:8px}
.hd{position:absolute;left:-12px;width:50px;height:26px;margin-top:-13px;background:linear-gradient(#777,#333);border:2px solid #aaa;border-radius:6px}
.yk{position:relative;width:230px;height:230px;border-radius:50%;background:radial-gradient(#222,#000);border:3px solid #555}
.yk:before,.yk:after{content:"";position:absolute;background:#333}
.yk:before{left:50%;top:8px;bottom:8px;width:1px}
.yk:after{top:50%;left:8px;right:8px;height:1px}
.pk{position:absolute;left:50%;top:50%;width:44px;height:44px;margin:-22px;border-radius:50%;background:radial-gradient(#fa0,#a60);border:2px solid #fc6}
.kn{position:relative;width:130px;height:130px;margin:auto;border-radius:50%;background:radial-gradient(#333,#111);border:3px solid #666}
.kn i{position:absolute;left:50%;top:6px;width:4px;height:50px;margin-left:-2px;background:#fa0;border-radius:2px;transform-origin:50% 59px}
.sw{display:flex;justify-content:space-around;margin:12px 0}
.sw label{font-size:12px;text-align:center}
.sw input{display:block;margin:4px auto;width:42px;height:22px}
button{background:#222;color:#fa0;border:2px solid #555;border-radius:8px;padding:12px 16px;font-family:monospace;font-size:14px;margin:4px}
.c{text-align:center}
</style></head><body><div id="p">
<div class="ro">
<div><small>PITCH</small><span id="rp">0</span></div>
<div><small>ROLL</small><span id="rr">0</span></div>
<div><small>SPD</small><span id="rs">0</span></div>
<div><small>ALT</small><span id="ra">0</span></div>
<div><small>HDG</small><span id="rh">0</span></div>
</div>
<div class="row">
<div class="lv">SPEED<div class="tr" id="ls"><div class="hd" id="hs"></div></div></div>
<div class="yk" id="yk"><div class="pk" id="pk"></div></div>
<div class="lv">ALT<div class="tr" id="la"><div class="hd" id="ha"></div></div></div>
</div>
<div class="c" style="font-size:11px;color:#888">HEADING</div>
<div class="kn" id="kn"><i id="ki"></i></div>
<div class="sw">
<label>PITCH/ROLL<br>SENSOR<input type="checkbox" id="spr"></label>
<label>HEADING<br>SENSOR<input type="checkbox" id="sh"></label>
</div>
<div class="c"><button id="z">ZERO P/R</button><button id="c0">CENTER</button><button id="d">AUTO DEMO</button></div>
</div>
<script>
const $=i=>document.getElementById(i);
const v={p:0,r:0,s:120,a:1500,h:0};
let sPR=0,sH=0,on=0,busy=0,q=0,cur={p:0,r:0,h:0},off={p:0,r:0};
const cl=(x,a,b)=>Math.max(a,Math.min(b,x));
function ui(){
  const R=$('yk').clientWidth/2-24;
  $('pk').style.transform='translate('+(v.r/90*R)+'px,'+(-v.p/30*R)+'px)';
  $('hs').style.top=(1-v.s/300)*100+'%';
  $('ha').style.top=(1-v.a/5000)*100+'%';
  $('ki').style.transform='rotate('+v.h+'deg)';
  $('rp').innerText=Math.round(v.p);$('rr').innerText=Math.round(v.r);
  $('rs').innerText=Math.round(v.s);$('ra').innerText=Math.round(v.a);
  $('rh').innerText=String(Math.round(v.h)%360).padStart(3,'0');
}
function send(){
  if(busy){q=1;return;}
  busy=1;
  fetch('/d?p='+v.p.toFixed(1)+'&r='+v.r.toFixed(1)+'&s='+Math.round(v.s)+'&a='+Math.round(v.a)+'&h='+Math.round(v.h))
    .finally(()=>{busy=0;if(q){q=0;send();}});
}
function drag(el,fn){
  el.addEventListener('pointerdown',e=>{el.setPointerCapture(e.pointerId);fn(e);el.onpointermove=fn;});
  el.addEventListener('pointerup',()=>{el.onpointermove=null;});
}
drag($('yk'),e=>{
  if(sPR)return;
  const b=$('yk').getBoundingClientRect(),R=b.width/2-24;
  let x=e.clientX-b.left-b.width/2,y=e.clientY-b.top-b.height/2;
  const d=Math.hypot(x,y);if(d>R){x*=R/d;y*=R/d;}
  v.r=x/R*90;v.p=-y/R*30;ui();send();
});
function lever(id,k,max){
  drag($(id),e=>{
    const b=$(id).getBoundingClientRect();
    v[k]=Math.round(cl(1-(e.clientY-b.top)/b.height,0,1)*max);ui();send();
  });
}
lever('ls','s',300);lever('la','a',5000);
drag($('kn'),e=>{
  if(sH)return;
  const b=$('kn').getBoundingClientRect();
  const x=e.clientX-b.left-b.width/2,y=e.clientY-b.top-b.height/2;
  v.h=Math.round((Math.atan2(x,-y)*180/Math.PI+360)%360);ui();send();
});
$('c0').onclick=()=>{if(!sPR){v.p=0;v.r=0;ui();send();}};
$('z').onclick=()=>{off.p=cur.p;off.r=cur.r;};
$('d').onclick=()=>fetch('/demo');
$('spr').onchange=e=>{sPR=e.target.checked;if(sPR)start();};
$('sh').onchange=e=>{sH=e.target.checked;if(sH)start();};
function start(){
  if(on)return;
  if(typeof DeviceOrientationEvent!=='undefined'&&DeviceOrientationEvent.requestPermission)
    DeviceOrientationEvent.requestPermission().then(s=>{if(s==='granted')listen();});
  else listen();
}
function listen(){
  if(on)return;on=1;
  addEventListener('deviceorientation',e=>{
    if(e.beta===null)return;
    cur.p=90-e.beta;cur.r=e.gamma;
    cur.h=e.webkitCompassHeading!==undefined?e.webkitCompassHeading:(360-(e.alpha||0))%360;
    if(sPR){v.p=cl(cur.p-off.p,-30,30);v.r=cl(cur.r-off.r,-90,90);}
    if(sH)v.h=Math.round(cur.h)%360;
    if(sPR||sH){ui();send();}
  });
}
ui();
</script></body></html>
)rawliteral";

void pt(float th, float R, int &x, int &y) {
  th *= DEG_TO_RAD;
  x = C + R * sinf(th);
  y = C - R * cosf(th);
}

void horizon() {
  float r = roll * DEG_TO_RAD, s = sinf(r), c = cosf(r), p = pitch * K;

  for (int y = yo; y < yo + SH; y++) {
    float a = (y - C) * c - p;
    int gs, ge;
    if (fabsf(s) < 1e-3f) {
      gs = 0;
      ge = a > 0 ? 240 : 0;
    } else {
      int t = constrain((int)(C + a / s), 0, 240);
      if (s > 0) { gs = 0; ge = t; }
      else       { gs = t; ge = 240; }
    }
    HL(0, y, 240, SKY);
    if (ge > gs) HL(gs, y, ge - gs, GND);
  }

  for (int a = -30; a <= 30; a += 5) {
    float v = p - a * K;
    float cx = C - s * v, cy = C + c * v;
    if (hypotf(cx - C, cy - C) > 80) continue;
    float h = (a == 0) ? 90 : (a % 10 == 0 ? 22 : 11);
    LN(cx - c * h, cy - s * h, cx + c * h, cy + s * h, WHT);
    if (a != 0 && a % 10 == 0) {
      CUR(cx + c * h + 4, cy + s * h - 3);
      g->print(abs(a));
    }
  }

  const int ticks[] = {-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60};
  for (int a : ticks) {
    int x0, y0, x1, y1;
    float len = (a % 30 == 0) ? 12 : 7;
    pt(a - roll, 118, x0, y0);
    pt(a - roll, 118 - len, x1, y1);
    LN(x0, y0, x1, y1, WHT);
  }
}

void tape(int x, float val, float px, int step, int lblEvery, bool ticksRight) {
  FR(x, TY, TW, TH, PNL);
  float half = (TH / 2) / px;
  int v0 = (int)floorf((val - half) / step) * step;
  for (int v = v0; v <= val + half; v += step) {
    int y = C - (int)((v - val) * px);
    if (y < TY + 4 || y > TY + TH - 4) continue;
    HL(ticksRight ? x + TW - 8 : x, y, 8, WHT);
    if (v % lblEvery == 0) {
      CUR(ticksRight ? x + 2 : x + 11, y - 3);
      g->print(v);
    }
  }
  FR(x - 2, C - 7, TW + 4, 14, BLK);
  DR(x - 2, C - 7, TW + 4, 14, ORG);
  CUR(x + 6, C - 3);
  g->print((int)val);
}

void headingTape() {
  const int x0 = 62, w = 116, y0 = 198, h = 18;
  FR(x0, y0, w, h, PNL);
  for (int d = (int)hdg - 32; d <= (int)hdg + 32; d++) {
    if (d % 5) continue;
    int x = C + (int)((d - hdg) * 2);
    if (x < x0 + 2 || x > x0 + w - 2) continue;
    int dd = (d + 360) % 360;
    VL(x, y0, dd % 10 == 0 ? 6 : 3, WHT);
    if (dd % 30 == 0 && x > x0 + 8 && x < x0 + w - 8) {
      bool card = dd % 90 == 0;
      int n = dd / 10;
      CUR(x - (card || n < 10 ? 3 : 6), y0 + 8);
      if (card) g->print("NESW"[dd / 90]);
      else      g->print(n);
    }
  }
  FT(C, y0 - 1, C - 4, y0 - 7, C + 4, y0 - 7, ORG);
  char b[5];
  snprintf(b, sizeof(b), "%03d", (int)hdg);
  CUR(C - 9, y0 + h + 3);
  g->print(b);
}

void scene() {
  horizon();
  tape(14, spd, 2.5f, 10, 20, true);
  tape(190, alt, 0.5f, 50, 100, false);
  headingTape();

  FT(C, 20, C - 6, 32, C + 6, 32, ORG);
  FR(55, 118, 40, 5, ORG);
  FR(145, 118, 40, 5, ORG);
  FR(95, 118, 5, 12, ORG);
  FR(140, 118, 5, 12, ORG);
  FC(C, C, 4, ORG);
}

void draw() {
  for (yo = 0; yo < 240; yo += SH) {
    scene();
    tft->draw16bitRGBBitmap(0, yo, g->getFramebuffer(), 240, SH);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!tft->begin(20000000)) Serial.println("tft init failed");
  tft->fillScreen(BLK);

  if (!g->begin(GFX_SKIP_OUTPUT_BEGIN)) {
    Serial.println("strip buffer failed");
    while (1) delay(1000);
  }
  g->setTextSize(1);
  g->setTextColor(WHT);

  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP("PFD", "12345678", 1, 0, 4);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  Serial.println(ok ? "AP started" : "AP FAILED");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/d", []() {
    if (server.hasArg("p")) pitch = server.arg("p").toFloat();
    if (server.hasArg("r")) roll = server.arg("r").toFloat();
    if (server.hasArg("s")) spd = server.arg("s").toFloat();
    if (server.hasArg("a")) alt = server.arg("a").toFloat();
    if (server.hasArg("h")) hdg = server.arg("h").toFloat();
    demo = false;
    server.send(204);
  });
  server.on("/demo", []() {
    demo = true;
    server.send(204);
  });
  server.begin();
}

void loop() {
  server.handleClient();

  if (demo) {
    float t = millis() / 1000.0f;
    pitch = 15 * sinf(t * 0.7f);
    roll = 35 * sinf(t * 0.5f);
    spd = 120 + 15 * sinf(t * 0.3f);
    alt = 1500 + 100 * sinf(t * 0.2f);
    hdg = t * 8;
  }
  hdg = fmodf(fmodf(hdg, 360) + 360, 360);
  draw();
}
