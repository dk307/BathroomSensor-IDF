#pragma once

#include "credentials.h"
#include "preferences.h"
#include "util/arduino_json_helper.h"
#include "util/noncopyable.h"
#include "util/semaphore_lockable.h"
#include "util/singleton.h"
#include <atomic>
#include <mutex>
#include <optional>
#include <vector>

class config : public esp32::singleton<config>
{
  public:
    void begin();
    void save();

    std::string get_all_config_as_json();

    std::string get_host_name();
    void set_host_name(const std::string &host_name);

    credentials get_web_user_credentials();
    void set_web_user_credentials(const credentials &web_user_credentials);

    void set_wifi_credentials(const credentials &wifi_credentials);
    credentials get_wifi_credentials();

  private:
    config() = default;

    friend class esp32::singleton<config>;

    mutable esp32::semaphore data_mutex_;
    preferences nvs_storage;
};
