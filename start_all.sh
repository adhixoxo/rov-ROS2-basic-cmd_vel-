
#!/usr/bin/env bash
set -e

SESSION="rov"
ROS_SETUP="source /opt/ros/jazzy/setup.bash && source ~/rov_ws/install/setup.bash"

# Reattach if already running
if tmux has-session -t "$SESSION" 2>/dev/null; then
    echo "ROV session already exists. Attaching..."
    exec tmux attach-session -t "$SESSION"
fi

# 0 - Controller + thrust feedback
tmux new-session -d -s "$SESSION" -n SUBSCRIBER \
"bash -lc '$ROS_SETUP
ros2 run rov_control rov_controller > /tmp/rov_controller.log 2>&1 &
NODE_PID=\$!
trap \"kill \$NODE_PID 2>/dev/null || true\" EXIT
ros2 topic echo /motor_thrust std_msgs/msg/Float32MultiArray'"

# 1 - Motor simulator + actual RPM feedback
tmux new-window -d -t "$SESSION" -n PUBLISHER \
"bash -lc '$ROS_SETUP
ros2 run rov_control motor_simulator > /tmp/rov_motor_simulator.log 2>&1 &
NODE_PID=\$!
trap \"kill \$NODE_PID 2>/dev/null || true\" EXIT
ros2 topic echo /motor_rpm std_msgs/msg/Float32MultiArray'"

# 2 - Target RPM message transfer
tmux new-window -d -t "$SESSION" -n MSG_TRANSFER \
"bash -lc '$ROS_SETUP
ros2 topic echo /motor_rpm_command std_msgs/msg/Float32MultiArray'"

# 3 - Interactive dashboard
tmux new-window -d -t "$SESSION" -n DASHBOARD \
"bash -lc '$ROS_SETUP
ros2 run rov_control dashboard.py'"

# 4 - ROS graph
tmux new-window -d -t "$SESSION" -n NODE_GRAPH \
"bash -lc '$ROS_SETUP
ros2 run rqt_graph rqt_graph'"

# Start on dashboard
tmux select-window -t "$SESSION:3"

# Attach
tmux attach-session -t "$SESSION"

