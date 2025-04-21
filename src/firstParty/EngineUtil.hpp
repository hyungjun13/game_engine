#pragma once

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include "Engine.hpp"

#include <iostream>

class EngineUtil {
  public:
    static void ReadJsonFile(const std::string &path, rapidjson::Document &out_document) {
        FILE *file_pointer = nullptr;
#ifdef _WIN32
        fopen_s(&file_pointer, path.c_str(), "rb");
#else
        file_pointer = fopen(path.c_str(), "rb");
#endif
        char                      buffer[65536];
        rapidjson::FileReadStream stream(file_pointer, buffer, sizeof(buffer));
        out_document.ParseStream(stream);
        std::fclose(file_pointer);

        if (out_document.HasParseError()) {
            rapidjson::ParseErrorCode errorCode = out_document.GetParseError();
            std::cout << "error parsing json at [" << path << "]" << '\n';
            exit(0);
        }
    }

    static void startup();

  private:
};
