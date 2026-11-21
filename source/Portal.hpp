#pragma once

#include "Entity.hpp"

class Portal : public Entity {
public:
    Portal();
    void activate(const Entity& player) noexcept;
    void deactivate() noexcept;
    [[nodiscard]] bool tryEnter(const Entity& player) noexcept;
    [[nodiscard]] bool waitingForExit() const noexcept { return m_waitingForExit; }

private:
    bool m_waitingForExit{false};
};
