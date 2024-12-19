# SmartPill Ingestible Sensor

**Date:** September 20, 2024

## Overview
The SmartPill project is a proof-of-concept ingestible sensor device designed to capture, log, and transmit key biometric data as it travels through the digestive tract. Leveraging the ESP32 microcontroller platform and the ESP-IDF framework, this prototype focuses on continuous data acquisition, conversion into engineering units, and periodic reporting to a serial console. It simulates functionality that could serve as the foundation for advanced, non-invasive diagnostic tools in the future.

## Key Features

- **Temperature Monitoring:**  
  Measures internal temperature and reports it in °C or °F, ensuring precise thermal profiling.

- **Light Intensity Measurement:**  
  Utilizes a calibrated photocell to measure illumination levels (Lux), informing the device’s state transitions as it moves into darker or brighter environments.

- **Battery Voltage Monitoring:**  
  Employs a stable voltage divider and ADC readings to continuously track battery health, ensuring the device operates reliably throughout its journey.

- **Tilt Orientation Detection:**  
  Reports tilt as an on/off state, providing insight into the device’s orientation and helping characterize its position in real-time.

- **LED Status Indicators:**  
  - **Green LED:** “Ready-to-swallow” mode, activated when ambient light is detected.  
  - **Blue LED:** Sensing mode, active in darkness and blinking every 2 seconds to indicate ongoing measurements.  
  - **Red LED:** Completion signal, lighting up once ambient light is detected again, signifying that the sensing routine has ended.

## Operational Workflow

1. **Startup:**  
   Upon power-up or button press, the SmartPill enters “ready-to-swallow” mode with the Green LED lit, indicating it’s primed for deployment.

2. **Sensing Cycle:**  
   When ambient light falls below a certain threshold, the system transitions to sensing mode. Every 2 seconds, it:
   - Reads temperature, light levels, battery voltage, and tilt orientation.
   - Converts raw values into engineering units.
   - Logs all data to the console.
   - Flashes the Blue LED to confirm active measurements.

3. **Completion State:**  
   Once ambient light rises above the threshold, the SmartPill concludes its sensing routine. The Red LED activates, indicating that data collection is complete and the device has effectively “exited” its operational environment.

## Implementation Details

- **Hardware Integration:**  
  Sensors, LEDs, and a voltage divider are connected to the ESP32’s GPIO and ADC pins. The thermistor, photocell, and orientation sensor (or switch) are arranged for efficient data sampling and stable measurements.

- **Software Structure:**  
  Using the ESP-IDF framework, sensor readings occur on a fixed schedule, ensuring consistent data intervals. A state machine dictates transitions between modes. Periodic tasks manage LED updates, input checks, and data reporting, providing clear functional separation and predictable behavior.

## Results and Learnings

- **Achievements:**
  - Reliable, periodic multi-sensor data acquisition.
  - Clear and consistent LED feedback, aligning with each operational state.
  - Accurate reporting in engineering units directly to the serial console.

- **Challenges:**  
  - Fine-tuning the light threshold to accommodate varying ambient conditions required iterative testing.
  - Achieving stable tilt detection presented obstacles due to hardware limitations and calibration constraints.

- **Improvements:**
  - Implement hardware interrupts for button operations to reduce processor load compared to polling.
  - Streamline wiring and reduce complexity in the circuit for improved reliability and easier maintenance.

## Future Directions

- **Advanced Sensors:**  
  Integrate more sensitive and accurate temperature or orientation sensors to enhance data fidelity.

- **Wireless Connectivity:**  
  Incorporate wireless communication (e.g., BLE or Wi-Fi) to enable remote data monitoring and retrieval without physical connections.

- **Miniaturization and Materials:**  
  Explore smaller form factors, flexible PCB designs, and potentially biodegradable materials to move closer to a practical, clinically relevant ingestible device.

## Demonstrations

- [**Device Functionality Demo Video**](https://drive.google.com/file/d/11GS_PD7nRqs_4I9aimhMB19hdrCAkj27/view?usp=sharing)
- [**Design Walkthrough Video**](https://drive.google.com/file/d/1CU57YWfw8BjZGP4tDwL273PEkrWFR-8J/view?usp=sharing)

---

This prototype highlights the viability of ingestible sensor technology, emphasizing stable data acquisition, user feedback via LEDs, and robust state management. While currently a proof-of-concept, the SmartPill project sets the stage for future innovations in non-invasive medical diagnostics, pushing closer to a new era of real-time, patient-friendly health monitoring.
