#pragma once

#include <filesystem>
#include <string>

#include "Actor.hpp"
#include "EngineUtil.hpp"
#include "rapidjson/document.h"

class TemplateDB {
  public:
    void loadTemplate(std::string templateName, Actor &actor);

  private:
};