// Definisi Pin Relay di ESP32
const int RELAY_PIN = 23;

// Catatan: Sebagian besar modul relay bersifat "Active LOW"
// Low = Relay ON (Lampu Nyala)
// HIGH = Relay OFF (Lampu Mati)
// Ubah logika jika modul relay kamu "Active HIGH".
const int RELAY_ON  = LOW;
const int RELAY_OFF = HIGH;

void setup() {
  // Inisialisasi Serial Monitor untuk pemantauan karakterisasi
  Serial.begin(115200);
  
  // Atur pin relay sebagai OUTPUT
  pinMode(RELAY_PIN, OUTPUT);
  
  // Posisi awal: Matikan relay/lampu
  digitalWrite(RELAY_PIN, RELAY_OFF);
  
  Serial.println("--- PENGUJIAN KARAKTERISASI RELAY & LAMPU ---");
  delay(1000);
}

void loop() {
  // 1. Uji Nyala
  Serial.println("[STATUS] Relay ON -> Lampu HARUS NYALA (Relay berbunyi 'KLIK')");
  digitalWrite(RELAY_PIN, RELAY_ON);
  delay(3000); // Tahan nyala selama 3 detik

  // 2. Uji Mati
  Serial.println("[STATUS] Relay OFF -> Lampu HARUS MATI");
  digitalWrite(RELAY_PIN, RELAY_OFF);
  delay(3000); // Tahan mati selama 3 detik
}