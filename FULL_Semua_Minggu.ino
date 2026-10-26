#include <WiFi.h>
#include <Wire.h>
#include <WebServer.h>
#include <Adafruit_MLX90640.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

Adafruit_MLX90640 mlx;
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);
Preferences pref;

float frame[768];
const int relayPin = 27; // Active High Relay
bool lampu = false;

unsigned long waktuLampuNyala = 0;
unsigned long waktuRelayMati = 0;
bool tungguNyala = false;

// SAFETY TIMER
const unsigned long delayLampuNyala = 120000; // Minimal NYALA 2 Menit (120.000 ms)
const unsigned long delayLampuMati  = 60000;  // Maksimal MATI 1 Menit (60.000 ms)

// Variabel Waktu & Set Point Dinamis
unsigned long totalDetikBerjalan = 0;
unsigned long previousSecMillis = 0;
unsigned long previousSaveMillis = 0;

float batasBawah = 32.0;
float batasAtas  = 35.0;
String labelMinggu = "M1";
int hariKe = 1;

const char* ap_ssid = "Alat_Monitoring_Suhu";
const char* ap_password = "password123";

float suhuMax = 0.0, suhuMin = 0.0, suhuCenter = 0.0, suhuAmbient = 0.0, suhuAvg = 0.0, suhuKontrol = 0.0;
String statusKondisi = "Menunggu...";

unsigned long previousMillis = 0;
const long intervalSensor = 3000; 

