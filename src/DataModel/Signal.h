#pragma once
#include <functional>
#include <utility>
#include <vector>

// Tiny signal/slot helper so the data model stays free of Qt and G3D.
template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    void connect(Slot s) { m_slots.push_back(std::move(s)); }

    void operator()(Args... args) const {
        const std::vector<Slot> copy = m_slots;   // safe if a slot connects another slot
        for (const Slot& s : copy) { s(args...); }
    }

private:
    std::vector<Slot> m_slots;
};
