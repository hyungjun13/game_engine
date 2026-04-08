#pragma once

#include "rapidjson/document.h"
#include <string>

class EngineUtil {
  public:
    static void ReadJsonFile(const std::string &path, rapidjson::Document &out_document);

    static void startup();

  private:
};
