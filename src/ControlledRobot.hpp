#pragma once

#include "MessageTypes.hpp"
#include "Transports/Transport.hpp"
#include "UpdateThread/UpdateThread.hpp"
#include "TelemetryBuffer.hpp"
#include <map>
#include <string>


namespace robot_remote_control {

class ControlledRobot: public UpdateThread{
    public:
        ControlledRobot(TransportSharedPtr commandTransport, TransportSharedPtr telemetryTransport);
        virtual ~ControlledRobot() {}

        /**
         * @brief threaded update function called by UpdateThread that receives commands
         */
        virtual void update();


        // Command getters

        /**
         * @brief Get the Target Pose the robot should move to
         * 
         * @return true if the command was not read before
         * @param command the last received command
         */
        bool getTargetPoseCommand(Pose *command) {
            return poseCommand.read(command);
        }

        /**
         * @brief Get the Twist Command with velocities to robe should move at
         * 
         * @return true if the command was not read before
         * @param command the last received command
         */
        bool getTwistCommand(Twist *command) {
            return twistCommand.read(command);
        }

        /**
         * @brief Get the GoTo Command the robot should execute
         *
         * @return true if the command was not read before
         * @param command the last received command
         */
        bool getGoToCommand(GoTo *command) {
            return goToCommand.read(command);
        }

        /**
         * @brief Get the Joints Command the robot should execute
         *
         * @return true if the command was not read before
         * @param command the last received command
         */
        bool getJointsCommand(JointState *command) {
            return jointsCommand.read(command);
        }


        /**
         * @brief Get the SimpleActions Command the robot should execute
         *
         * @return true if the command was not read before
         * @param command the last received command
         */
        bool getSimpleActionCommand(SimpleAction *command) {
            return simpleActionsCommand.read(command);
        }

        /**
         * @brief Get the ComplexActions Command the robot should execute
         *
         * @return true if the command was not read before
         * @param command the last received command
         */
        bool getComplexActionCommand(ComplexAction *command) {
            return complexActionsCommand.read(command);
        }

        // Telemetry setters

    private:
        /**
         * @brief generic send of telemetry types
         * 
         * @tparam CLASS 
         * @param protodata 
         * @param type 
         * @return int size sent
         */
        template<class CLASS> int sendTelemetry(const CLASS &protodata, const TelemetryMessageType& type) {
            if (telemetryTransport.get()) {
                std::string buf;
                buf.resize(sizeof(uint16_t));
                uint16_t uint_type = type;
                uint16_t* data = reinterpret_cast<uint16_t*>(const_cast<char*>(buf.data()));
                *data = uint_type;
                protodata.AppendToString(&buf);
                // store latest data for future requests
                buffers.lock();
                RingBufferAccess::pushData(buffers.get_ref()[type], protodata, true);
                buffers.unlock();
                return telemetryTransport->send(buf) - sizeof(uint16_t);
            }
            printf("ERROR Transport invalid\n");
            return 0;
        }

    public:
        /**
         * @brief The robot uses this method to provide information about its controllable joints
         *
         * @param controllableJoints the controllable joints of the robot as a JointState
         * @return int number of bytes sent
         */
        int initControllableJoints(const JointState& telemetry) {
            return sendTelemetry(telemetry, CONTROLLABLE_JOINTS);
        }

        /**
         * @brief The robot uses this method to provide information about its set of simple actions
         *
         * @param simpleActions the simple actions of the robot to report to the controler
         * the state field of the SimpleActions class should be filled with the max value
         * @return int number of bytes sent
         */
        int initSimpleActions(const SimpleActions& telemetry) {
            return sendTelemetry(telemetry, SIMPLE_ACTIONS);
        }

        /**
         * @brief The robot uses this method to provide information about its set of complex actions
         *
         * @param complexActions the complex actions of the robot as a ComplexActions
         * @return int number of bytes sent
         */
        int initComplexActions(const ComplexActions& telemetry) {
            return sendTelemetry(telemetry, COMPLEX_ACTIONS);
        }

        /**
         * @brief The robot uses this method to provide information about its sensors
         * The name is only mandatory here, setSimpleSnsor() may omit this value and identify by id
         * 
         * @param telemetry a list of simple sensors and their names/ids, other firelds not nessecary
         * @return int  number of bytes sent
         */
        int initSimpleSensors(const SimpleSensors &telemetry) {
            return sendTelemetry(telemetry, SIMPLE_SENSOR_DEFINITION);
        }

        /**
         * @brief The robot uses this method to provide information about its name
         *
         * @param robotName the name of the robot as a RobotName
         * @return int number of bytes sent
         */
        int initRobotName(const RobotName& telemetry) {
            return sendTelemetry(telemetry, ROBOT_NAME);
        }

