#include "core_shared/logicsim_core_shared.h"

#include "core/circuit_example.h"
#include "core/editable_circuit.h"
#include "core/vocabulary/simulation_config.h"
#include "core/vocabulary/view_config.h"

#include <gsl/gsl>

namespace logicsim {

//
// Exported Circuit Impl
//

class ExportedCircuit_Impl {
   public:
    auto load_circuit(int number) -> void;

   private:
    ViewConfig view_config_ {};
    SimulationConfig simulation_config_ {};
    EditableCircuit editable_circuit_ {Layout {},
                                       EditableCircuit::Config {.enable_history = true}};
};

auto ExportedCircuit_Impl::load_circuit(int number) -> void {
    editable_circuit_ = load_example_with_logging(number);
}

}  // namespace logicsim

//
// C DLL Interface
//

// extern "C" {
struct ls_circuit {
    logicsim::ExportedCircuit_Impl data;
};

// }

template <typename Func>
auto ls_translate_exception(Func&& func) {
    try {
        return std::invoke(func);
    } catch (...) {
        std::terminate();
    }
}

auto ls_circuit_construct() -> ls_circuit_t {
    return ls_translate_exception([]() { return new ls_circuit; });
}

auto ls_circuit_destruct(ls_circuit_t obj) -> void {
    ls_translate_exception([&]() { delete obj; });
}

auto ls_circuit_load(ls_circuit_t obj, int32_t example_circuit) -> void {
    ls_translate_exception([&]() {
        Expects(obj);
        obj->data.load_circuit(example_circuit);
    });
}

auto ls_test() -> int {
    return ls_translate_exception([]() { return 13; });
}
