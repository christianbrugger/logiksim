#ifndef LOGICSIM_SELECTION_RESOURCE_H
#define LOGICSIM_SELECTION_RESOURCE_H

#include "component/selection_registry/registry_store.h"
#include "editable_circuit/selection_registrar.h"

#include <memory>

namespace logicsim {

/**
 * @brief: Holds the ownership of a Selection in EditableCircuit.
 *
 * This class breaks the whole-part relationship. As it contains a writable reference to
 * the Selection Registrar and allows some spooky action at the distance. We however
 * need this to allow RHII style resource allocations of selections outside of
 * editable circuit. It is a compromise to prevent leaks in case of exceptions.
 *
 * The impact of reference semantics is limited in two ways:
 *    + This class does not give any read or write access to the selection itself
 *      or anything else.
 *    + The action at a distance is limited to the destruction of the selection.
 *    + The class is unaffected by the order of destruction of this handle,
 *      the registry or the selection.
 *
 * This limits the scope of this class to two things:
 *    + hold the memory resource of a selection
 *    + get the selection_id referred to by the resource
 */
class SelectionResource {
   public:
    [[nodiscard]] explicit SelectionResource() = default;
    [[nodiscard]] explicit SelectionResource(
        std::shared_ptr<selection_registry::ControlObject> &&control_object);
    ~SelectionResource();

    // allow move
    SelectionResource(SelectionResource&& other) noexcept;
    auto operator=(SelectionResource&& other) noexcept -> SelectionResource&;
    auto swap(SelectionResource& other) noexcept -> void;

    // prevent copying
    SelectionResource(const SelectionResource& other) = delete;
    auto operator=(const SelectionResource& other) = delete;
    
    /**
     * @brief: Returns true if this control objects holds a selection resource.
     */
    [[nodiscard]] operator bool() const;

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
    std::shared_ptr<selection_registry::ControlObject> control_object_ {};
};

auto swap(SelectionResource& a, SelectionResource& b) noexcept -> void;

}  // namespace logicsim

#endif