        /**
         * @brief submit the video strem urls
         * 
         * @param telemetry list of streams and camera poses
         * @return int number of bytes sent
         */
        int initVideoStreams(const VideoStreams& telemetry) {
            return sendTelemetry(telemetry, VIDEO_STREAMS);
        }

        /**
         * @brief Send a log message, is only send, if the log level set by the controller is higher
         * or equal to the lvl or higher than CUSTOM in these parameters. CUSTOM Messages can be 20 or higher
         * 
         * @param lvl LogLevel (NONE=0,FATAL,ERROR,WARN,INFO,DEBUG,CUSTOM=20)
         * @param message the message to send
         * @return int number of bytes sent
         */
        int setLogMessage(enum LogLevel lvl, const std::string& message);

        /**
         * @brief Set the Log Message object, 
         * 
         * @param log_message 
         * @return int number of bytes sent
         */
        int setLogMessage(const LogMessage& log_message);

        /**
         * @brief Set the Robot State string
         * 
         * @param state the state description
         * @return int number of bytes sent
         */
        int setRobotState(const std::string& state);


        /**
         * @brief Set the current Pose of the robot
         * 
         * @param telemetry current pose
         * @return int number of bytes sent
         */
        int setCurrentPose(const Pose& telemetry) {
            return sendTelemetry(telemetry, CURRENT_POSE);
        }

        /**
         * @brief Set the curretn JointState of the robot
         * 
         * @param telemetry current JointState
         * @return int number of bytes sent
         */
        int setJointState(const JointState& telemetry) {
                return sendTelemetry(telemetry, JOINT_STATE);
        }

        /**
         * @brief Set a single Simple Sensor value
         * the name strin can be omitted, if it was provided using initSimpleSensors()
         * 
         * @param telemetry a single sensor value
         * @return int number of bytes sent
         */
        int setSimpleSensor(const SimpleSensor &telemetry ) {
            return sendTelemetry(telemetry, SIMPLE_SENSOR_VALUE);
        }


    protected:
        virtual ControlMessageType receiveRequest();

        virtual ControlMessageType evaluateRequest(const std::string& request);

    private:
        struct CommandBufferBase{
            CommandBufferBase() {}
            virtual ~CommandBufferBase() {}
            virtual bool write(const std::string &serializedMessage) = 0;
            virtual bool read(std::string *receivedMessage) = 0;
        };

        template<class COMMAND> struct CommandBuffer: public CommandBufferBase{
            public:
                CommandBuffer():isnew(false) {}

                virtual ~CommandBuffer() {}

                bool read(COMMAND *target) {
                    bool oldval = isnew.get();
                    *target = command.get();
                    isnew.set(false);
                    return oldval;
                }

                void write(const COMMAND &src) {
                    command.set(src);
                    isnew.set(true);
                }

                virtual bool write(const std::string &serializedMessage) {
                    command.lock();
                    if (!command.get_ref().ParseFromString(serializedMessage)) {
                        command.unlock();
                        isnew.set(false);
                        return false;
                    }
                    command.unlock();
                    isnew.set(true);
                    return true;
                }

                virtual bool read(std::string *receivedMessage) {
                    bool oldval = isnew.get();
                    command.lock();
                    command.get_ref().SerializeToString(receivedMessage);
                    command.unlock();
                    isnew.set(false);
                    return oldval;
                }

            private:
                ThreadProtectedVar<COMMAND> command;
                ThreadProtectedVar<bool> isnew;
        };

        // command buffers
        CommandBuffer<Pose> poseCommand;
        CommandBuffer<Twist> twistCommand;
        CommandBuffer<GoTo> goToCommand;
        CommandBuffer<SimpleAction> simpleActionsCommand;
        CommandBuffer<ComplexAction> complexActionsCommand;
        CommandBuffer<JointState> jointsCommand;

        std::map<uint32_t, CommandBufferBase*> commandbuffers;
        void registerCommandBuffer(const uint32_t & ID, CommandBufferBase *bufptr) {
            commandbuffers[ID] = bufptr;
        }



        void addControlMessageType(std::string *buf, const ControlMessageType& type);
        void addTelemetryMessageType(std::string *buf, const TelemetryMessageType& type);

        TransportSharedPtr commandTransport;
        TransportSharedPtr telemetryTransport;

        std::string serializeControlMessageType(const ControlMessageType& type);
        // std::string serializeCurrentPose();


        // buffer of sent telemetry (used for telemetry requests)
        TelemetryBuffer buffers;

        uint32_t logLevel;
};

}  // namespace robot_remote_control

