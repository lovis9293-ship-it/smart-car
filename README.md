# PRJCV – Bluetooth Controlled Car with RFID and Obstacle Detection

C firmware for an ESP32, built with ESP-IDF and FreeRTOS. The car is locked and unlocked using an RC522 RFID reader, receives commands over Bluetooth Classic SPP, and stops when an obstacle or a distance measurement error is detected.

## Features

- Starts with the car locked and the motors stopped.
- Scanning an authorized RFID card unlocks the car and enables Bluetooth. Scanning it again locks the car and disables Bluetooth.
- Supports forward, reverse, left, right, stop, and nine speed levels.
- Measures distance while unlocked. A distance of 20 cm or less, or a measurement error, blocks all movement commands.
- Requires a new movement command after an obstacle clears. The car also stops when the Bluetooth client disconnects.
- Keeps the car locked if Bluetooth shutdown fails; a reset is required to recover.

## Hardware and Software Requirements

- An ESP32 with Bluetooth Classic SPP support. Make sure the GPIO assignments below do not conflict with onboard hardware; GPIO16 may be used by PSRAM on some modules.
- Two DC motors and an H-bridge driver with EN/PWM, IN1, and IN2 inputs for each motor.
- An RC522 RFID reader and a compatible card.
- An HC-SR05 ultrasonic sensor, suitable power supplies, and wiring.
- ESP-IDF **5.5.5**, as recorded in `sdkconfig` and `dependencies.lock`.
- `abobija/rc522` **4.0.1**, as recorded in the lockfile.
- A device and application that can send characters over Bluetooth Classic SPP, such as an Android SPP terminal application.

ESP32-S2, ESP32-S3, and ESP32-C3 do not support the Bluetooth Classic SPP functionality used by this project. Although the component manifest specifies IDF `>=4.1.0`, this does not establish compatibility with all those versions: the source uses newer drivers and APIs. Use the recorded version, 5.5.5.

## GPIO Connections

These assignments come from `main/PRJCV.c` and `components/car/car.c`.

| Component | Signal | ESP32 GPIO |
| --- | --- | --- |
| Left motor | EN/PWM | 5 |
| Left motor | IN1 | 22 |
| Left motor | IN2 | 21 |
| Right motor | EN/PWM | 23 |
| Right motor | IN1 | 19 |
| Right motor | IN2 | 18 |
| RC522 | MISO | 16 |
| RC522 | MOSI | 13 |
| RC522 | SCLK | 4 |
| RC522 | SDA/SS (SPI chip select) | 15 |
| RC522 | RST | 2 |
| HC-SR05 | TRIG | 25 |
| HC-SR05 | ECHO | 26 |

Connect all grounds together. Do not power the motors directly from ESP32 GPIO pins. Power the RC522 according to its module specifications, typically 3.3 V. If the ultrasonic sensor produces a 5 V ECHO signal, use a voltage divider or level shifter before GPIO26: ESP32 inputs are not 5 V tolerant. GPIO2, GPIO5, and GPIO15 are ESP32 strapping pins; check peripheral signal levels during boot if the board fails to start or flash.

## Project Structure

```text
.
├── CMakeLists.txt          # PRJCV project definition
├── sdkconfig              # Current ESP-IDF configuration
├── dependencies.lock      # Locked dependency versions
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── PRJCV.c            # Initialization and lock/unlock handling
└── components/
    ├── bluetooth/         # Bluetooth Classic SPP
    ├── car/               # Commands, speed, state, and motor control task
    ├── hcsr05/            # Distance measurements using GPIO interrupts and a queue
    ├── motor/             # Direction GPIOs and LEDC PWM
    ├── rc/                # RC522, UID validation, and RFID events
    └── safety/            # Obstacle monitoring
```

## Building and Flashing

1. Install ESP-IDF **5.5.5** using the [Espressif installation guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32/get-started/index.html).
2. Open a terminal with the ESP-IDF environment activated and navigate to the project directory containing the root `CMakeLists.txt`.
3. Build the firmware:

```sh
idf.py build
```

The `esp32` target, Bluetooth Classic, Bluedroid, and SPP are already configured in `sdkconfig`. ESP-IDF Component Manager downloads the RC522 dependency into `managed_components/`; this directory does not need to be committed to GitHub.

4. Flash the board and view logs. Replace `PORT` with the actual serial port, such as `COM5` on Windows or `/dev/ttyUSB0` on Linux:

```sh
idf.py -p PORT flash monitor
```

Exit the monitor with `Ctrl+]`. To inspect or change the configuration:

```sh
idf.py menuconfig
```

The saved configuration uses **2 MB flash** and the default ESP-IDF **Single factory app** partition table. A separate `partitions.csv` is not required. If the application exceeds the partition size, choose a partition layout that fits the board's actual flash capacity in `menuconfig`, then rebuild.

## Usage

1. Power on the board. The car starts locked.
2. Scan an authorized RFID card. The accepted UID is defined in the `valid_uid` array in `components/rc/rc.c`. Update the array for your card, then rebuild and flash.
3. Connect to the Bluetooth device **ESP32_BT** and open an SPP connection.
4. Send individual characters from the table below; no newline is required. After unlocking, movement commands are accepted only after the sensor reports a valid distance with no obstacle within the stop threshold.
5. Scan the authorized card again to lock the car.

| Character | Action |
| --- | --- |
| `w` | Forward |
| `s` | Reverse |
| `a` | Turn left |
| `d` | Turn right |
| `x` | Stop |
| `1` … `9` | Set speed level; default is 5 |

Speed levels map to PWM duty using `600 + level * 44`, with 10-bit LEDC resolution at 5 kHz. A speed command only changes the PWM level; send `x` to stop. Other characters are ignored. A movement command remains active until another command arrives, the car is locked, the sensor blocks movement, or the Bluetooth connection closes. There is currently no automatic stop timeout for missing commands.

The stop threshold and task delay can be changed in `components/safety/safety.h`: `SAFETY_STOP_CM = 20.0f` and `SAFETY_PERIOD_MS = 70`. The actual interval between measurements also includes sensor measurement and waiting time.

## Hardware Verification

The firmware has not been built or run on an ESP32 in the environment used to prepare this repository. Keep the wheels off the ground during initial testing and check the following:

- On boot, the motors remain stopped and Bluetooth is disabled.
- An unauthorized card does not unlock the car; an authorized card enables Bluetooth.
- Direction and speed commands produce the expected motor behavior for the actual wiring.
- An obstacle at or within 20 cm, or a measurement error, stops the car. Clearing the obstacle does not resume the previous movement command.
- Disconnecting Bluetooth or scanning the card to lock the car stops the motors.
- Repeated lock/unlock cycles initialize and shut down Bluetooth correctly, as shown in the logs.

## Files to Commit

Keep `main/`, `components/`, `CMakeLists.txt`, `sdkconfig`, `dependencies.lock`, `README.md`, and `.gitignore`. The backup configuration `sdkconfig.old` is ignored, along with `build/` and `managed_components/`. The original firmware source and configuration were preserved when preparing this repository.
