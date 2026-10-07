#include "Instance.h"
#include <algorithm>

void Instance::addChild(const std::shared_ptr<Instance>& child) {
    if (child->m_parent) { child->m_parent->removeChild(child.get()); }
    child->m_parent = this;
    m_children.push_back(child);
}

void Instance::removeChild(const Instance* child) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const std::shared_ptr<Instance>& c) { return c.get() == child; });
    if (it != m_children.end()) {
        (*it)->m_parent = nullptr;
        m_children.erase(it);
    }
}

void Instance::destroy() {
    if (m_parent) { m_parent->removeChild(this); }
}
