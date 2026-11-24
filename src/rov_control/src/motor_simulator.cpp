#include <algorithm>
#include <functional>
#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"

using namespace std::chrono_literals;

class MotorSimulator : public rclcpp::Node
{
public:
    MotorSimulator() : Node("motor_simulator")
    {
        this->declare_parameter<double>("max_rpm", 2000.0);
        this->declare_parameter<double>("response_rate", 500.0);
        this->declare_parameter<double>("thrust_coefficient", 0.00002);

        max_rpm_ = this->get_parameter("max_rpm").as_double();
        response_rate_ = this->get_parameter("response_rate").as_double();
        thrust_coefficient_ =
            this->get_parameter("thrust_coefficient").as_double();

        target_rpm_.resize(8, 0.0);
        actual_rpm_.resize(8, 0.0);

        rpm_sub_ = this->create_subscription<
            std_msgs::msg::Float32MultiArray>(
            "/motor_rpm_command", 10,
            std::bind(
                &MotorSimulator::command_callback,
                this,
                std::placeholders::_1));

        rpm_pub_ = this->create_publisher<
            std_msgs::msg::Float32MultiArray>(
            "/motor_rpm", 10);

        thrust_pub_ = this->create_publisher<
            std_msgs::msg::Float32MultiArray>(
            "/motor_thrust", 10);

        last_command_time_ = this->now();

        timer_ = this->create_wall_timer(
            50ms,
            std::bind(&MotorSimulator::update_motors, this));

        RCLCPP_INFO(this->get_logger(),
                    "Motor simulator started with 8 thrusters");
    }

private:
    void command_callback(
        const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        if (msg->data.size() != 8) {
            RCLCPP_WARN(
                this->get_logger(),
                "Expected 8 RPM commands, received %zu",
                msg->data.size());
            return;
        }

        for (size_t i = 0; i < 8; ++i) {
            double rpm = msg->data[i];

            if (!std::isfinite(rpm)) {
                RCLCPP_WARN(this->get_logger(),
                            "Invalid RPM command received");
                return;
            }

            target_rpm_[i] = std::clamp(
                rpm, -max_rpm_, max_rpm_);
        }

        last_command_time_ = this->now();
    }

    void update_motors()
    {
        const double dt = 0.05;

        // Stop if commands become stale.
        if ((this->now() - last_command_time_).seconds() > 0.5) {
            std::fill(target_rpm_.begin(),
                      target_rpm_.end(), 0.0);
        }

        std_msgs::msg::Float32MultiArray rpm_msg;
        std_msgs::msg::Float32MultiArray thrust_msg;

        for (size_t i = 0; i < 8; ++i) {
            double difference = target_rpm_[i] - actual_rpm_[i];

            double max_change = response_rate_ * dt;

            difference = std::clamp(
                difference, -max_change, max_change);

            actual_rpm_[i] += difference;

            // Simplified signed thrust model.
            double thrust = thrust_coefficient_ *
                            actual_rpm_[i] *
                            std::abs(actual_rpm_[i]);

            rpm_msg.data.push_back(
                static_cast<float>(actual_rpm_[i]));

            thrust_msg.data.push_back(
                static_cast<float>(thrust));
        }

        rpm_pub_->publish(rpm_msg);
        thrust_pub_->publish(thrust_msg);
    }

    double max_rpm_;
    double response_rate_;
    double thrust_coefficient_;

    std::vector<double> target_rpm_;
    std::vector<double> actual_rpm_;

    rclcpp::Time last_command_time_;

    rclcpp::Subscription<
        std_msgs::msg::Float32MultiArray>::SharedPtr rpm_sub_;

    rclcpp::Publisher<
        std_msgs::msg::Float32MultiArray>::SharedPtr rpm_pub_;

    rclcpp::Publisher<
        std_msgs::msg::Float32MultiArray>::SharedPtr thrust_pub_;

    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MotorSimulator>());
    rclcpp::shutdown();
    return 0;
}
