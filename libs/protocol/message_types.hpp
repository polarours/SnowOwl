#pragma once

#include <cstdint>

namespace SnowOwl::Protocol {

enum class MessageType : std::uint8_t {
    Frame = 0x01,
    Event = 0x02,
    Heartbeat = 0x03,
    Control = 0x10
};

}