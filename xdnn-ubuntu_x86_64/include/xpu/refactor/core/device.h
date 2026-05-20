#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_DEVICE_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_DEVICE_H
#include <string>

namespace baidu {
namespace xpu {
namespace api {
enum class DeviceType {
    CPU = 0,
    XPU1 = 1,
    XPU2 = 2,
    XPU3 = 3,
    XPU4 = 4,
};
constexpr DeviceType kCPU = DeviceType::CPU;
constexpr DeviceType kXPU1 = DeviceType::XPU1;
constexpr DeviceType kXPU2 = DeviceType::XPU2;
constexpr DeviceType kXPU3 = DeviceType::XPU3;
constexpr DeviceType kXPU4 = DeviceType::XPU4;

struct Device {
    Device(DeviceType type = kCPU, int id = 0) : type_(type), id_(id) {
        if (type == kCPU) {
            id = 0;
            type_compared_to_cpu = kXPU2;
        }
    }
    bool operator==(const Device& other) const noexcept {
        return this->type_ == other.type_ && this->id_ == other.id_;
    }
    bool operator!=(const Device& other) const noexcept {
        return !(*this == other);
    }
    DeviceType type() const noexcept {
        return type_;
    }
    int id() const noexcept {
        return id_;
    }
    // Set the member function type_compared_to_cpu.
    int set_type_compared_to_cpu(DeviceType type) {
        if (this->type_ == kCPU) {
            this->type_compared_to_cpu = type;
            return 0;
        }
        return -1;
    }

    // Get the value of type_compared_to_cpu.
    DeviceType type_compared_to_cpu_value() const noexcept {
        return type_compared_to_cpu;
    }
private:
    DeviceType type_;
    // On different XPU hardware, the rounding methods, overflow handling of maximum and minimum 
    // values, etc., are all different, leading to results that cannot be aligned with those of the CPU.
    // This variable is used to implement differentiated CPU methods for different XPU hardware.
    DeviceType type_compared_to_cpu;
    int id_;
};

inline std::string to_string(DeviceType dt) {
    std::string arr[5] = {
        std::string("kCPU"),
        std::string("kXPU1"),
        std::string("kXPU2"),
        std::string("kXPU3"),
        std::string("kXPU4"),
    };
    return arr[static_cast<int>(dt)];
}

inline std::string to_string(Device dev) {
    return std::string("{") + api::to_string(dev.type()) + "," + std::to_string(dev.id()) + "}";
}

}
}
}
#endif
