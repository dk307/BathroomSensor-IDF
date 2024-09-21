#include "config_manager.h"
#include "app_events.h"
#include "logging/logging_tags.h"
#include "util/default_event.h"
#include "util/filesystem/file_info.h"
#include "util/filesystem/filesystem.h"
#include "util/hash/hash.h"
#include "util/helper.h"
#include "util/psram_allocator.h"
#include <esp_log.h>
#include <filesystem>

constexpr std::string_view host_name_key{"host_name"};
constexpr std::string_view use_fahrenheit_key{"use_fahrenheit"};
constexpr std::string_view web_login_username_key{"web_username"};
constexpr std::string_view web_login_password_key{"web_password"};
constexpr std::string_view ssid_key{"ssid"};
constexpr std::string_view ssid_password_key{"ssid_password"};
constexpr std::string_view default_host_name{"Sensor"};
constexpr std::string_view default_user_id_and_password{"admin"};

static const char HostNameId[] = "hostname";
static const char WebUserNameId[] = "webusername";
static const char WebPasswordId[] = "webpassword";
static const char SsidId[] = "ssid";
static const char SsidPasswordId[] = "ssidpassword";

void config::begin()
{
    ESP_LOGD(CONFIG_TAG, "Loading Configuration");

    nvs_storage.begin("nvs", "config");

    ESP_LOGI(CONFIG_TAG, "Hostname:%s", get_host_name().c_str());
    ESP_LOGI(CONFIG_TAG, "Web user name:%s", get_web_user_credentials().get_user_name().c_str());
    ESP_LOGI(CONFIG_TAG, "Web user password:%s", get_web_user_credentials().get_password().c_str());
    ESP_LOGI(CONFIG_TAG, "Wifi ssid:%s", get_wifi_credentials().get_user_name().c_str());
    ESP_LOGI(CONFIG_TAG, "Wifi ssid password:%s", get_wifi_credentials().get_password().c_str());
}

void config::save()
{
    ESP_LOGI(CONFIG_TAG, "config save");
    nvs_storage.commit();
    CHECK_THROW_ESP(esp32::event_post(APP_COMMON_EVENT, CONFIG_CHANGE));
}

std::string config::get_all_config_as_json()
{
    BasicJsonDocument<esp32::psram::json_allocator> json_document(2048);

    json_document[(HostNameId)] = get_host_name();
    const auto web_cred = get_web_user_credentials();
    json_document[(WebUserNameId)] = web_cred.get_user_name();
    json_document[(WebPasswordId)] = web_cred.get_password();

    const auto wifi_cred = get_wifi_credentials();
    json_document[(SsidId)] = wifi_cred.get_user_name();
    json_document[(SsidPasswordId)] = wifi_cred.get_password();

    std::string json;
    serializeJson(json_document, json);

    return json;
}

std::string config::get_host_name()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    return nvs_storage.get(host_name_key, default_host_name);
}

void config::set_host_name(const std::string &host_name)
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    nvs_storage.save(host_name_key, host_name);
}

credentials config::get_web_user_credentials()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    return credentials(nvs_storage.get(web_login_username_key, default_user_id_and_password),
                       nvs_storage.get(web_login_password_key, default_user_id_and_password));
}

void config::set_web_user_credentials(const credentials &web_user_credentials)
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    nvs_storage.save(web_login_username_key, web_user_credentials.get_user_name());
    nvs_storage.save(web_login_password_key, web_user_credentials.get_password());
}

void config::set_wifi_credentials(const credentials &wifi_credentials)
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    nvs_storage.save(ssid_key, wifi_credentials.get_user_name());
    nvs_storage.save(ssid_password_key, wifi_credentials.get_password());
}

credentials config::get_wifi_credentials()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    return credentials(nvs_storage.get(ssid_key, std::string_view()), nvs_storage.get(ssid_password_key, std::string_view()));
}
