#include "component/selection_registry/registry_store.h"

#include "algorithm/fmt_join.h"
#include "format/std_type.h"

namespace logicsim {

namespace selection_registry {

//
// Control Object
//

ControlObject::ControlObject(RegistryStore& store, selection_id_t selection_id)
    : registry_store_ {&store}, selection_id_ {selection_id} {}

ControlObject::~ControlObject() {
    clear();
}

auto ControlObject::format() const -> std::string {
    return fmt::format("SelectionControlObject({}, selection_id = {})",
                       static_cast<void*>(registry_store_), selection_id_);
}

auto ControlObject::holds_selection() const -> bool {
    return registry_store_ && selection_id_;
}

auto ControlObject::clear() -> void {
    if (holds_selection()) {
        const auto delete_id = selection_id_;

        // First free our ownership
        registry_store_ = nullptr;
        selection_id_ = null_selection_id;

        // Delegate deletion to registry, now that we freed our ownership.
        Ensures(!holds_selection());
        registry_store_->destroy_selection(delete_id);
    }
}

auto ControlObject::selection_id() const -> selection_id_t {
    return selection_id_;
}

//
// Controlled Selection
//

auto controlled_selection_t::format() const -> std::string {
    return fmt::format("({}, Selection = {})", control_object, selection);
}

//
// Registry Store
//

RegistryStore::~RegistryStore() {
    clear();
}

RegistryStore::RegistryStore(RegistryStore&& other) noexcept {
    this->swap(other);
}

auto RegistryStore::operator=(RegistryStore&& other) noexcept -> RegistryStore& {
    auto temp = RegistryStore {std::move(other)};
    swap(temp);
    return *this;
}

auto RegistryStore::swap(RegistryStore& other) noexcept -> void {
    using std::swap;
    swap(selections_, other.selections_);
    // TODO update all references
}

auto RegistryStore::format() const -> std::string {
    const auto item_str = fmt_join(",\n", selections_.values());
    return fmt::format("RegistryStore({})", item_str);
}

auto RegistryStore::clear() -> void {
    while (!selections_.empty()) {
        const auto selection_id = selections_.begin()->first;
        destroy_selection(selection_id);
    }

    Ensures(selections_.empty());
}

auto RegistryStore::create_selection() -> std::shared_ptr<ControlObject> {
    const auto selection_id = next_selection_key_++;
    auto control_ptr = std::make_shared<ControlObject>(*this, selection_id);

    auto&& [it, inserted] = selections_.emplace(
        selection_id, controlled_selection_t {
                          .selection = Selection {},
                          .control_object = std::weak_ptr(control_ptr),
                      });

    Ensures(inserted);
    Ensures(control_ptr && control_ptr->holds_selection());

    return control_ptr;
}

auto RegistryStore::destroy_selection(selection_id_t selection_id) -> void {
    const auto it = selections_.find(selection_id);

    if (it == selections_.end()) {
        throw std::runtime_error("Selection not found in destroy_selection");
    }

    if (auto control_ptr = it->second.control_object.lock();
        control_ptr && control_ptr->holds_selection()) {
        // If the control object still holds this resource, delegate deletion to it.
        // Note this will call this method again, but this time it is not owned.
        control_ptr->clear();
    } else {
        // We are the sole owner of the selection, so we can delete it.
        selections_.erase(it);
    }
}

auto RegistryStore::selection(selection_id_t selection_id) -> Selection& {
    const auto it = selections_.find(selection_id);

    if (it == selections_.end()) {
        throw std::runtime_error("Selection not found in registry store.");
    }

    return it->second.selection;
}

auto RegistryStore::selection(selection_id_t selection_id) const -> const Selection& {
    const auto it = selections_.find(selection_id);

    if (it == selections_.end()) {
        throw std::runtime_error("Selection not found in registry store.");
    }

    return it->second.selection;
}

auto swap(RegistryStore& a, RegistryStore& b) noexcept -> void {
    a.swap(b);
}

}  // namespace selection_registry

}  // namespace logicsim
