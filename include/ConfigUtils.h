#pragma once

#include <string>
#include <type_traits>

namespace ConfigUtils {
    /**
     * @brief Ensure the config is loaded
     */
    void EnsureConfigLoaded();
    // Bool support
    bool GetBool(const std::string& key, bool defaultValue = false);
    void SetBool(const std::string& key, bool value);

    // Int support
    int GetInt(const std::string& key, int defaultValue = 0);
    void SetInt(const std::string& key, int value);

    // String support
    std::string GetString(const std::string& key, const std::string& defaultValue = "");
    void SetString(const std::string& key, const std::string& value);

    template<typename T>
    class Config {
    public:
        Config(const std::string& key, const T& defaultValue = T{}) 
            : m_key(key), m_defaultValue(defaultValue) {}

        T Get() const {
            if constexpr (std::is_same_v<T, bool>) {
                return GetBool(m_key, m_defaultValue);
            } else if constexpr (std::is_integral_v<T>) {
                 return static_cast<T>(GetInt(m_key, static_cast<int>(m_defaultValue)));
            } else if constexpr (std::is_convertible_v<T, std::string>) {
                return static_cast<T>(GetString(m_key, std::string(m_defaultValue)));
            } else {
                static_assert(std::is_same_v<T, bool> || std::is_integral_v<T> || std::is_convertible_v<T, std::string>, "Unsupported type for Config");
                return m_defaultValue;
            }
        }

        void Set(const T& value) {
            if constexpr (std::is_same_v<T, bool>) {
                SetBool(m_key, value);
            } else if constexpr (std::is_integral_v<T>) {
                SetInt(m_key, static_cast<int>(value));
            } else if constexpr (std::is_convertible_v<T, std::string>) {
                SetString(m_key, std::string(value));
            }
        }

        // Implicit conversion to T
        operator T() const {
            return Get();
        }

        // Assignment operator
        Config& operator=(const T& value) {
            Set(value);
            return *this;
        }

    private:
        std::string m_key;
        T m_defaultValue;
    };
}
