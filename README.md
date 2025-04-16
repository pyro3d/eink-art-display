# _Art Display_
<img src="https://raw.githubusercontent.com/pyro3d/eink-art-display/refs/heads/main/display.jpg" width=600>

An ESP32-S3 and Waveshare 13.3 Spectra-6 art display. Connects to Home Assistant via MQTT for config and updates. 
## _Needed Config_
In menuconfig, make sure to set these variables under _Project Configuration_
 - `SSID` 
 - `WIFI_PASSWORD` 
 - `MQTT_BROKER` 
 - `MQTT_USERNAME` 
 - `MQTT_PASSWORD`
 - `ART_CONVERTER_URL`