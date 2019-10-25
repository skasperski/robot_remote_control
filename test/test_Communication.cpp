#include <boost/test/unit_test.hpp>

#include <interaction-library-controlled_robot/ControlledRobot.hpp>
#include <interaction-library-controlled_robot/RobotController.hpp>
#include <interaction-library-controlled_robot/Transports/TransportZmq.hpp>


using namespace controlledRobot;
 
TransportSharedPtr commands;
TransportSharedPtr telemetry;

TransportSharedPtr command;
TransportSharedPtr telemetri;


#define COMPARE_PROTOBUF(VAR1,VAR2) BOOST_TEST(VAR1.SerializeAsString() == VAR2.SerializeAsString())

/**
 * @brief used to init the communication only when it is used, it is reused then.
 * 
 */
void initComms(){
  
  if (!commands.get()){commands = TransportSharedPtr(new TransportZmq("tcp://127.0.0.1:7003",TransportZmq::REQ));}
  if (!telemetry.get()){telemetry = TransportSharedPtr(new TransportZmq("tcp://127.0.0.1:7004",TransportZmq::SUB));}

  if (!command.get()){command = TransportSharedPtr(new TransportZmq("tcp://*:7003",TransportZmq::REP));}
  if (!telemetri.get()){telemetri = TransportSharedPtr(new TransportZmq("tcp://*:7004",TransportZmq::PUB));}
}

controlledRobot::Pose initTestPose(){
  Position position;
  Orientation orientation;
  Pose pose;
  
	
  position.set_x(4);
  position.set_y(2);
  position.set_z(7);

  orientation.set_x(8);
  orientation.set_y(1);
  orientation.set_z(3);
  orientation.set_w(4);
    
  *(pose.mutable_position()) = position;
  *(pose.mutable_orientation()) = orientation;
  return pose;
}

controlledRobot::JointState initTestJointState(){
  JointState state;
  state.add_name("test");
  state.add_position(1);
  state.add_velocity(2);
  state.add_effort(3);
  return state;
}


BOOST_AUTO_TEST_CASE(checking_twist_command_transfer)
{
  initComms();

  RobotController controller(commands, telemetry);
  ControlledRobot robot(command, telemetri);

	robot.startUpdateThread(10);
  
  Twist sendtwistcommand;
  std::pair<unsigned long long int, Twist> receivetwistcommand;
  sendtwistcommand.mutable_angular()->set_z(0.4);
  sendtwistcommand.mutable_linear()->set_x(0.6);
  
  controller.setTwistCommand(sendtwistcommand);	
  
  receivetwistcommand = robot.getTwistCommand();
	
  robot.stopUpdateThread();
 
  COMPARE_PROTOBUF(sendtwistcommand,receivetwistcommand.second);

}

BOOST_AUTO_TEST_CASE(checking_target_pose)
{
  initComms();
  
  RobotController controller(commands, telemetry);
	ControlledRobot robot(command, telemetri);
	
  Pose pose = initTestPose();
  Pose pose2; 
	
  robot.startUpdateThread(10);

  controller.setTargetPose(pose);
    
  pose2 = robot.getTargetPose();

  robot.stopUpdateThread();
	

  COMPARE_PROTOBUF(pose,pose2);
	

}

BOOST_AUTO_TEST_CASE(checking_current_pose)
{

  initComms();
  RobotController controller(commands, telemetry);
  ControlledRobot robot(command, telemetri);
  controller.update();
  usleep(100 * 1000);

  
  Pose pose = initTestPose();
  Pose currentpose;

  //buffer for size comparsion
  std::string buf;
  pose.SerializeToString(&buf);
 
  //send telemetry data
  int sent = robot.setCurrentPose(pose);
    
  //wait a little for data transfer
  //the Telemetry send is non-blocking in opposite to commands
  usleep(100 * 1000);
  //receive pending data
  controller.update();

  controller.getCurrentPose(currentpose);
 
  
  //data was sent completely
  BOOST_CHECK((unsigned int)sent == buf.size());
  //and is the same
  COMPARE_PROTOBUF(pose,currentpose);

}


BOOST_AUTO_TEST_CASE(generic_request_telemetry_data)
{
  initComms();
  RobotController controller(commands, telemetry);
  ControlledRobot robot(command, telemetri);
  controller.startUpdateThread(10);
  robot.startUpdateThread(10);
  
  Pose pose = initTestPose();
  JointState jointstate = initTestJointState();

  
  //send telemetry data
  int sent;
  sent = robot.setCurrentPose(pose);
  sent = robot.setJointState(jointstate);



  //test single request
  Pose requestedpose;
  controller.requestTelemetry(CURRENT_POSE,requestedpose);
  COMPARE_PROTOBUF(pose,requestedpose);


  JointState requestedJointState;
  controller.requestTelemetry(JOINT_STATE,requestedJointState);
  COMPARE_PROTOBUF(jointstate,requestedJointState);


  robot.stopUpdateThread();
  controller.stopUpdateThread();
}

