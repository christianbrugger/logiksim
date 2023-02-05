#ifndef LOGIKSIM_SCENE_H
#define LOGIKSIM_SCENE_H

#include "circuit.h"
#include "render_scene.h"
#include "simulation.h"

//
// Tasks:
// * store scene related data
//   * position, orientation
//   * line tree
// * handle collision
//   * kd-tree like structure
//   * store bounding boxes
//   * store inputs & outputs
//   * tell if new element can be inserted or not
// * querying
//   * query items in selection
//   * query item at position
//   * query input at position
//
// * get 2d position of inputs & outputs of element
//

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