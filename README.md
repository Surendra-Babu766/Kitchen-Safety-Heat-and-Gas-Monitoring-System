<h1 align="center">🔥 LPC2148 Kitchen Safety – Heat & Gas Monitoring System</h1>

<p align="center">
  A robust, password-protected real-time kitchen safety and hazard monitoring system powered by the <b>NXP LPC2148 (ARM7)</b> microcontroller.<br/>
  Features continuous <b>temperature & gas sensing</b>, <b>hardware RTC timestamping</b>, <b>external interrupt handling</b>, and a <b>secure configuration menu</b>.
</p>

<p align="center">
  <img alt="MCU" src="https://img.shields.io/badge/MCU-NXP%20LPC2148%20(ARM7TDMI)-blue?style=for-the-badge&logo=microchip"/>
  <img alt="IDE" src="https://img.shields.io/badge/IDE-Keil%20%C2%B5Vision-orange?style=for-the-badge"/>
  <img alt="Language" src="https://img.shields.io/badge/Language-Embedded%20C-green?style=for-the-badge"/>
  <img alt="Platform" src="https://img.shields.io/badge/Platform-Vector%20India%20ARM7%20Board-purple?style=for-the-badge"/>
</p>

---

## 📸 Hardware Prototype Showcase

<p align="center">
  <img src="assets/hardware_setup.jpeg" width="700" alt="Hardware Prototype on Vector India ARM7 Board">
</p>
<p align="center"><em>Fully assembled physical prototype featuring the LPC2148 board, 16x2 LCD display, 4x4 matrix keypad, LM35 temperature sensor, MQ-2 gas sensor module, and active-high alert LED/buzzer system.</em></p>

---

## 📑 Table of Contents

1. [Overview & Objectives](#-overview--objectives)
2. [System Architecture & Diagrams](#-system-architecture--diagrams)
3. [System File Structure](#-system-file-structure)
4. [Hardware & Pin Configuration](#-hardware--pin-configuration)
5. [Core Features & Functional Workflow](#-core-features--functional-workflow)
6. [Software Requirements & Compilation](#-software-requirements--compilation)
7. [Author](#-author)

---

## 🎯 Overview & Objectives

Designed as an advanced embedded systems project[cite: 25], this application provides automated protection against kitchen hazards by monitoring thermal and atmospheric conditions:
* **Temperature Monitoring:** Continuously tracks ambient kitchen temperature via an **LM35** analog sensor connected to the on-chip ADC.
* **Gas Leakage Detection:** Senses combustible gases (LPG, smoke, methane) using an **MQ-2** gas sensor module[cite: 25, 26].
* **Multi-Modal Alerting:** Instantly activates an audible buzzer (`P1.27`) and a visual active-high alert LED (`P0.0`) upon threshold breaches.
* **Event Logging:** Records historical threshold-crossing events with precise hardware **RTC timestamps** and cycles them on the LCD screen every 10 seconds.

---

## 📊 System Architecture & Diagrams

### 1. System Block Diagram
The following block diagram illustrates the peripheral connections and core interfaces centered around the LPC2148 ARM7 microcontroller[cite: 25]:

<p align="center">
  <img src="assets/block_diagram.png" width="600" alt="LPC2148 Kitchen Safety Block Diagram">
</p>

### 2. System Workflow Flowchart
The execution flow governing real-time sensor sampling, interrupt response, password validation, and hazard management is detailed below:

```mermaid
graph TD
    A[Start: System Initialization] --> B[Display Welcome Banner & RTC Init]
    B --> C[Continuous Monitoring Loop]
    C --> D[Sample LM35 & MQ-2 via ADC Channels]
    D --> E[Fetch Hardware RTC Time & Date]
    E --> F{Check Hazards: Temp > Thresh OR Gas > Thresh?}
    
    F -- Yes --> G{Is Alert Silenced & No Higher Spike?}
    G -- No --> H[Turn ON Buzzer & Alert LED P0.0]
    H --> I[Record Peak Event Timestamp & Sensor Values]
    G -- Yes --> J[Maintain Muted State]
    
    F -- No --> K[Turn OFF Buzzer & Alert LED]
    K --> L[Reset Alert Silenced Flag]
    
    C --> M{Switch 1 Pressed? EINT1 Interrupt}
    M -- Yes --> N[Enter Secure Edit Mode]
    N --> O[Prompt Password via 4x4 Keypad]
    O --> P{Password Correct?}
    P -- No --> Q{Attempts < 3?}
    Q -- Yes --> O
    Q -- No --> R[10-Second Lockout Countdown & System Locked]
    P -- Yes --> S[Display Configuration Menu: RTC, Setpoint, Pass, Exit]
    S --> T[Update Parameters & Return to Monitoring]
    
    C --> U{Switch 2 Pressed? EINT0 Interrupt}
    U -- Yes --> V[Silence Buzzer & Turn OFF LED Immediately]
    
    C --> W[Update LCD Live Screen & 10s Event Log Timer]
    W --> C
