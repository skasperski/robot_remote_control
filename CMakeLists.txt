# CMakeLists.txt has to be located in the project folder and cmake has to be
# executed from 'project/build' with 'cmake ../'.
cmake_minimum_required(VERSION 3.1.0)

find_package(Rock)
rock_init(robot_remote_control 0.1)
rock_standard_layout()
