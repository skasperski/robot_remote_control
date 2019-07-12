#pragma once

#include "MessageTypes.hpp"
#include "Transports/Transport.hpp"
#include "UpdateThread/UpdateThread.hpp"


namespace interaction
{
    class ControlledRobot: public UpdateThread
    {
        public: 

            ControlledRobot(TransportSharedPtr commandTransport,TransportSharedPtr telemetryTransport);
            virtual ~ControlledRobot(){};

            virtual void update();

            interaction::Pose getTargetPose(){
                return targetPose.get();
            }

            int setCurrentPose(const interaction::Pose& pose);

            int setJointState(const interaction::JointState& state);


            interaction::Twist getTwistCommand(){
                return twistCommand.get();
            }

        protected:
            virtual ControlMessageType receiveRequest();

            ControlMessageType evaluateRequest(const std::string& request);

        private:
            void addControlMessageType(std::string &buf, const ControlMessageType& type);
            void addTelemetryMessageType(std::string &buf, const TelemetryMessageType& type);

            TransportSharedPtr commandTransport;
            TransportSharedPtr telemetryTransport;

            std::string serializeControlMessageType(const ControlMessageType& type);
            std::string serializeCurrentPose();

            //buffers
            ThreadProtecetedVar<interaction::Pose> targetPose;
            ThreadProtecetedVar<interaction::Pose> currentPose;
            ThreadProtecetedVar<interaction::Twist> twistCommand;

            /**
             * @brief 
             * 
             * @tparam CLASS 
             * @param protodata 
             * @param type 
             * @return int size sent
             */
            template<class CLASS> int sendTelemetry(const CLASS &protodata, const TelemetryMessageType& type){
                if (telemetryTransport.get()){
                    std::string buf;
                    buf.resize(sizeof(uint16_t));
                    uint16_t uint_type = type;
                    uint16_t* data = (uint16_t*)buf.data();
                    *data = uint_type;
                    protodata.AppendToString(&buf);
                    return telemetryTransport->send(buf) - sizeof(uint16_t);
                }
                printf("ERROR Transport invalid\n");
                return 0;
            };
            


    };

} // end namespace interaction-library-controlled_robot

