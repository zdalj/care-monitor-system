# Care Monitor System

An ESP32-based monitoring system that detects inactivity and potential falls using an accelerometer.

## Features
- Active / Warning / Alert states
- Fall detection using motion patterns
- Buzzer alerts
- Live web dashboard (Flask)

## Technologies
- ESP32 (Arduino)
- Python Flask
- Serial communication

## How it works
- No movement for 30s → WARNING
- No movement for 40s → ALERT
- Sudden impact + stillness → FALL DETECTED

## Demo
(To be added)
