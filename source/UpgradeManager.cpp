#include "UpgradeManager.hpp"
#include "Weapon.hpp"

#include <algorithm>
#include <limits>

std::optional<WeaponUpgrade> UpgradeManager::unlockForRound(int round, std::mt19937& random, const Weapon* weapon) {
    if (round <= m_lastRound || round <= 0) return std::nullopt;
    std::optional<WeaponUpgrade> last;
    while (m_lastRound < round) {
        if (m_bag.empty()) {
            for (std::size_t i = 0; i < upgradeKindCount; ++i) m_bag.push_back(static_cast<UpgradeKind>(i));
        }
        const auto nextTier = [&](UpgradeKind kind) {
            const auto index = static_cast<std::size_t>(kind);
            const int previous = weapon ? std::max(m_tiers[index], weapon->acquiredTier(kind)) : m_tiers[index];
            return previous < std::numeric_limits<int>::max() ? previous + 1 : previous;
        };
        std::vector<UpgradeKind> eligible;
        for (const auto kind : m_bag) {
            if (!weapon || weapon->canApply({kind, nextTier(kind), 1})) eligible.push_back(kind);
        }
        // Se restaram apenas atributos saturados na sacola, uma variante util de
        // outro tipo substitui esses candidatos. Nao desbloqueia um item sem efeito.
        if (eligible.empty() && weapon) {
            for (std::size_t i = 0; i < upgradeKindCount; ++i) {
                const auto kind = static_cast<UpgradeKind>(i);
                if (weapon->canApply({kind, nextTier(kind), 1})) eligible.push_back(kind);
            }
        }
        // O modo historico tem teto intencional e nao participa da progressao procedural.
        if (eligible.empty()) eligible = m_bag;
        if (eligible.size() > 1 && !m_unlocked.empty()) {
            eligible.erase(std::remove(eligible.begin(), eligible.end(), m_unlocked.back().kind), eligible.end());
        }
        const auto kind = eligible[static_cast<std::size_t>(random()) % eligible.size()];
        const auto bagEntry = std::find(m_bag.begin(), m_bag.end(), kind);
        if (bagEntry != m_bag.end()) m_bag.erase(bagEntry);
        const int tier = nextTier(kind);
        m_tiers[static_cast<std::size_t>(kind)] = tier;
        last = WeaponUpgrade{kind, tier, 1};
        m_unlocked.push_back(*last);
        ++m_lastRound;
    }
    return last;
}

std::optional<WeaponUpgrade> UpgradeManager::chooseUpgrade(const Weapon& weapon, std::mt19937& random) const {
    std::vector<const WeaponUpgrade*> eligible;
    for (const auto& upgrade : m_unlocked) if (weapon.canApply(upgrade)) eligible.push_back(&upgrade);
    if (eligible.empty()) return std::nullopt;
    return *eligible[static_cast<std::size_t>(random()) % eligible.size()];
}

void UpgradeManager::reset() noexcept {
    m_lastRound = 0;
    m_tiers.fill(0);
    m_bag.clear();
    m_unlocked.clear();
}
