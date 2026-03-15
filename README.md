[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/-Acvnhrq)

# Final Project

**Team Number: T2**

**Team Name:** 

| Team Member Name | Email Address           |
| ---------------- | ----------------------- |
| Marko Mijatovic  | markomij@seas.upenn.edu |
| Devan Malik      | devmalik@seas.upenn.edu |
| Kim Huang        | huangkim@seas.upenn.edu |
| Victor Wanjohi   | vwanjohi@seas.upenn.edu |


**GitHub Repository URL:  [https://github.com/upenn-embedded/final-project-s26-t2]([https://github.com/upenn-embedded/final-project-s26-t2]())**

**GitHub Pages Website URL:** [for final submission] 

## Final Project Proposal

### 1. Abstract

This project is a self-contained infrared laser tag system built around two ATmega328PB microcontrollers running bare-metal C. The system consists of a handheld blaster that transmits coded IR packets, a wearable vest that detects and validates hits with audiovisual feedback, and a Bluetooth-linked web application that serves as a referee console for match control and live scoring. The blaster features an LCD display (SPI) for ammo tracking, an MPU6050 accelerometer (I2C) enabling a shake-to-reload gesture, and sound effects via a piezo buzzer. The vest uses multiple IR receivers for coverage, RGB LEDs for hit and health feedback, and communicates match state to the web app over BLE. Together, the system enables a full match loop: start, play, elimination, winner declaration, and reset.

### 2. Motivation

Laser tag is a proven recreational concept, but commercial systems are expensive, proprietary, and offer no insight into the engineering behind them. This project recreates a fully functional laser tag experience from scratch using embedded systems principles, providing a hands-on application of IR communication, real-time firmware state machines, serial peripherals, and wireless connectivity. The project is interesting because it integrates multiple complex subsystems (shooting, hit detection, game logic, wireless comms, and a user-facing web interface) into a single playable product. The intended purpose is a two-player laser tag demo that can run a complete match cycle in under two minutes, showcasing real-time embedded system design at the ESE 3500 final demo day.

### 3. System Block Diagram

![1773617645851](image/README/1773617645851.png)

The system is composed of three subsystems communicating through IR and BLE:



Blaster (MCU 1 — ATmega328PB): Trigger and reload buttons (GPIO with internal pull-ups and pin-change interrupts), IR LED emitter driven through an NPN transistor (2N2222) switching 100 mA, modulated at 38 kHz via Timer1 output compare, SPI-driven ST7735 LCD breakout (128x160, onboard 3.3V level shifter), MPU6050 IMU breakout over I2C at 400 kHz (onboard LDO and level shifter for 5V-safe operation), piezo buzzer (Timer0 PWM), and a shoot-indicator LED with 220-ohm current-limiting resistor. Powered by a USB power bank providing 5V directly (no voltage regulator needed). 100 nF decoupling capacitors on MCU VCC pins.



Vest (MCU 2 — ATmega328PB): Two TSOP38238 IR receivers (5V-tolerant output, connected via pin-change interrupts for hit detection), WS2812B RGB LED strip (5V, 800 kHz data line), piezo buzzer (Timer2 PWM), and an HM-10 BLE module (3.3V logic) connected through a bidirectional logic level shifter on UART TX/RX lines at 9600 baud. The vest MCU runs game state logic (health tracking, invulnerability windows, elimination detection) and reports status to the web app via BLE. Powered by a USB power bank providing 5V directly. 100 nF decoupling capacitors on MCU VCC pins.



Web App (Referee Console): A browser-based application using the Web Bluetooth API that connects to the vest's BLE module. Provides match start/reset commands and displays live player health, ammo (relayed), hit events, and winner/loser at match end.

Communication Protocols

IR (Gun → Vest): 38 kHz modulated carrier with NEC-style packet encoding (preamble + shooter ID + team ID + CRC)

SPI (MCU 1 → LCD): ST7735 display driven over hardware SPI

I2C (MCU 1 → MPU6050): Accelerometer/gyroscope for shake-to-reload gesture detection on the blaster

UART (MCU 2 → HM-10 BLE): Serial link at 9600 baud through a bidirectional 5V/3.3V logic level shifter

GPIO + Interrupts: Button inputs (trigger, reload), IR receiver hit detection via pin-change interrupts

Topics Covered

Timers: PWM for IR carrier generation (38 kHz), buzzer tones, and LED brightness/patterns

Interrupts: Button debouncing, IR receiver packet detection, timer overflow for game timing

Serial Communication (SPI + I2C + UART): LCD display over SPI, MPU6050 accelerometer over I2C, BLE module over UART (satisfies I2C/SPI requirement with both protocols)

(Advanced) Wireless Communication: IR shot protocol design + BLE connectivity for remote match control

### 4. Design Sketches

#### Blaster

Handheld enclosure (3D printed or laser-cut) housing the ATmega328PB on perfboard, IR LED at the barrel tip driven by a 2N2222 NPN transistor, trigger and reload buttons on the grip, an ST7735 LCD breakout on the back face visible to the shooter, an MPU6050 IMU breakout mounted inside for shake-to-reload detection, a piezo buzzer, and a shoot-indicator LED. Powered by a USB power bank (5V direct, no regulator needed). Decoupling capacitors on all IC power pins.

#### Vest

A fabric vest or harness with 3D-printed/laser-cut mounting points for two IR receivers (front and back), WS2812B RGB LEDs for hit feedback visible to other players, a piezo buzzer for audio cues, and a small enclosure for the ATmega328PB, bidirectional level shifter, and HM-10 BLE module. Powered by a USB power bank (5V direct).

#### Manufacturing

3D printing (Tangen Hall / RPL): blaster enclosure, vest sensor mounts

Laser cutting (RPL): potential flat panels for enclosure sides, vest backing plate

### 5. Software Requirements Specification (SRS)

**5.1 Definitions, Abbreviations**

Here, you will define any special terms, acronyms, or abbreviations you plan to use for hardware

**5.2 Functionality**

| ID     | Description                                                                                                                                                                                                              |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| SRS-01 | The IMU 3-axis acceleration will be measured with 16-bit depth every 100 milliseconds +/-10 milliseconds                                                                                                                 |
| SRS-02 | The distance sensor shall operate and report values at least every .5 seconds.                                                                                                                                           |
| SRS-03 | Upon non-nominal distance detected (i.e., the trap mechanism has changed at least 10 cm from the nominal range), the system shall be able to detect the change and alert the user in a timely manner (within 5 seconds). |
| SRS-04 | Upon a request from the user, the system shall get an image from the internal camera and upload the image to the user system within 10s.                                                                                 |

### 6. Hardware Requirements Specification (HRS)

**6.1 Definitions, Abbreviations**

Here, you will define any special terms, acronyms, or abbreviations you plan to use for hardware

**6.2 Functionality**

| ID     | Description                                                                                                                        |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------- |
| HRS-01 | A distance sensor shall be used for obstacle detection. The sensor shall detect obstacles at a maximum distance of at least 10 cm. |
| HRS-02 | A noisemaker shall be inside the trap with a strength of at least 55 dB.                                                           |
| HRS-03 | An electronic motor shall be used to reset the trap remotely and have a torque of 40 Nm in order to reset the trap mechanism.      |
| HRS-04 | A camera sensor shall be used to capture images of the trap interior. The resolution shall be at least 480p.                       |

### 7. Bill of Materials (BOM)

### 8. Final Demo Goals

### 9. Sprint Planning

| Milestone  | Functionality Achieved | Distribution of Work |
| ---------- | ---------------------- | -------------------- |
| Sprint #1  |                        |                      |
| Sprint #2  |                        |                      |
| MVP Demo   |                        |                      |
| Final Demo |                        |                      |

**This is the end of the Project Proposal section. The remaining sections will be filled out based on the milestone schedule.**

## Sprint Review #1

### Last week's progress

### Current state of project

### Next week's plan

## Sprint Review #2

### Last week's progress

### Current state of project

### Next week's plan

## MVP Demo

## Final Report

Don't forget to make the GitHub pages public website!
If you’ve never made a GitHub pages website before, you can follow this webpage (though, substitute your final project repository for the GitHub username one in the quickstart guide):  [https://docs.github.com/en/pages/quickstart](https://docs.github.com/en/pages/quickstart)

### 1. Video

### 2. Images

### 3. Results

#### 3.1 Software Requirements Specification (SRS) Results

| ID     | Description                                                                                               | Validation Outcome                                                                          |
| ------ | --------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| SRS-01 | The IMU 3-axis acceleration will be measured with 16-bit depth every 100 milliseconds +/-10 milliseconds. | Confirmed, logged output from the MCU is saved to "validation" folder in GitHub repository. |

#### 3.2 Hardware Requirements Specification (HRS) Results

| ID     | Description                                                                                                                        | Validation Outcome                                                                                                      |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| HRS-01 | A distance sensor shall be used for obstacle detection. The sensor shall detect obstacles at a maximum distance of at least 10 cm. | Confirmed, sensed obstacles up to 15cm. Video in "validation" folder, shows tape measure and logged output to terminal. |
|        |                                                                                                                                    |                                                                                                                         |

### 4. Conclusion

## References
