#pragma once

#define ARDUINOJSON_ENABLE_STRING_VIEW 1
#include <ArduinoJson.h>
#include <esp_heap_caps.h>

namespace ArduinoJson
{
template <typename T, typename _Alloc> struct Converter<std::vector<T, _Alloc>>
{
    static void toJson(const std::vector<T, _Alloc> &src, JsonVariant dst)
    {
        JsonArray array = dst.to<JsonArray>();
        for (T item : src)
            array.add(item);
    }
};

template <typename T> struct Converter<std::optional<T>>
{
    static void toJson(const std::optional<T> &src, JsonVariant dst)
    {
        if (src.has_value())
        {
            dst.set(src.value());
        }
        else
        {
            dst.set(nullptr);
        }
    }
};

struct SpiRamAllocator : ArduinoJson::Allocator
{
    void *allocate(size_t size)
    {
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    }

    void deallocate(void *pointer)
    {
        heap_caps_free(pointer);
    }

    void *reallocate(void *ptr, size_t new_size)
    {
        return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
    }

    static SpiRamAllocator & instance()
    {
        static SpiRamAllocator allocator;
        return allocator;
    }
};

} // namespace ArduinoJson
