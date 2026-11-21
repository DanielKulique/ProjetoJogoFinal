#include "Game.hpp"
#include "ProjectilePattern.hpp"
#include "UpgradeManager.hpp"
#include "WeaponUpgrade.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>

struct GameTestAccess {
    static void finishWave(Game& game) {
        while (game.m_waves.pending() > 0) {
            game.m_waves.update(100);
            game.m_waves.confirmSpawn();
        }
        game.m_enemies.clear();
    }
    static void pickup(Game& game, Pickup pickup) { game.m_pickups.push_back(std::move(pickup)); }
    static void asteroid(Game& game, Asteroid asteroid) { game.m_asteroids.push_back(std::move(asteroid)); }
};

namespace {
constexpr float dt = 1.0f / 60;
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b, float epsilon = 0.001f) { return std::abs(a-b) < epsilon; }
TileMap arena() { return TileMap(TileMap::Matrix(15, std::string(19, '.'))); }
GameConfig quiet() {
    GameConfig config;
    config.asteroid.spawnChance = config.gravity.spawnChance = 0;
    config.seed = 1001;
    return config;
}
bool different(const WeaponStats& a, const WeaponStats& b) {
    return !near(a.damage,b.damage,0.00001f) || !near(a.cooldown,b.cooldown,0.00001f) ||
        !near(a.projectileSpeed,b.projectileSpeed,0.00001f) || !near(a.lifetime,b.lifetime,0.00001f) ||
        a.projectiles!=b.projectiles || a.burstCount!=b.burstCount ||
        !near(a.spreadDegrees,b.spreadDegrees,0.00001f) || !near(a.formationRadius,b.formationRadius,0.00001f);
}

void testSeededUnlocks() {
    std::mt19937 a(112),b(112),c(113);
    UpgradeManager first, same, other;
    Weapon unmodified;
    std::set<UpgradeKind> firstKinds;
    bool diversity = false;
    for (int round = 1; round <= 60; ++round) {
        const auto x = first.unlockForRound(round,a);
        const auto y = same.unlockForRound(round,b);
        const auto z = other.unlockForRound(round,c);
        require(x && y && z, "Cada novo round desbloqueia uma definicao");
        require(x->kind==y->kind && x->tier==y->tier && near(x->strength,y->strength),
                "Mesma semente reproduz sequencia completa de desbloqueios");
        diversity = diversity || x->kind!=z->kind || x->tier!=z->tier;
        require(!first.unlockForRound(round,a), "Um round nao desbloqueia duas vezes");
        require(first.unlocked().size()==static_cast<std::size_t>(round), "Um registro por round");
        if (round <= static_cast<int>(upgradeKindCount)) require(firstKinds.insert(x->kind).second,
            "Nao repete tipos basicos antes de esgotar o conjunto");
        else require(x->tier>1, "Apos tipos basicos continuam variantes parametrizadas");
    }
    require(diversity && firstKinds.size()==upgradeKindCount, "Sementes distintas produzem ordem diferente cobrindo tipos");
    require(unmodified.acquiredCount()==0 && unmodified.pattern()==ProjectilePattern::Standard,
            "Desbloquear nao concede melhorias automaticamente");
    first.reset();
    require(first.unlocked().empty(), "Reset limpa desbloqueios");
}

