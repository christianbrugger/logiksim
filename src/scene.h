#ifndef LOGIKSIM_SCENE_H
#define LOGIKSIM_SCENE_H

#include "circuit.h"
#include "render_scene.h"
#include "simulation.h"

namespace logicsim {

class SceneManager {
   public:
    SceneManager() = default;

    auto add_line() -> void {}

   private:
    Circuit circuit;
    Simulation simulation;
    SimulationScene scene;
};

}  // namespace logicsim

#endif