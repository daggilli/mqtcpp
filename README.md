# mqtcpp

## C++ classes for MQTT subscribers and publishers

### Scope

Thsi project provides a pair of classes (`MqttCpp::Subscriber` and `MqttCpp::Publisher`) that implement the MQTT message passing protocol. They can be used as the building block for _e.g._ intercommunication between Internt of Things devices.

### Requirements

You will need a server running MQTT version 5.

The classes are built on the Paho Eclipse C and C++ libraries. It is recommended to build these from source.

Parsing of configuration files uses `libjsoncpp`. Your distro may include a sufficiently recent version; if not, building from source is recommended with the proviso that if you are using a modern C++ standard, you will need to modify the top-level `CMakeLists.txt` file to ensure that `CMAKE_CXX_STANDARD` is at least 17. This is because with a modern C++ compiler, the `jsoncpp` code will try to use string views for JSON element access, but the cmake configuration uses C++11, resulting in linker errors because string view post-dates C++11.

### Usage

The code should build with `CMAKE`, `DBUILD` for the Debug version and `RBUILD` for the Release version. Executables are built in the appropriate directories. The source code is illustrative of use. Operation should be straightforward in a multithreaded environment as access to the publisher's queue is via a thread-safe structure and the only state variables shared across instances are atomic.