// Evaluasi & Pembaruan Minggu Berdasarkan Total Detik
void updateKondisiMinggu() {
  hariKe = (totalDetikBerjalan / 86400UL) + 1; // 1 Hari = 86400 Detik

  if (hariKe <= 7) {
    // Minggu 1 (Hari 1-7)
    batasBawah = 32.0;
    batasAtas  = 35.0;
    labelMinggu = "M1";
  } 
  else if (hariKe <= 14) {
    // Minggu 2 (Hari 8-14)
    batasBawah = 30.0;
    batasAtas  = 32.0;
    labelMinggu = "M2";
  } 
  else {
    // Minggu 3 (Hari 15-21)
    batasBawah = 28.0;
    batasAtas  = 30.0;
    labelMinggu = "M3";
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Monitoring Brooding Otomatis</title>";
  html += "<style>";
  html += "body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif; text-align:center; background:#121212; color:#fff; margin:0; padding:15px;}";
  html += ".container{max-width:400px; margin:0 auto; background:#1e1e1e; padding:20px; border-radius:16px; box-shadow:0 4px 20px rgba(0,0,0,0.6);}";
  html += "h2{margin:5px 0 2px 0; font-size:20px; color:#00adb5;} p.sub{font-size:11px; color:#888; margin:0 0 15px 0;}";
  html += ".canvas-card{background:#000; padding:8px; border-radius:12px; display:inline-block; width:100%; box-sizing:border-box;}";
  html += "canvas{border-radius:8px; width:100%; height:auto; display:block;}";
  html += ".controls{margin:10px 0;} button{background:#333; color:#fff; border:none; padding:6px 14px; border-radius:15px; margin:0 4px; font-size:11px; cursor:pointer;}";
  html += "button.active{background:#00adb5; font-weight:bold;}";
  html += ".btn-reset{background:#e53935 !important; color:#fff !important; font-weight:bold; margin-top:12px; padding:10px 20px; border-radius:8px; width:100%; font-size:13px; border:none; cursor:pointer;}";
  html += ".color-bar{height:10px; border-radius:5px; background:linear-gradient(to right, blue, cyan, green, yellow, red); margin:10px 0 15px 0;}";
  html += ".grid-info{display:grid; grid-template-columns: 1fr 1fr; gap:10px; text-align:center;}";
  html += ".card{background:#2a2a2a; padding:12px; border-radius:10px;}";
  html += ".card-full{grid-column: span 2; background:#242b35; border: 1px solid #00adb5;}";
  html += ".card-title{font-size:11px; color:#aaa; margin-bottom:4px;}";
  html += ".card-value{font-size:18px; font-weight:bold;}";
  html += ".status-card{background:#2a2a2a; padding:12px; border-radius:10px; margin-top:12px; text-align:left; font-size:13px; border-left: 4px solid #00adb5;}";
  html += ".badge{padding:3px 8px; border-radius:10px; font-weight:bold; font-size:11px; display:inline-block;}";
  html += ".bg-on{background:#2e7d32; color:#fff;} .bg-off{background:#c62828; color:#fff;}";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h2>Sistem Brooding (<span id='mGinggu'>-</span>)</h2>";
  html += "<p class='sub'>Kamera Thermal MLX90640 | Hari Ke-<b><span id='hari'>1</span></b></p>";
  
  html += "<div class='canvas-card'><canvas id='thermalCanvas' width='384' height='288'></canvas></div>"; 
  
  html += "<div class='controls'>";
  html += "  <button id='btnSmooth' class='active' onclick='setMode(true)'>Smooth</button>";
  html += "  <button id='btnPixel' onclick='setMode(false)'>Pixel View</button>";
  html += "</div>";

  html += "<div class='color-bar'></div>";

  html += "<div class='grid-info'>";
  html += "  <div class='card card-full'><div class='card-title'>Rata-Rata Suhu Thermal Ayam</div><div class='card-value'><span id='avg'>0.0</span> &deg;C</div></div>";
  html += "  <div class='card'><div class='card-title'>Center</div><div class='card-value'><span id='center'>0.0</span> &deg;C</div></div>";
  html += "  <div class='card'><div class='card-title'>Ambient</div><div class='card-value'><span id='ambient'>0.0</span> &deg;C</div></div>";
  html += "  <div class='card'><div class='card-title'>Minimum</div><div class='card-value'><span id='min'>0.0</span> &deg;C</div></div>";
  html += "  <div class='card'><div class='card-title'>Maximum</div><div class='card-value'><span id='max'>0.0</span> &deg;C</div></div>";
  html += "</div>";

  html += "<div class='status-card'>";
  html += "  <div><b>Target Suhu:</b> <span id='target' style='color:#00adb5;'>-</span></div>";
  html += "  <div style='margin-top:4px;'><b>Status Lingkungan:</b> <span id='status' style='color:#00fff0; font-weight:bold;'>-</span></div>";
  html += "  <div style='margin-top:6px;'><b>Status Lampu:</b> <span id='lampuBadge' class='badge bg-off'>OFF</span></div>";
  html += "</div>";

  html += "<button class='btn-reset' onclick='resetHari()'>RESET KE HARI 1</button>";
  html += "</div>";

  html += "<script>";
  html += "let isSmooth = true;";
  html += "function setMode(smooth){";
  html += "  isSmooth = smooth;";
  html += "  document.getElementById('btnSmooth').className = smooth ? 'active' : '';";
  html += "  document.getElementById('btnPixel').className = !smooth ? 'active' : '';";
  html += "}";

  html += "function resetHari(){";
  html += "  if(confirm('Apakah Anda yakin ingin mereset hitungan ke Hari 1?')){";
  html += "    fetch('/reset').then(res=>res.text()).then(msg=>{ alert(msg); });";
  html += "  }";
  html += "}";

  html += "function updateData(){";
  html += "  fetch('/data').then(res=>res.json()).then(data=>{";
  html += "    document.getElementById('avg').innerText = data.avg.toFixed(2);";
  html += "    document.getElementById('center').innerText = data.center.toFixed(2);";
  html += "    document.getElementById('min').innerText = data.min.toFixed(2);";
  html += "    document.getElementById('max').innerText = data.max.toFixed(2);";
  html += "    document.getElementById('ambient').innerText = data.ambient.toFixed(2);";
  html += "    document.getElementById('status').innerText = data.status;";
  html += "    document.getElementById('mGinggu').innerText = data.minggu;";
  html += "    document.getElementById('hari').innerText = data.hari;";
  html += "    document.getElementById('target').innerText = data.target;";
  html += "    ";
  html += "    let badge = document.getElementById('lampuBadge');";
  html += "    if(data.lampu === 'ON'){";
  html += "      badge.innerText = 'MENYALA (ON)';";
  html += "      badge.className = 'badge bg-on';";
  html += "    } else {";
  html += "      badge.innerText = 'MATI (OFF)';";
  html += "      badge.className = 'badge bg-off';";
  html += "    }";
  html += "    ";
  html += "    let canvas = document.getElementById('thermalCanvas');";
  html += "    let ctx = canvas.getContext('2d');";
  html += "    let minT = data.min;";
  html += "    let maxT = data.max;";
  html += "    if(maxT === minT) maxT += 0.1;";
  html += "    ";
  html += "    let rawW = 32, rawH = 24;";
  html += "    let outW = canvas.width, outH = canvas.height;";
  html += "    let imgData = ctx.createImageData(outW, outH);";
  html += "    ";
  html += "    for(let y=0; y<outH; y++){";
  html += "      for(let x=0; x<outW; x++){";
  html += "        let gx = (x / outW) * (rawW - 1);";
  html += "        let gy = (y / outH) * (rawH - 1);";
  html += "        let gxi = Math.floor(gx);";
  html += "        let gyi = Math.floor(gy);";
  html += "        let tx = gx - gxi;";
  html += "        let ty = gy - gyi;";
  html += "        if(!isSmooth){ tx = 0; ty = 0; }";
  html += "        ";
  html += "        let c00 = data.frame[gyi * rawW + gxi];";
  html += "        let c10 = data.frame[gyi * rawW + Math.min(gxi + 1, rawW - 1)];";
  html += "        let c01 = data.frame[Math.min(gyi + 1, rawH - 1) * rawW + gxi];";
  html += "        let c11 = data.frame[Math.min(gyi + 1, rawH - 1) * rawW + Math.min(gxi + 1, rawW - 1)];";
  html += "        ";
  html += "        let temp = (1-tx)*(1-ty)*c00 + tx*(1-ty)*c10 + (1-tx)*ty*c01 + tx*ty*c11;";
  html += "        let norm = (temp - minT) / (maxT - minT);";
  html += "        if(norm < 0) norm = 0; if(norm > 1) norm = 1;";
  html += "        ";
  html += "        let hue = (1 - norm) * 240;";
  html += "        let rgb = hslToRgb(hue / 360, 1.0, 0.5);";
  html += "        let idx = (y * outW + x) * 4;";
  html += "        imgData.data[idx] = rgb[0];";
  html += "        imgData.data[idx+1] = rgb[1];";
  html += "        imgData.data[idx+2] = rgb[2];";
  html += "        imgData.data[idx+3] = 255;";
  html += "      }";
  html += "    }";
  html += "    ctx.putImageData(imgData, 0, 0);";
  html += "  });";
  html += "}";

  html += "function hslToRgb(h, s, l){";
  html += "  let r, g, b;";
  html += "  if(s == 0){ r = g = b = l; } else {";
  html += "    let q = l < 0.5 ? l * (1 + s) : l + s - l * s;";
  html += "    let p = 2 * l - q;";
  html += "    r = hue2rgb(p, q, h + 1/3);";
  html += "    g = hue2rgb(p, q, h);";
  html += "    b = hue2rgb(p, q, h - 1/3);";
  html += "  }";
  html += "  return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];";
  html += "}";

  html += "function hue2rgb(p, q, t){";
  html += "  if(t < 0) t += 1; if(t > 1) t -= 1;";
  html += "  if(t < 1/6) return p + (q - p) * 6 * t;";
  html += "  if(t < 1/2) return q;";
  html += "  if(t < 2/3) return p + (q - p) * (2/3 - t) * 6;";
  html += "  return p;";
  html += "}";

  html += "setInterval(updateData, 2000);"; 
  html += "updateData();";
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"avg\":" + String(suhuAvg, 2) + ",";
  json += "\"max\":" + String(suhuMax, 2) + ",";
  json += "\"min\":" + String(suhuMin, 2) + ",";
  json += "\"center\":" + String(suhuCenter, 2) + ",";
  json += "\"ambient\":" + String(suhuAmbient, 2) + ",";
  json += "\"status\":\"" + statusKondisi + "\",";
  json += "\"minggu\":\"" + labelMinggu + "\",";
  json += "\"hari\":" + String(hariKe) + ",";
  json += "\"target\":\"" + String(batasBawah, 0) + "-" + String(batasAtas, 0) + " C\",";
  json += "\"lampu\":\"" + String(lampu ? "ON" : "OFF") + "\",";
  json += "\"frame\":[";
  for (int i = 0; i < 768; i++) {
    json += String(frame[i], 1);
    if (i < 767) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleReset() {
  totalDetikBerjalan = 0;
  pref.putULong("runtime", 0);
  updateKondisiMinggu();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Memori Di-reset!");
  lcd.setCursor(0, 1);
  lcd.print("Kembali ke Hari 1");
  
  server.send(200, "text/plain", "Sistem berhasil di-reset kembali ke Hari 1 (M1)!");
}

void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); 
  lampu = false;
  tungguNyala = false;

  Wire.begin(21, 22); // I2C Pins ESP32
  Wire.setClock(400000);

  lcd.init();
  lcd.backlight();
  lcd.print("Memuat Memori...");

  pref.begin("brooding", false);
  totalDetikBerjalan = pref.getULong("runtime", 0);

  updateKondisiMinggu();

  WiFi.softAP(ap_ssid, ap_password);
  
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/reset", handleReset);
  server.begin();

  if(!mlx.begin()) {
    Serial.println("MLX Gagal");
    lcd.clear(); lcd.print("MLX Error");
    while(1);
  }
  
  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_4_HZ);

  lcd.clear();
  lcd.print("Ready: H-"); lcd.print(hariKe);
  lcd.print(" "); lcd.print(labelMinggu);
  delay(2000);
}

