#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "global.h"
struct SectionInfo {
    SectionInfo() {}
    ~SectionInfo() {
        _section_datas.clear();
    }

    SectionInfo(const SectionInfo &src);

    SectionInfo &operator=(const SectionInfo &src);

    std::map<std::string, std::string> _section_datas;
    std::string operator[](const std::string &key);
};

class ConfigManager
{
public:
  ~ConfigManager();

  SectionInfo operator[](const std::string &section);

  static ConfigManager &get_instance() {
      static ConfigManager cfg_mgr;
      return cfg_mgr;
  }

private:
    ConfigManager();
    ConfigManager& operator=(const ConfigManager& src) = delete;
    ConfigManager(const ConfigManager& src) = delete;

    std::map<std::string, SectionInfo> _config_map;
};

#endif /* CONFIGMANAGER_H */
