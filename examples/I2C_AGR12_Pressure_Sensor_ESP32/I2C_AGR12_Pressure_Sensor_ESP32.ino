#include <Wire.h>

/*
    Y251216: Test I2C AGR12 Pressure Sensor 0~100kPa ASAIR với Vietduino ESP32
        - Clock I2C 50kHz
        - Với data từ I2C, giá trị áp suất đã được tính toán trong cảm biến nên code này đọc cho cả 3 loại ngưỡng áp suất.
            - I2C AGR12100C00 (0~100kPa): ~6.5kPA (Bơm chỗ Quí)
            - I2C AGR12010C00 (0~10kPa): ~6.5kPA (Bơm chỗ Quí)

            - chạy OK với Vietduino ESP32 (có tích hợp Logic convert I2C 3V3-5V)

        - Kết nối dây:
            - Vietduino ESP32 -----------------------Sensor
                  5V      ----------------------------- 1
                  GND     ----------------------------- 2
                  SCL     ----------------------------- 3
                  SDA     ----------------------------- 4
*/

// Định nghĩa I2C Constants
const uint8_t AGR12_I2C_ADDRESS = 0x50; // Địa chỉ I2C 7-bit (từ 0xA0/0xA1)
const uint8_t CMD_MEASURE_HIGH = 0xAC; // Byte lệnh 1 [8]
const uint8_t CMD_MEASURE_LOW = 0x12;  // Byte lệnh 2 [8]
const int WAIT_TIME_MS = 80;           // Thời gian chờ sau khi gửi lệnh đo (ms) [8]

void setup() {
  Serial.begin(115200);
  // Wire.begin(); // Khởi tạo I2C bus
  i2c_50Khz();
  Serial.println("Khoi tao cam bien AGR12 I2C...");
}

void loop() {
  if (readPressure()) {
    // Đã đọc thành công, kết quả được in trong hàm readPressure
  } else {
    Serial.println("Loi doc cam bien.");
  }
  delay(1000); // Đọc mỗi 1 giây
}

bool readPressure() {
  // BƯỚC 1: Gửi lệnh đo lường (0xAC 0x12) [8]
  Wire.beginTransmission(AGR12_I2C_ADDRESS);
  Wire.write(CMD_MEASURE_HIGH); 
  Wire.write(CMD_MEASURE_LOW); 
  uint8_t error = Wire.endTransmission(); 

  if (error != 0) {
    // Xử lý lỗi truyền I2C nếu cần
    Serial.print("Loi I2C khi gui lenh: ");
    Serial.println(error);
    return false;
  }
  
  // BƯỚC 2: Chờ 80ms để cảm biến hoàn tất phép đo [8]
  delay(WAIT_TIME_MS);

  // BƯỚC 3: Yêu cầu đọc 3 byte dữ liệu (DATA0, DATA1, CRC) [8]
  uint8_t tempCount = Wire.requestFrom(AGR12_I2C_ADDRESS, 4);
  
  Serial.println("count: " + String(tempCount));
  if (tempCount >= 3) {
    uint8_t noData = Wire.read(); 
    uint8_t data0 = Wire.read(); // Byte dữ liệu áp suất cao (DATA0) [8]
    uint8_t data1 = Wire.read(); // Byte dữ liệu áp suất thấp (DATA1) [8]
    uint8_t crc = Wire.read();   // Byte CRC kiểm tra [8]

    // BƯỚC 4: Tính toán và kiểm tra CRC
    // CRC là kết quả của DATA0 XOR DATA1 [24, Bảng 5]
    uint8_t calculated_crc = data0 ^ data1; 
    
    if (calculated_crc != crc) {
      Serial.print("Loi CRC! Du lieu khong hop le. CRC nhan: ");
      Serial.print(crc, HEX);
      Serial.print(", CRC tinh: ");
      Serial.println(calculated_crc, HEX);
      // return false;
    }

    // BƯỚC 5: Chuyển đổi dữ liệu thành giá trị áp suất 
    
    // Ghép 2 byte dữ liệu thành giá trị 16 bit (kPa * 10)
    // kPa = (DATA0 << 8) | DATA1; [10]
    uint16_t raw_pressure_data = (data0 << 8) | data1;
    
    // Ép kiểu thành số nguyên có dấu 16 bit (Signed Short Int) 
    // để xử lý áp suất âm (nếu có, ví dụ AGR12xxPxx hoặc AGR12xxNxx) [10]
    int16_t signed_raw_data = (int16_t)raw_pressure_data; 
    
    // Chia cho 10.0 để có giá trị áp suất thực tế (kPa) [10]
    float pressure_kPa = (float)signed_raw_data / 10.0;
    
    // BƯỚC 6: Hiển thị kết quả
    Serial.print("Raw: ");
    Serial.print(signed_raw_data);
    Serial.print(" (0x");
    Serial.print(data0, HEX);
    Serial.print(data1, HEX);
    Serial.print(") | Ap suat: ");
    Serial.print(pressure_kPa, 1);
    Serial.println(" kPa");

    return true;

  } else {
    Serial.println("Khong nhan du du lieu (yeu cau 3 byte). " + String(tempCount));
    return false;
  }
}

void i2c_50Khz()
{
  // *** THIẾT LẬP TỐC ĐỘ I2C ***
  Wire.setClock(50000); 
  Wire.begin(); 
  
  Serial.println("Khoi tao cam bien AGR12 I2C voi toc do 50 kHz...");
}