void loop() {
  server.handleClient(); 

  unsigned long currentMillis = millis();

  // 1. PENGHITUNG DETIK AKUMULASI WAKTU BERJALAN
  if (currentMillis - previousSecMillis >= 1000) {
    previousSecMillis = currentMillis;
    totalDetikBerjalan++;
    updateKondisiMinggu();
  }

  // 2. SIMPAN TOTAL WAKTU KE MEMORI SETIAP 10 DETIK
  if (currentMillis - previousSaveMillis >= 10000) {
    previousSaveMillis = currentMillis;
    pref.putULong("runtime", totalDetikBerjalan);
  }
  
  // 3. LOGIKA PEMBACAAN SENSOR & KONTROL HEATER DENGAN FILTER SUHU
  if (currentMillis - previousMillis >= intervalSensor) {
    previousMillis = currentMillis;

    if(mlx.getFrame(frame) != 0) {
      Serial.println("Frame Error!");
      suhuMax = 30.0; suhuMin = 29.0; suhuCenter = 29.5; suhuAvg = 29.5;
    } 
    else {
      suhuAmbient = mlx.getTa(true); 

      suhuMax = frame[0];
      suhuMin = frame[0];
      
      float totalSuhuAyam = 0.0;
      int jumlahPikselAyam = 0;

      // THRESHOLD FILTER UNTUK TANGKAPAN SUHU ANAK AYAM
      float minAyam = 31.0; // Mengabaikan alas dingin
      float maxAyam = 39.0; // Mengabaikan pantulan/radiasi lampu pemanas

      for(int i = 0; i < 768; i++) {
        // TMin dan TMax murni dicari dari seluruh 768 piksel
        if(frame[i] > suhuMax) suhuMax = frame[i];
        if(frame[i] < suhuMin) suhuMin = frame[i];

        // Rata-rata hanya memproses piksel dalam rentang tubuh anak ayam
        if(frame[i] >= minAyam && frame[i] <= maxAyam) {
          totalSuhuAyam += frame[i];
          jumlahPikselAyam++;
        }
      }
      
      if(jumlahPikselAyam > 0) {
        suhuAvg = totalSuhuAyam / jumlahPikselAyam;
      } else {
        suhuAvg = frame[399]; // Default jika ayam di luar jangkauan
      }

      suhuCenter = frame[399];
      suhuKontrol = suhuAvg; 
    }

    if (tungguNyala) {
      if (millis() - waktuRelayMati >= delayLampuMati) {
        tungguNyala = false; 
      }
    }

    if (!tungguNyala && suhuKontrol < batasAtas && !lampu) {
      lampu = true;
      digitalWrite(relayPin, HIGH); 
      waktuLampuNyala = millis();   
    }

    if (suhuKontrol >= batasAtas && lampu) {
      if (millis() - waktuLampuNyala >= delayLampuNyala) { 
        lampu = false;
        digitalWrite(relayPin, LOW); 
        waktuRelayMati = millis();  
        tungguNyala = true;          
      }
    }

    if (suhuKontrol < batasBawah) {
      statusKondisi = "Cold Stress";
    } else if (suhuKontrol >= batasAtas) {
      statusKondisi = "Heat Stress";
    } else {
      statusKondisi = "Nyaman";
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("H"); lcd.print(hariKe);
    lcd.print(" Avg:"); lcd.print(suhuAvg, 1); lcd.print((char)223); lcd.print("C");
    
    lcd.setCursor(0, 1);
    if (suhuKontrol < batasBawah) {
      lcd.print("Sts: Cold ");
    } else if (suhuKontrol >= batasAtas) {
      lcd.print("Sts: Heat ");
    } else {
      lcd.print("Sts:Nyaman ");
    }
    
    lcd.print(labelMinggu);
    lcd.print(lampu ? " L:ON" : " L:OF");
  }
}