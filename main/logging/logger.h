#pragma once

#include "util/semaphore_lockable.h"
#include "util/singleton.h"
#include "web_callback_sink.h"
#include <esp_log.h>
#include <vector>

class esp32_hook;
class sd_card;

class logger : public esp32::singleton<logger>
{
  public:
    bool enable_web_logging(const std::function<void(std::unique_ptr<std::string>)> &callbackP);
    void disable_web_logging();

    auto get_general_logging_level()
    {
        return esp_log_level_get("*");
    }

    void set_logging_level(const char *tag, esp_log_level_t level);

    void set_general_logging_level(esp_log_level_t level)
    {
        set_logging_level("*", level);
    }

  private:
    logger();
    friend class esp32::singleton<logger>;

    static void logging_shutdown_handler();

    esp32::semaphore hook_mutex_;
    std::unique_ptr<web_callback_sink> web_callback_sink_instance_;
    std::unique_ptr<esp32_hook> hook_instance_{nullptr};
    std::vector<std::string> logging_tags;

    void hook_logger();

    template <class T> void remove_sink(std::unique_ptr<T> &p);
    friend class esp32_hook;
};