BOOST_AUTO_TEST_CASE(check_log_message)
{
  initComms();
  RobotController controller(commands, telemetry);
  ControlledRobot robot(command, telemetri);
  controller.startUpdateThread(10);
  robot.startUpdateThread(10);

  LogMessage requested_log_message;



  //test debug message, should go through
  controller.setLogLevel(DEBUG);

  LogMessage debug_message;
  debug_message.set_type(DEBUG);
  debug_message.set_message("[DEBUG] This is a debug message.");
  robot.setLogMessage(debug_message);

  //wait a little for data transfer (Telemetry is non-blocking)
  usleep(100 * 1000);
  controller.getLogMessage(requested_log_message);
  COMPARE_PROTOBUF(debug_message, requested_log_message);



  //test fatal message, should go through
  controller.setLogLevel(FATAL);

  LogMessage fatal_message;
  fatal_message.set_type(FATAL);
  fatal_message.set_message("[FATAL] This is a fatal message.");
  robot.setLogMessage(fatal_message);

  usleep(100 * 1000);
  controller.getLogMessage(requested_log_message);
  COMPARE_PROTOBUF(fatal_message, requested_log_message);



  //test error message, should not go through (because LogLevel is still at FATAL)
  LogMessage error_message;
  error_message.set_type(ERROR);
  error_message.set_message("[ERROR] This is an error message.");
  robot.setLogMessage(error_message);

  usleep(100 * 1000);
  controller.getLogMessage(requested_log_message);
  //compare if message is still the fatal message, not the error message
  COMPARE_PROTOBUF(fatal_message, requested_log_message);



  //test logLevel NONE, fatal message should not go through
  controller.setLogLevel(NONE);

  LogMessage another_fatal_message;
  another_fatal_message.set_type(FATAL);
  another_fatal_message.set_message("[FATAL] This is another fatal message.");
  robot.setLogMessage(another_fatal_message);

  usleep(100 * 1000);
  controller.getLogMessage(requested_log_message);
  //compare if message is still the first fatal message, not the second one
  COMPARE_PROTOBUF(fatal_message, requested_log_message);


  robot.stopUpdateThread();
  controller.stopUpdateThread();
}

BOOST_AUTO_TEST_CASE(checking_robot_state){
  initComms();
  RobotController controller(commands, telemetry);
  ControlledRobot robot(command, telemetri);
  controller.startUpdateThread(10);
  robot.startUpdateThread(10);
  
  std::string requested_robot_state;
  std::string gotten_robot_state;



  std::cout << "RUNNING" << std::endl;
  robot.setRobotState("ROBOT_DEMO_RUNNING");
  //wait a little for data transfer (Telemetry is non-blocking)
  usleep(100 * 1000);

  controller.getRobotState(gotten_robot_state);
  std::cout << gotten_robot_state << std::endl;

  controller.requestRobotState(requested_robot_state);
  std::cout << requested_robot_state << std::endl;

  BOOST_TEST(requested_robot_state == gotten_robot_state);


  controller.getRobotState(gotten_robot_state);
  std::cout << gotten_robot_state << std::endl;

  //should be empty because state was already recieved
  BOOST_TEST(gotten_robot_state == "");



  std::cout << "FINISHED" << std::endl;
  robot.setRobotState("ROBOT_DEMO_FINISHED");
  usleep(100 * 1000);

  controller.getRobotState(gotten_robot_state);
  std::cout << gotten_robot_state << std::endl;

  //requestRobotState always gets first setted robot state (wanted behaviour?)
  controller.requestRobotState(requested_robot_state);
  std::cout << requested_robot_state << std::endl;

  controller.requestRobotState(requested_robot_state);
  std::cout << requested_robot_state << std::endl;

  BOOST_TEST(gotten_robot_state == "ROBOT_DEMO_FINISHED");
  //wanted behaviour?
  BOOST_TEST(requested_robot_state == "ROBOT_DEMO_RUNNING");



  std::cout << "RUNNING_AGAIN THEN STOPPED" << std::endl;
  robot.setRobotState("ROBOT_DEMO_RUNNING_AGAIN");
  usleep(100 * 1000);
  robot.setRobotState("ROBOT_DEMO_STOPPED");
  usleep(100 * 1000);

  //getRobotState recieves RUNNING_AGAIN first time, then nothing (wanted behaviour?)
  controller.getRobotState(gotten_robot_state);
  std::cout << gotten_robot_state << std::endl;

  //wanted behaviour? 
  BOOST_TEST(gotten_robot_state == "ROBOT_DEMO_RUNNING_AGAIN");


  controller.getRobotState(gotten_robot_state);
  std::cout << gotten_robot_state << std::endl;

  //wanted behaviour? ROBOT_DEMO_STOPPED is lost
  BOOST_TEST(gotten_robot_state == "");


  //requestRobotState still outputs first robot state
  controller.requestRobotState(requested_robot_state);
  std::cout << requested_robot_state << std::endl;

  BOOST_TEST(requested_robot_state == "ROBOT_DEMO_RUNNING");


  robot.stopUpdateThread();
  controller.stopUpdateThread();
}