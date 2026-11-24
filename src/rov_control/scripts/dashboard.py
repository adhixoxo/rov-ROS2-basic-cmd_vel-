#!/usr/bin/env python3

import curses
import time

import rclpy
from rclpy.node import Node

from geometry_msgs.msg import Twist
from std_msgs.msg import Float32MultiArray


class ROVDashboard(Node):

    def __init__(self):
        super().__init__("rov_dashboard")

        self.command_pub = self.create_publisher(
            Twist, "/cmd_vel", 10
        )

        self.rpm_sub = self.create_subscription(
            Float32MultiArray,
            "/motor_rpm",
            self.rpm_callback,
            10
        )

        self.thrust_sub = self.create_subscription(
            Float32MultiArray,
            "/motor_thrust",
            self.thrust_callback,
            10
        )

        self.rpm = [0.0] * 8
        self.thrust = [0.0] * 8

        self.command = Twist()

        # Continuously publish commands at 10 Hz.
        self.timer = self.create_timer(
            0.1, self.publish_command
        )

        self.get_logger().info("ROV Dashboard started")

    def publish_command(self):
        self.command_pub.publish(self.command)

    def rpm_callback(self, msg):
        if len(msg.data) == 8:
            self.rpm = list(msg.data)

    def thrust_callback(self, msg):
        if len(msg.data) == 8:
            self.thrust = list(msg.data)

    def set_axis(self, axis, value):
        axes = {
            "surge": (self.command.linear, "x"),
            "sway": (self.command.linear, "y"),
            "heave": (self.command.linear, "z"),
            "roll": (self.command.angular, "x"),
            "pitch": (self.command.angular, "y"),
            "yaw": (self.command.angular, "z"),
        }

        if axis not in axes:
            return False

        vector, component = axes[axis]

        setattr(vector, component, max(-1.0, min(1.0, value)))

        return True

    def stop(self):
        self.command = Twist()



def draw_dashboard(screen, node, command_text, message):
    screen.erase()

    height, width = screen.getmaxyx()

    def write(y, x, text):
        if 0 <= y < height and 0 <= x < width:
            try:
                screen.addnstr(y, x, text, max(0, width - x - 1))
            except curses.error:
                pass

    write(0, 2, "========== ROV CONTROL DASHBOARD ==========")

    write(2, 2, "VELOCITY COMMANDS")
    write(3, 4, f"Surge : {node.command.linear.x:+.2f} m/s")
    write(4, 4, f"Sway  : {node.command.linear.y:+.2f} m/s")
    write(5, 4, f"Heave : {node.command.linear.z:+.2f} m/s")
    write(6, 4, f"Roll  : {node.command.angular.x:+.2f} rad/s")
    write(7, 4, f"Pitch : {node.command.angular.y:+.2f} rad/s")
    write(8, 4, f"Yaw   : {node.command.angular.z:+.2f} rad/s")

    write(10, 2, "MOTOR TELEMETRY")
    write(11, 2, "Motor       RPM          Estimated Thrust")

    for i in range(8):
        write(
            12 + i, 2,
            f"M{i + 1:<10} {node.rpm[i]:>8.1f} RPM"
            f"       {node.thrust[i]:>10.2f}"
        )

    write(20, 2, "COMMANDS: surge 0.5 | sway -0.3 | heave 0.2")
    write(21, 2, "          roll 0.2 | pitch 0.2 | yaw 0.5")
    write(22, 2, "          stop | help | quit")
    write(23, 2, f"rov> {command_text}")

    # Keep cursor within the terminal boundaries.
    cursor_y = min(23, height - 1)
    cursor_x = min(width - 1, 6 + len(command_text))
    screen.move(max(0, cursor_y), max(0, cursor_x))

    screen.refresh()
 

def run_dashboard(screen, node):
    curses.curs_set(1)
    screen.nodelay(True)

    command_text = ""
    message = "Ready"

    while rclpy.ok():
        rclpy.spin_once(node, timeout_sec=0.0)

        draw_dashboard(screen, node, command_text, message)

        key = screen.getch()

        if key == -1:
            time.sleep(0.03)
            continue

        if key in (10, 13, curses.KEY_ENTER):
            parts = command_text.strip().lower().split()
            command_text = ""

            if not parts:
                continue

            action = parts[0]

            if action == "quit":
                node.stop()
                node.publish_command()
                message = "Stopping and exiting"
                break

            elif action == "stop":
                node.stop()
                message = "All velocity commands set to zero"

            elif action == "help":
                message = "Use: axis value | stop | quit"

            elif len(parts) == 2:
                try:
                    value = float(parts[1])

                    if not (-1.0 <= value <= 1.0):
                        message = "Value must be between -1 and 1"
                    elif node.set_axis(action, value):
                        message = f"{action} set to {value:+.2f}"
                    else:
                        message = "Unknown axis"

                except ValueError:
                    message = "Invalid number"

            else:
                message = "Invalid command. Type help."

        elif key in (curses.KEY_BACKSPACE, 127, 8):
            command_text = command_text[:-1]

        elif 32 <= key <= 126:
            command_text += chr(key)

    time.sleep(0.15)


def main():
    rclpy.init()

    node = ROVDashboard()

    try:
        curses.wrapper(run_dashboard, node)

    except KeyboardInterrupt:
        pass

    finally:
        node.stop()
        node.publish_command()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
