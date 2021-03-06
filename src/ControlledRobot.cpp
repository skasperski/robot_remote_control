#include "ControlledRobot.hpp"
#include <iostream>


namespace robot_remote_control {


ControlledRobot::ControlledRobot(TransportSharedPtr commandTransport, TransportSharedPtr telemetryTransport):UpdateThread(),
    commandTransport(commandTransport),
    telemetryTransport(telemetryTransport),
    logLevel(CUSTOM-1) {
    registerCommandBuffer(TARGET_POSE_COMMAND, &poseCommand);
    registerCommandBuffer(TWIST_COMMAND, &twistCommand);
    registerCommandBuffer(GOTO_COMMAND, &goToCommand);
    registerCommandBuffer(SIMPLE_ACTIONS_COMMAND, &simpleActionsCommand);
    registerCommandBuffer(COMPLEX_ACTIONS_COMMAND, &complexActionsCommand);
    registerCommandBuffer(JOINTS_COMMAND, &jointsCommand);
}

void ControlledRobot::update() {
    while (receiveRequest() != NO_CONTROL_DATA) {}
}

ControlMessageType ControlledRobot::receiveRequest() {
    std::string msg;
    Transport::Flags flags = Transport::NONE;
    // if (!this->threaded()){
        flags = Transport::NOBLOCK;
    // }
    int result = commandTransport->receive(&msg, flags);
    if (result) {
        ControlMessageType reqestType = evaluateRequest(msg);
        return reqestType;
    }
    return NO_CONTROL_DATA;
}

ControlMessageType ControlledRobot::evaluateRequest(const std::string& request) {
    uint16_t* type = reinterpret_cast<uint16_t*>(const_cast<char*>(request.data()));
    ControlMessageType msgtype = (ControlMessageType)*type;
    std::string serializedMessage(request.data()+sizeof(uint16_t), request.size()-sizeof(uint16_t));

    switch (msgtype) {
        case TELEMETRY_REQUEST: {
            uint16_t* requestedtype = reinterpret_cast<uint16_t*>(const_cast<char*>(serializedMessage.data()));
            TelemetryMessageType type = (TelemetryMessageType) *requestedtype;
            std::string reply = buffers.peekSerialized(type);
            commandTransport->send(reply);
            return TELEMETRY_REQUEST;
        }
        case LOG_LEVEL_SELECT: {
            logLevel = *reinterpret_cast<uint32_t*>(const_cast<char*>(serializedMessage.data()));
            commandTransport->send(serializeControlMessageType(LOG_LEVEL_SELECT));
            return LOG_LEVEL_SELECT;
        }
        default: {
            CommandBufferBase * cmdbuffer = commandbuffers[msgtype];
            if (cmdbuffer) {
                if (!cmdbuffer->write(serializedMessage)) {
                    printf("unable to parse message of type %i in %s:%i\n", msgtype, __FILE__, __LINE__);
                    commandTransport->send(serializeControlMessageType(NO_CONTROL_DATA));
                    return NO_CONTROL_DATA;
                }
                commandTransport->send(serializeControlMessageType(msgtype));
                return msgtype;

            } else {
                commandTransport->send(serializeControlMessageType(NO_CONTROL_DATA));
                return msgtype;
            }
        }
    }
}


int ControlledRobot::setRobotState(const std::string& state) {
    RobotState protostate;
    protostate.set_state(state);
    return sendTelemetry(protostate, ROBOT_STATE);
}

int ControlledRobot::setLogMessage(enum LogLevel lvl, const std::string& message) {
    if (lvl <= logLevel || lvl >= CUSTOM) {
        LogMessage msg;
        msg.set_level(lvl);
        msg.set_message(message);
        return sendTelemetry(msg, LOG_MESSAGE);
    }
    return -1;
}

int ControlledRobot::setLogMessage(const LogMessage& log_message) {
    if (log_message.level() <= logLevel || log_message.level() >= CUSTOM) {
        return sendTelemetry(log_message, LOG_MESSAGE);
    }
    return -1;
}

void ControlledRobot::addTelemetryMessageType(std::string *buf, const TelemetryMessageType& type) {
    int currsize = buf->size();
    buf->resize(currsize + sizeof(uint16_t));
    uint16_t typeint = type;
    uint16_t* data = reinterpret_cast<uint16_t*>(const_cast<char*>(buf->data()+currsize));
    *data = typeint;
}

void ControlledRobot::addControlMessageType(std::string *buf, const ControlMessageType& type) {
    int currsize = buf->size();
    buf->resize(currsize + sizeof(uint16_t));
    uint16_t typeint = type;
    uint16_t* data = reinterpret_cast<uint16_t*>(const_cast<char*>(buf->data()+currsize));
    *data = typeint;
}

std::string ControlledRobot::serializeControlMessageType(const ControlMessageType& type) {
    std::string buf;
    addControlMessageType(&buf, type);
    return buf;
}


}  // namespace robot_remote_control
