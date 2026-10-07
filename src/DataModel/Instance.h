#pragma once
#include <memory>
#include <string>
#include <vector>

// Base class of everything in the place tree (like Roblox's Instance).
class Instance {
public:
    std::string name = "Instance";

    virtual ~Instance() = default;
    virtual std::string className() const { return "Instance"; }

    Instance* parent() const { return m_parent; }
    const std::vector<std::shared_ptr<Instance>>& children() const { return m_children; }

    void addChild(const std::shared_ptr<Instance>& child);
    void removeChild(const Instance* child);   // destroys it if nothing else holds it
    void destroy();                            // remove self from parent

private:
    Instance* m_parent = nullptr;
    std::vector<std::shared_ptr<Instance>> m_children;
};
