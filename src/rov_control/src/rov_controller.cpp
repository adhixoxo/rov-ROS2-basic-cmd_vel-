
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"

class ROVController : public rclcpp::Node
{
public:
    ROVController() : Node("rov_controller")
    {
        declare_parameter<double>("max_rpm", 2000.0);

        max_rpm_ = get_parameter("max_rpm").as_double();

        cmd_sub_ = create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel",
            10,
            std::bind(
                &ROVController::cmd_callback,
                this,
                std::placeholders::_1
            )
        );

        rpm_pub_ =
            create_publisher<std_msgs::msg::Float32MultiArray>(
                "/motor_rpm_command", 10
            );

        RCLCPP_INFO(
            get_logger(),
            "ROV Controller started. 8 thrusters ready."
        );
    }

private:

    void cmd_callback(
        const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        // Normalize velocity commands to the range -1 to +1.
        // Linear axes use m/s; angular axes use rad/s.

        double surge = std::clamp(msg->linear.x / 1.0, -1.0, 1.0);
        double sway  = std::clamp(msg->linear.y / 1.0, -1.0, 1.0);
        double heave = std::clamp(msg->linear.z / 1.0, -1.0, 1.0);

        double roll  = std::clamp(msg->angular.x / 1.0, -1.0, 1.0);
        double pitch = std::clamp(msg->angular.y / 1.0, -1.0, 1.0);
        double yaw   = std::clamp(msg->angular.z / 1.0, -1.0, 1.0);

        // Horizontal thruster mixing
        std::array<double, 8> motor = {
            surge - sway + yaw,
            surge + sway - yaw,
            surge + sway + yaw,
            surge - sway - yaw,

            // Vertical thruster mixing
            heave + pitch + roll,
            heave + pitch - roll,
            heave - pitch + roll,
            heave - pitch - roll
        };

        // Normalize to prevent exceeding maximum RPM.
        double largest = 1.0;

        for (double value : motor) {
            largest = std::max(largest, std::abs(value));
        }

        std_msgs::msg::Float32MultiArray output;

        for (double value : motor) {
            double rpm = (value / largest) * max_rpm_;
            output.data.push_back(static_cast<float>(rpm));
        }

        rpm_pub_->publish(output);

        RCLCPP_INFO(
            get_logger(),
            "Motor RPM: [%.0f, %.0f, %.0f, %.0f, %.0f, %.0f, %.0f, %.0f]",
            motor[0] / largest * max_rpm_,
            motor[1] / largest * max_rpm_,
            motor[2] / largest * max_rpm_,
            motor[3] / largest * max_rpm_,
            motor[4] / largest * max_rpm_,
            motor[5] / largest * max_rpm_,
            motor[6] / largest * max_rpm_,
            motor[7] / largest * max_rpm_
        );
    }

    double max_rpm_;

    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;

    rclcpp::Publisher<
        std_msgs::msg::Float32MultiArray
    >::SharedPtr rpm_pub_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    rclcpp::spin(std::make_shared<ROVController>());

    rclcpp::shutdown();

    return 0;
}