void testModifiersAndBounds() {
    Weapon base;
    const auto initial = base.effectiveStats();
    Weapon heavy;
    require(heavy.applyUpgrade({UpgradeKind::Heavy,1,1}), "Aplica tiro pesado");
    require(heavy.effectiveStats().damage>initial.damage && heavy.cooldown()>initial.cooldown &&
            heavy.effectiveStats().projectileSpeed<initial.projectileSpeed, "Pesado troca frequencia/velocidade por dano");
    Weapon rate;
    rate.applyUpgrade({UpgradeKind::FireRate,1,1});
    require(rate.cooldown()<initial.cooldown && rate.effectiveStats().damage<initial.damage,
            "Cadencia troca potencia individual por frequencia");
    Weapon damage;
    damage.applyUpgrade({UpgradeKind::Damage,1,1});
    require(damage.effectiveStats().damage>initial.damage && damage.cooldown()>initial.cooldown,
            "Dano troca frequencia por potencia");
    Weapon speed;
    speed.applyUpgrade({UpgradeKind::ProjectileSpeed,1,1});
    require(speed.effectiveStats().projectileSpeed>initial.projectileSpeed && speed.effectiveStats().damage<initial.damage,
            "Velocidade maior cobra custo de potencia");
    Weapon lifetime;
    lifetime.applyUpgrade({UpgradeKind::Lifetime,1,1});
    require(lifetime.effectiveStats().lifetime>initial.lifetime && lifetime.effectiveStats().projectileSpeed<initial.projectileSpeed,
            "Duracao troca velocidade por alcance temporal");
    Weapon combined;
    require(combined.applyUpgrade({UpgradeKind::Damage,1,1}) && combined.applyUpgrade({UpgradeKind::FireRate,1,1}) &&
            combined.applyUpgrade({UpgradeKind::Circle,1,1}), "Modificadores compativeis coexistem com padrao");
    require(!combined.applyUpgrade({UpgradeKind::Circle,1,1}) && !combined.applyUpgrade({UpgradeKind::Damage,0,1}),
            "Duplicatas e niveis invalidos nao consomem melhoria");
    require(combined.applyUpgrade({UpgradeKind::Square,1,1}) && combined.pattern()==ProjectilePattern::Square &&
            combined.acquiredTier(UpgradeKind::Damage)==1, "Padrao substitui padrao sem apagar modificadores passivos");
    require(!combined.canApply({UpgradeKind::Damage,2,-1}), "Forca invalida rejeitada");

    std::mt19937 random(41);
    UpgradeManager manager;
    Weapon growing;
    for (int round=1; round<=600; ++round) {
        require(manager.unlockForRound(round,random,&growing).has_value(), "Novo round disponibiliza variante util");
        const auto option=manager.chooseUpgrade(growing,random);
        require(option.has_value() && growing.canApply(*option), "Round tardio ainda oferece melhoria aplicavel");
        const auto old = growing.effectiveStats();
        const auto oldPattern = growing.pattern();
        require(growing.applyUpgrade(*option), "Melhoria sorteada e adquirida");
        const auto stats = growing.effectiveStats();
        require(oldPattern!=growing.pattern() || different(old,stats), "Melhoria aplicada tem efeito observavel");
        require(stats.projectiles <= growing.config().maxProjectilesPerEmission &&
                stats.burstCount <= growing.config().maxBurstShots && stats.damage>=1 &&
                stats.cooldown>=growing.config().minimumCooldown-0.001f &&
                stats.projectileSpeed<=growing.config().maximumProjectileSpeed+0.01f &&
                stats.lifetime<=growing.config().maximumLifetime+0.001f,
                "Progressao tardia conserva limites de custo e projeteis");
    }
    require(growing.acquiredCount()>3, "Nao existe teto global de tres aquisicoes");
}

void testGeometry() {
    const Vector2D muzzle{200,300};
    const auto circle = projectilePatternRays(ProjectilePattern::Circle,muzzle,{0,1},8,30,20);
    Vector2D sum;
    for (const auto& ray:circle) {
        require(near((ray.position-muzzle).length(),20) && near(ray.direction.length(),1), "Circunferencia e direcoes unitarias");
        require((ray.position-muzzle).normalized().dot(ray.direction)>0.999f, "Circular dispara radialmente");
        sum+=ray.position-muzzle;
    }
    require(sum.length()<0.001f, "Circunferencia cobre direcoes sem centro deslocado");
    for (const auto pattern:{ProjectilePattern::Triangle,ProjectilePattern::Square}) {
        const int count=pattern==ProjectilePattern::Triangle?3:4;
        const auto rays=projectilePatternRays(pattern,muzzle,{0,1},count,30,20);
        const float edge=(rays[1].position-rays[0].position).length();
        Vector2D centroid;
        for (int i=0;i<count;++i) {
            require(near((rays[(i+1)%count].position-rays[i].position).length(),edge), "Vertices formam poligono regular");
            require(rays[i].direction.equals({0,1}), "Formacao acompanha orientacao escolhida");
            centroid+=rays[i].position-muzzle;
        }
        require(centroid.length()<0.001f, "Triangulo/quadrado centrados na origem de tiro");
    }
    const auto parallel=projectilePatternRays(ProjectilePattern::Parallel,muzzle,{1,0},3,30,20);
    require(parallel.front().position.y<parallel.back().position.y, "Paralelos possuem origens laterais distintas");
    for (const auto& ray:parallel) require(ray.direction.equals({1,0}), "Paralelos conservam direcao");
    const auto spread=projectilePatternRays(ProjectilePattern::Spread,muzzle,{1,0},3,40,20);
    require(spread.front().direction.y<0 && spread.back().direction.y>0 && spread[1].direction.equals({1,0}),
            "Espalhamento cobre angulos nos dois lados da mira");
}

