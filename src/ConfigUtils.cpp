#include "ConfigUtils.h"
#include "FileUtils.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <shared_mutex>


namespace ConfigUtils {
namespace {
std::string g_pluginName;
std::filesystem::path g_configPath;
nlohmann::json g_configData;
std::shared_mutex g_mutex;
bool g_isLoaded = false;

// Assumes mutex is held by caller
void LoadConfig() {
  std::string content = FileUtils::ReadBinaryFile(g_configPath);
  if (content.empty()) {
    g_configData = nlohmann::json::object();
    return;
  }

  try {
    g_configData = nlohmann::json::parse(content);
  } catch (const std::exception &) {
    g_configData = nlohmann::json::object();
  }
}

void SaveConfig() {
  std::string content;
  {
    std::shared_lock<std::shared_mutex> lock(g_mutex);
    if (g_configPath.empty()) {
      return;
    }
    content = g_configData.dump(4);
  }
  FileUtils::WriteBinaryFile(g_configPath, content);
}
} // namespace

void EnsureConfigLoaded() {
  {
    std::shared_lock<std::shared_mutex> lock(g_mutex);
    if (g_isLoaded) {
      return;
    }
  }

  std::unique_lock<std::shared_mutex> lock(g_mutex);
  if (g_isLoaded) {
    return;
  }

  if (g_pluginName.empty()) {
    g_pluginName = FileUtils::GetCurrentModuleName();
  }

  if (!g_pluginName.empty()) {
    // Construct path: FileUtils::GetExecutableDirectory() / "reframework" / "data" /
    // (pluginName + ".json")
    g_configPath = FileUtils::GetExecutableDirectory() / "reframework" / "data" /
                   (g_pluginName + ".json");
    LoadConfig();
  }

  g_isLoaded = true;
}

bool GetBool(const std::string &key, bool defaultValue) {
  EnsureConfigLoaded();
  std::shared_lock<std::shared_mutex> lock(g_mutex);
  if (g_configData.contains(key)) {
    const auto &val = g_configData[key];
    if (val.is_boolean()) {
      return val.get<bool>();
    }
  }
  return defaultValue;
}

void SetBool(const std::string &key, bool value) {
  EnsureConfigLoaded();
  {
    std::unique_lock<std::shared_mutex> lock(g_mutex);
    g_configData[key] = value;
  }
  SaveConfig();
}

int GetInt(const std::string &key, int defaultValue) {
  EnsureConfigLoaded();
  std::shared_lock<std::shared_mutex> lock(g_mutex);
  if (g_configData.contains(key)) {
    const auto &val = g_configData[key];
    if (val.is_number_integer()) {
      return val.get<int>();
    }
  }
  return defaultValue;
}

void SetInt(const std::string &key, int value) {
  EnsureConfigLoaded();
  {
    std::unique_lock<std::shared_mutex> lock(g_mutex);
    g_configData[key] = value;
  }
  SaveConfig();
}

std::string GetString(const std::string &key, const std::string &defaultValue) {
  EnsureConfigLoaded();
  std::shared_lock<std::shared_mutex> lock(g_mutex);
  if (g_configData.contains(key)) {
    const auto &val = g_configData[key];
    if (val.is_string()) {
      return val.get<std::string>();
    }
  }
  return defaultValue;
}

void SetString(const std::string &key, const std::string &value) {
  EnsureConfigLoaded();
  {
    std::unique_lock<std::shared_mutex> lock(g_mutex);
    g_configData[key] = value;
  }
  SaveConfig();
}
} // namespace ConfigUtils
