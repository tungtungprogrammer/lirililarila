#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// Base class of everything in the place tree (like Roblox's Instance).
class Instance : public std::enable_shared_from_this<Instance> {
public:
    std::string name = "Instance";

    virtual ~Instance();
    virtual std::string className() const { return "Instance"; }

    Instance* parent() const { return m_parent; }
    const std::vector<std::shared_ptr<Instance>>& children() const { return m_children; }

    void addChild(const std::shared_ptr<Instance>& child);
    void insertChild(std::size_t index, const std::shared_ptr<Instance>& child);
    std::shared_ptr<Instance> removeChild(const Instance* child);   // returns the owning pointer
    std::shared_ptr<Instance> detach();                             // remove from parent
    std::size_t indexOf(const Instance* child) const;               // == children().size() if absent

private:
    Instance* m_parent = nullptr;
    std::vector<std::shared_ptr<Instance>> m_children;
};
