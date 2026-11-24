# ROV Control System – ROS 2

## Overview

A ROS 2-based ROV control and motor simulation project developed as part of an assessment.

The system converts velocity commands into individual motor RPM commands, simulates motor responses, and displays motor telemetry through a terminal dashboard.

## Features

- Velocity command handling through `/cmd_vel`.
- 8-thruster RPM command generation.
- Simulated motor response with rate-limited RPM changes.
- Motor RPM and thrust telemetry.
- Interactive terminal dashboard for control and monitoring.

## System Architecture

| Node | Responsibility |
|---|---|
| `rov_controller` | Converts velocity commands into motor RPM commands |
| `motor_simulator` | Simulates motor response and calculates thrust |
| `rov_dashboard` | Sends velocity commands and displays telemetry |

## ROS 2 Topics

| Topic | Message Type | Purpose |
|---|---|---|
| `/cmd_vel` | `geometry_msgs/msg/Twist` | Velocity input |
| `/motor_rpm_command` | `std_msgs/msg/Float32MultiArray` | Target motor RPM |
| `/motor_rpm` | `std_msgs/msg/Float32MultiArray` | Simulated motor RPM |
| `/motor_thrust` | `std_msgs/msg/Float32MultiArray` | Simulated motor thrust |

## Requirements

- Ubuntu 24.04
- ROS 2 Jazzy
- `colcon`
- C++ compiler and Python 3

## Build

```bash
cd ~/rov_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-select rov_control
source install/setup.bash
```

## Run

Open three separate terminals. Source ROS 2 and the workspace in each terminal:

```bash
source /opt/ros/jazzy/setup.bash
source ~/rov_ws/install/setup.bash
```

**Terminal 1 – Motor Simulator**

```bash
ros2 run rov_control motor_simulator
```

**Terminal 2 – ROV Controller**

```bash
ros2 run rov_control rov_controller
```

**Terminal 3 – Dashboard**

```bash
ros2 run rov_control dashboard.py
```

## Dashboard Commands

Enter commands in the dashboard to control the ROV:

```text
surge 0.5
sway 0.3
heave 0.5
yaw 0.2
stop
help
quit
```

Input values range from `-1.0` to `1.0`.

## Important Note

This project uses a simplified motor simulation. RPM and thrust values are simulated, not measurements from physical thrusters. The thruster mixing configuration is provisional and must be validated against the actual ROV geometry before hardware deployment.

---

Developed as part of a ROS 2 ROV assessment.
