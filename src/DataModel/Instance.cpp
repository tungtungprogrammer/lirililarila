#include "Instance.h"
#include <algorithm>

Instance::~Instance() {
    for (auto& c : m_children) { c->m_parent = nullptr; }
}

void Instance::addChild(const std::shared_ptr<Instance>& child) {
    insertChild(m_children.size(), child);
}

void Instance::insertChild(std::size_t index, const std::shared_ptr<Instance>& child) {
    if (child->m_parent) { child->m_parent->removeChild(child.get()); }
    child->m_parent = this;
    if (index > m_children.size()) { index = m_children.size(); }
    m_children.insert(m_children.begin() + static_cast<std::ptrdiff_t>(index), child);
}

std::shared_ptr<Instance> Instance::removeChild(const Instance* child) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const std::shared_ptr<Instance>& c) { return c.get() == child; });
    if (it == m_children.end()) { return nullptr; }
    std::shared_ptr<Instance> owned = *it;
    owned->m_parent = nullptr;
    m_children.erase(it);
    return owned;
}

std::shared_ptr<Instance> Instance::detach() {
    return m_parent ? m_parent->removeChild(this) : nullptr;
}

std::size_t Instance::indexOf(const Instance* child) const {
    for (std::size_t i = 0; i < m_children.size(); ++i) {
        if (m_children[i].get() == child) { return i; }
    }
    return m_children.size();
}
