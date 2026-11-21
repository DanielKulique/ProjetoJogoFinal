#include "Portal.hpp"

Portal::Portal() {
    active = false;
    collision.halfExtents = {24, 24};
}

void Portal::activate(const Entity& player) noexcept {
    active = true;
    m_waitingForExit = bounds().intersects(player.bounds());
}

void Portal::deactivate() noexcept {
    active = false;
    m_waitingForExit = false;
}

bool Portal::tryEnter(const Entity& player) noexcept {
    if (!active || !player.active) return false;
    const bool touching = bounds().intersects(player.bounds());
    if (m_waitingForExit) {
        if (!touching) m_waitingForExit = false;
        return false;
    }
    return touching;
}
