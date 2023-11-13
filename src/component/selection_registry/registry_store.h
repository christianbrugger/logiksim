#ifndef LOGICSIM_COMPONENT_SELECTION_REGISTRY_REGISTRY_STORE_H
#define LOGICSIM_COMPONENT_SELECTION_REGISTRY_REGISTRY_STORE_H

#include "editable_circuit/selection.h"
#include "format/struct.h"
#include "vocabulary/selection_id.h"

#include <ankerl/unordered_dense.h>

#include <memory>

namespace logicsim {

namespace selection_registry {

class RegistryStore;

/**
 * @brief: Manages the Ownership of a selection in the SelectionStore.
 */
class ControlObject {
   public:
    [[nodiscard]] explicit ControlObject() = default;
    [[nodiscard]] explicit ControlObject(RegistryStore& store,
                                         selection_id_t selection_id);

    ~ControlObject();

    // prevent move, as we store pointers to this object
    ControlObject(ControlObject&& other) = delete;
    auto operator=(ControlObject&& other) = delete;

    // prevent copying
    ControlObject(const ControlObject& other) = delete;
    auto operator=(const ControlObject& other) = delete;

    [[nodiscard]] auto format() const -> std::string;

    /**
     * @brief: Returns true if this control objects holds a selection resource.
     */
    [[nodiscard]] auto holds_selection() const -> bool;

    /**
     * @brief: Breaks the link between control object and registry.
     *
     * Note this frees the selection resource, if this object holds one.
     */
    auto clear() -> void;

    /**
     * @brief: Return the selection-id of the held resource or null_selection_id.
     */
    [[nodiscard]] auto selection_id() const -> selection_id_t;

   private:
    RegistryStore* registry_store_ {nullptr};
    selection_id_t selection_id_ {null_selection_id};
};

struct controlled_selection_t {
    Selection selection;
    std::weak_ptr<ControlObject> control_object;

    [[nodiscard]] auto format() const -> std::string;
};

using selection_map_t =
    ankerl::unordered_dense::map<selection_id_t, controlled_selection_t>;

/**
 * @brief: Manages selection with ownership being managed externally.
 *
 * This class breaks the whole-part relationship. We however
 * need this to allow RHII style resource allocations of selections outside of
 * editable circuit. It is a compromise to prevent leaks in case of exceptions.
 *
 * Note this also breaks copy-ability and it needs to be handled outside of this class.
 */
class RegistryStore {
   public:
    ~RegistryStore();

    // allow move
    RegistryStore(RegistryStore&& other) noexcept;
    auto operator=(RegistryStore&& other) noexcept -> RegistryStore&;
    auto swap(RegistryStore& other) noexcept -> void;

    // prevent copying
    RegistryStore(const RegistryStore& other) = delete;
    auto operator=(const RegistryStore& other) = delete;

    [[nodiscard]] auto format() const -> std::string;

    auto clear() -> void;
    [[nodiscard]] auto create_selection() -> std::shared_ptr<ControlObject>;
    auto destroy_selection(selection_id_t selection_id) -> void;

    [[nodiscard]] auto selection(selection_id_t selection_id) -> Selection&;
    [[nodiscard]] auto selection(selection_id_t selection_id) const -> const Selection&;

   private:
    selection_map_t selections_ {};
    selection_id_t next_selection_key_ {0};
};

auto swap(RegistryStore& a, RegistryStore& b) noexcept -> void;

}  // namespace selection_registry

}  // namespace logicsim

#endif
