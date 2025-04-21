#include "Actor.hpp"
#include <memory>
#include <string>
#include <vector>

class SceneLoader {
  public:
    static void                                       loadActors(std::string scene_path);
    inline static std::vector<std::shared_ptr<Actor>> loadedActors;

  private:
    inline static int idCounter = 0;
};