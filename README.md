# Wireless_Controller

-Selfmade model railload wireless controller-

<ol>
<li>Introduction<br>
   This is microprocessor ESP32 based model railload wireless controller. <br>
   ESP32 is a microprocessor developed by Espressif Systems, with built-in Wi-Fi and Bluetooth, low cost, low power consumption, and other features.
Although there are many variations of the ESP32, we recommend using a development board that uses the ESP-WROOM-32D (e.g. ESP32-DevKitC-32D).
The ESP32-DevKitC-32D can be purchased on Ebay, Aliexpress, etc. starting at $4. At this time, purchase one with a built-in antenna (Fig. 1).<br>
   <img src="./image/esp32.png" width=300><br>
    Fig.1<br><br>
  This controller are build by Parant unit and Child unit. Both unit includes ESP32 and are connected by Wifi. So to program Parent unit and Child unit, you have to prepare Arduino development enviloment and add esp32 configration.<br>

<li>Parent Unit<br>
   Parent unit is build by ESP32 and moterdriver mainly.<br>
   This is Parent unit schematic.<br>
   <img src="./image/cont_p.png" width=300><br>
   Parent Unit's program is wpower_p.ino, you should modify Wifi-SSID and password at wpowerp.ino.<br>

<li>Child Unit
   
</ol>
