# AquaEmi_IoT
# Overview
Arduino board có khả năng đo đạc các dữ kiện như nồng độ pH nước, lưu lượng nước, và nồng độ hòa tan chất rắn của nước, và gửi đến server thông qua MQTT, đồng thời ước lượng chất lượng nước. 

# Design
Thiết bị sử dụng:
- ESP32-WROOM
- TDS Meter V1.0
- Logo-Rnaeaor V2.0 (pH sensor)
- Yf-s401 Streamflow sensor
- Ds18b20 Underwater Temperature Sensor
  
ESP32 sẽ đo đạc các thông số mỗi 40ms và lấy trung bình mỗi 1s để tính WQI. Ngoài ra, mỗi 10s ESP32 sẽ gửi đo đạc cho hệ thống MQTT. 

# Hướng dẫn sử dụng:
1. Sử dụng Arduino IDE và tải MQTT Arduino Client
2. Chỉnh sửa các pin và chi tiết wifi, mqtt.
3. Upload vào arduino

