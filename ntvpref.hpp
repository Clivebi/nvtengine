#pragma once
#include "engine/value.hpp"
class NVTPref {
protected:
    Interpreter::Value mPref;

public:
    explicit NVTPref(Interpreter::Value pref) : mPref(pref) {
        if (!mPref.IsMap()) {
            throw Interpreter::RuntimeException("invalid config value");
        }
    }
    int number_of_concurrent_tasks() {
        auto iter = mPref._map().find("number_of_concurrent_tasks");
        if (iter != mPref._map().end()) {
            return (int)iter->second.ToInteger();
        }
        return 30;
    }
    int port_scan_send_packet_rate() {
        auto iter = mPref._map().find("port_scan_send_packet_rate");
        if (iter != mPref._map().end()) {
            return (int)iter->second.ToInteger();
        }
        return 1000;
    }

    int script_default_timeout_second() {
        auto iter = mPref._map().find("script_default_timeout_second");
        if (iter != mPref._map().end()) {
            return (int)iter->second.ToInteger();
        }
        return 120;
    }
    int service_detection_thread_count() {
        auto iter = mPref._map().find("service_detection_thread_count");
        if (iter != mPref._map().end()) {
            return (int)iter->second.ToInteger();
        }
        return 6;
    }
    std::string default_network_interface() {
        auto iter = mPref._map().find("default_network_interface");
        if (iter != mPref._map().end()) {
            return iter->second.ToString();
        }
        return "";
    }
    int log_level() {
        auto iter = mPref._map().find("log_level");
        if (iter != mPref._map().end()) {
            return (int)iter->second.ToInteger();
        }
        return 2;
    }
    int log_engine_warning() {
        auto iter = mPref._map().find("log_engine_warning");
        if (iter != mPref._map().end()) {
            return (int)iter->second.ToInteger();
        }
        return true;
    }
    int tcp_connect_timeout_second() {
        auto iter = mPref._map().find("tcp_connect_timeout_second");
        if (iter != mPref._map().end()) {
            return (int)iter->second.ToInteger();
        }
        return 5;
    }
    int trace_object_storage() {
        auto iter = mPref._map().find("trace_object_storage");
        if (iter != mPref._map().end()) {
            return (int)iter->second.ToInteger();
        }
        return false;
    }
    int tcp_read_write_timeout_second() {
        auto iter = mPref._map().find("tcp_read_write_timeout_second");
        if (iter != mPref._map().end()) {
            return (int)iter->second.ToInteger();
        }
        return 5;
    }
    std::string script_package() {
        auto iter = mPref._map().find("script_package");
        if (iter != mPref._map().end()) {
            return iter->second.ToString();
        }
        return "";
    }

    std::string script_folder() {
        auto iter = mPref._map().find("script_folder");
        if (iter != mPref._map().end()) {
            return iter->second.ToString();
        }
        return "";
    }

    std::string app_data_folder() {
        auto iter = mPref._map().find("app_data_folder");
        if (iter != mPref._map().end()) {
            return iter->second.ToString();
        }
        return "";
    }

    std::string builtin_script_path() {
        auto iter = mPref._map().find("builtin_script_path");
        if (iter != mPref._map().end()) {
            return iter->second.ToString();
        }
        return "";
    }
};