void testBurstAndCadence() {
    Weapon burst;
    burst.applyUpgrade({UpgradeKind::Burst,1,1});
    const auto stats=burst.effectiveStats();
    require(stats.burstCount>=3, "Rajada possui projeteis consecutivos");
    auto first=burst.fireVolley({100,100},{1,0});
    require(first.size()==1 && burst.pollProjectiles({100,100},{1,0}).empty(), "Rajada comeca por um unico tiro");
    burst.update(stats.burstInterval*0.5f);
    require(burst.pollProjectiles({100,100},{1,0}).empty(), "Segundo tiro espera intervalo");
    burst.update(stats.burstInterval*0.5f);
    auto next=burst.pollProjectiles({100,100},{1,0});
    require(next.size()==1, "Segundo tiro aparece apos tempo de simulacao");
    std::size_t count=first.size()+next.size();
    for(int i=0;i<120;++i) {
        burst.update(dt);
        count+=burst.pollProjectiles({100,100},{1,0}).size();
    }
    require(count==static_cast<std::size_t>(stats.burstCount), "Rajada termina uma vez mesmo com gatilho solto");
    require(burst.pollProjectiles({100,100},{1,0}).empty(), "Nao duplica tiros da rajada");
    Weapon normal;
    require(normal.fireVolley({}, {1,0}).size()==1 && normal.fireVolley({}, {1,0}).empty(), "Tiro comum respeita cooldown");
    normal.update(normal.cooldown());
    require(normal.fireVolley({}, {1,0}).size()==1, "Cooldown permite novo tiro depois do prazo");
}

void testRoundUnlockAcquisitionAndCombatRest() {
    Game game(arena(),quiet());
    require(game.upgrades().unlocked().size()==1 && game.player().weapon().acquiredCount()==0,
            "Round inicial desbloqueia sem adquirir");
    const auto unlocked=game.upgrades().unlocked().front();
    GameTestAccess::pickup(game,Pickup(game.player().body.position,PickupKind::WeaponUpgrade,{},unlocked));
    game.update(dt,{});
    require(game.player().weapon().acquiredCount()==1, "Coleta aplica variante desbloqueada");
    const auto current=game.player().weapon().summary();
    GameTestAccess::finishWave(game);
    game.update(dt,{});
    require(game.state()==GameState::RoundCompleted, "Fixture conclui combate normal");
    const auto target=game.player().body.position+Vector2D{200,0};
    AsteroidConfig config;
    config.speed=1;
    config.maxFragmentDepth=0;
    GameTestAccess::asteroid(game,Asteroid(target,{1,0},config));
    for(int i=0;i<120;++i) game.update(dt,{{},target,true});
    require(game.asteroids().empty(), "Pode atirar e destruir asteroide durante RoundCompleted");
    require(game.player().weapon().summary()==current, "Espera pelo portal conserva configuracao da arma");
    for(int i=0;i<30;++i) game.update(dt,{{1,0},{},false});
    for(int i=0;i<180 && game.state()==GameState::RoundCompleted;++i)
        game.update(dt,{game.portal().body.position-game.player().body.position,{},false});
    require(game.waves().round()==2 && game.upgrades().unlocked().size()==2 &&
            game.player().weapon().acquiredCount()==1, "Portal desbloqueia proxima opcao sem aquisicao automatica");
    require(game.player().weapon().summary()==current, "Configuracao adquirida persiste entre rounds");
}
}

int main() {
    try {
        testSeededUnlocks();
        testModifiersAndBounds();
        testGeometry();
        testBurstAndCadence();
        testRoundUnlockAcquisitionAndCombatRest();
        std::cout << "Upgrades procedurais, geometria, rajadas e integracao de rounds: OK\n";
    } catch(const std::exception& error) {
        std::cerr << "FALHA: " << error.what() << '\n';
        return 1;
    }
}
