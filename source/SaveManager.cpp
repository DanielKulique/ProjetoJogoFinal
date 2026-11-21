#include "SaveManager.hpp"
#include "LocalStorage.hpp"
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
#include <charconv>
#include <cstdlib>
#include <type_traits>
#include <stdexcept>
#include <locale>

struct StateCodec {
    template<class A> static void fields(A& a, ScenarioInfo& v) { a(v.type,v.seed,v.csvPath); }
    template<class A> static void fields(A& a, Vector2D& v) { a(v.x,v.y); if(std::abs(v.x)>1e7f || std::abs(v.y)>1e7f)throw std::runtime_error("Vetor fora do limite"); }
    template<class A> static void fields(A& a, RigidBody2D& v) { a(v.position,v.velocity,v.acceleration); }
    template<class A> static void fields(A& a, Collision2D& v) { a(v.halfExtents); }
    template<class A> static void fields(A& a, WeaponUpgrade& v) { a(v.kind,v.tier,v.strength); }
    template<class A> static void fields(A& a, ScoreConfig& v) { a(v.normal,v.advanced,v.asteroid,v.fragment,v.round); }
    template<class A> static void fields(A& a, PlayerConfig& v) { a(v.engineSpeeds,v.damageInvulnerability,v.driftDamping,v.maxDrift); }
    template<class A> static void fields(A& a, WeaponConfig& v) { a(v.cooldown,v.projectileSpeed,v.damage,v.projectileLifetime,v.upgradedCooldown,v.spreadDegrees,v.maxLevel,v.legacyProgression,v.progressionStrength,v.maxProjectilesPerEmission,v.maxBurstShots,v.minimumCooldown,v.maximumProjectileSpeed,v.maximumLifetime); }
    template<class A> static void fields(A& a, WeaponStats& v) { a(v.damage,v.cooldown,v.projectileSpeed,v.lifetime,v.projectiles,v.spreadDegrees,v.burstCount,v.burstInterval,v.formationRadius); }
    template<class A> static void fields(A& a, EnemyConfig& v) { a(v.health,v.speed,v.damage,v.attackInterval,v.detectionRadius,v.lossRadius,v.wanderInterval,v.wanderRadius,v.wanderSpeedFactor,v.normalHalfExtent,v.advancedHalfExtent,v.explosionRadius,v.advancedHealthMultiplier,v.advancedSpeedMultiplier,v.driftDamping,v.maxDrift); }
    template<class A> static void fields(A& a, WaveConfig& v) { a(v.initialEnemies,v.additionalEnemies,v.spawnInterval,v.enemy,v.healthPerRound,v.speedPerRound,v.maxEnemies,v.maxActive,v.minSpawnInterval,v.spawnIntervalDecrease,v.advancedChance,v.maxSpeed); }
    template<class A> static void fields(A& a, AsteroidConfig& v) { a(v.radius,v.speed,v.health,v.damage,v.fragmentCount,v.fragmentScale,v.fragmentSpeedFactor,v.maxFragmentDepth,v.fragmentGrace,v.maxActive,v.spawnInterval,v.spawnChance,v.maxSpeed,v.linearDamping,v.lifetime); }
    template<class A> static void fields(A& a, GravityFieldConfig& v) { a(v.spawnInterval,v.spawnChance,v.radius,v.duration,v.accelerationMeters,v.pixelsPerMeter,v.softening,v.damping,v.maxDrift); }
    template<class A> static void fields(A& a, PickupConfig& v) { a(v.lifetime,v.halfExtent,v.maxActive); }
    template<class A> static void fields(A& a, DropConfig& v) { a(v.chance,v.upgradeChance,v.avoidFullEngineDrops); }
    template<class A> static void fields(A& a, GameConfig& v) { a(v.scoring,v.player,v.weapon,v.waves,v.asteroid,v.gravity,v.pickup,v.drops,v.seed,v.reseedOnRestart,v.maxProjectiles,v.maxExplosions,v.explosionLifetime); }
    template<class A> static void fields(A& a, GameStats& v) { a(v.shotsFired,v.projectileHits,v.enemyKills,v.playerKills,v.damageDealt,v.asteroidDamageDealt,v.damageReceived,v.enginesLost,v.enginesRecovered,v.upgradesCollected,v.roundsCompleted,v.peakProjectiles,v.patternMask,v.perPatternShots,v.perPatternHits,v.perPatternDamage,v.roundDurations); }
    template<class A> static void fields(A& a, Weapon& v) { a(v.m_config,v.m_level,v.m_acquiredCount,v.m_tiers,v.m_strengths,v.m_pattern,v.m_remainingCooldown,v.m_damageCarry,v.m_burstRemaining,v.m_burstTimer,v.m_burstStats); }
    template<class A> static void fields(A& a, UpgradeManager& v) { a(v.m_lastRound,v.m_tiers,v.m_bag,v.m_unlocked); }
    template<class A> static void fields(A& a, Player& v) { a(v.body,v.collision,v.active,v.id,v.m_config,v.m_weapon,v.m_engines,v.m_movementDirection,v.m_drift,v.m_damageCooldown,v.m_angleDegrees); }
    template<class A> static void fields(A& a, Enemy& v) { a(v.body,v.collision,v.active,v.id,v.m_config,v.m_kind,v.m_state,v.m_health,v.m_attackCooldown,v.m_random,v.m_wanderTimer,v.m_wanderTarget,v.m_drift,v.m_path,v.m_nextWaypoint,v.m_pathGoal,v.m_pathCooldown); }
    template<class A> static void fields(A& a, Asteroid& v) { a(v.body,v.collision,v.active,v.id,v.m_config,v.m_health,v.m_depth,v.m_remainingLife,v.m_collisionGrace,v.m_fragmented); }
    template<class A> static void fields(A& a, GravityField& v) { a(v.body,v.collision,v.active,v.id,v.m_config,v.m_remainingLife); }
    template<class A> static void fields(A& a, Pickup& v) { a(v.body,v.collision,v.active,v.id,v.m_kind,v.m_upgrade,v.m_config,v.m_remainingLife); }
    template<class A> static void fields(A& a, WaveManager& v) { a(v.m_config,v.m_round,v.m_pending,v.m_spawnTimer); }
    template<class A> static void fields(A& a, Portal& v) { a(v.body,v.collision,v.active,v.id,v.m_waitingForExit); }
    template<class A> static void fields(A& a, Projectile& v) { a(v.body,v.collision,v.active,v.id,v.patternId,v.m_damage,v.m_remainingLife,v.m_previousPosition); }
    template<class A> static void fields(A& a, Explosion& v) { a(v.position,v.radius,v.remainingLife); }
    template<class A> static void fields(A& a, Game& v) { a(v.m_score,v.m_stats,v.m_upgrades,v.m_roundStartTime,v.m_random,v.m_asteroids,v.m_pickups,v.m_gravityField,v.m_explosions,v.m_gameplayTime,v.m_asteroidTimer,v.m_gravityTimer,v.m_player,v.m_projectiles,v.m_enemies,v.m_waves,v.m_portal,v.m_state,v.m_spawnCursor,v.m_pendingEnemyKind); }
    static void validate(const Game& g) {
        g.m_config.validate();
        if(static_cast<unsigned>(g.m_scenario.type)>2)throw std::runtime_error("Tipo de cenario invalido");
        if (g.m_state != GameState::Playing && g.m_state != GameState::RoundCompleted) throw std::runtime_error("Estado nao continuavel");
        if(g.m_waves.m_round<1 || g.m_waves.m_pending<0 || g.m_waves.m_pending>g.m_config.waves.maxEnemies ||
           g.m_player.m_engines<1 || g.m_player.m_engines>4 || !g.m_player.active ||
           g.m_gameplayTime<0 || g.m_roundStartTime<0 || g.m_roundStartTime>g.m_gameplayTime ||
           g.m_enemies.size()>g.m_config.waves.maxActive || g.m_projectiles.size()>g.m_config.maxProjectiles ||
           g.m_asteroids.size()>g.m_config.asteroid.maxActive || g.m_pickups.size()>g.m_config.pickup.maxActive ||
           g.m_explosions.size()>g.m_config.maxExplosions || g.m_asteroidTimer < -EPSILON || g.m_gravityTimer < -EPSILON ||
           g.m_upgrades.m_lastRound<0 || g.m_upgrades.m_lastRound>g.m_waves.m_round) throw std::runtime_error("Estado fora dos limites");
        if(g.m_state==GameState::RoundCompleted && (!g.m_portal.active || !g.m_enemies.empty() || g.m_waves.m_pending!=0)) throw std::runtime_error("Portal inconsistente");
        const auto entity=[&](const Entity& e) {
            if(!e.active || e.collision.halfExtents.x<=0 || e.collision.halfExtents.y<=0 || !g.m_map.isAreaFree(e.bounds())) throw std::runtime_error("Corpo invalido");
        };
        const auto weapon=[](const Weapon& w) {
            const Weapon valid(w.m_config);
            if(w.m_level<1 || w.m_level>w.m_config.maxLevel || w.m_acquiredCount<0 || static_cast<unsigned>(w.m_pattern)>=8 || w.m_burstRemaining<0 || w.m_burstRemaining>w.m_config.maxBurstShots || w.m_remainingCooldown<0 || w.m_damageCarry<0 || w.m_damageCarry>=1) throw std::runtime_error("Arma invalida");
            for(int n:w.m_tiers) if(n<0) throw std::runtime_error("Tier invalido");
            if(w.m_burstStats.burstInterval<=0 || w.m_burstStats.projectiles<1 || w.m_burstStats.projectiles>64 || w.m_burstStats.damage<1) throw std::runtime_error("Rajada invalida");
        };
        if(g.m_pendingEnemyKind && static_cast<unsigned>(*g.m_pendingEnemyKind)>1)throw std::runtime_error("Tipo pendente invalido");
        if(g.m_spawnCursor>=std::max<std::size_t>(1,g.m_spawnCandidates.size()))throw std::runtime_error("Indice de spawn invalido");
        entity(g.m_player); weapon(g.m_player.m_weapon);
        const Player validPlayer(g.m_player.m_config,g.m_player.m_weapon.m_config);
        const WaveManager validWaves(g.m_waves.m_config);
        for(auto kind:g.m_upgrades.m_bag) if(static_cast<unsigned>(kind)>=upgradeKindCount) throw std::runtime_error("Catalogo invalido");
        for(const auto& u:g.m_upgrades.m_unlocked) if(!validUpgrade(u)) throw std::runtime_error("Oferta invalida");
        for(const auto& e:g.m_enemies) {
            if(!e) throw std::runtime_error("Inimigo ausente");
            entity(*e); const Enemy valid(e->body.position,e->m_config,e->m_kind);
            if(e->m_health<=0 || e->m_health>e->m_config.health || static_cast<unsigned>(e->m_kind)>1 || static_cast<unsigned>(e->m_state)>1 || (!e->m_path.empty() && e->m_nextWaypoint>=e->m_path.size())) throw std::runtime_error("Alien invalido");
        }
        for(const auto& p:g.m_projectiles) { entity(p); if(p.m_damage<=0 || p.m_remainingLife<=0 || p.patternId>=8) throw std::runtime_error("Projetil invalido"); }
        for(const auto& r:g.m_asteroids) { entity(r); const Asteroid valid({}, {1,0},r.m_config,r.m_depth); if(r.m_health<=0 || r.m_remainingLife<=0 || r.m_depth>r.m_config.maxFragmentDepth) throw std::runtime_error("Rocha invalida"); }
        for(const auto& p:g.m_pickups) { entity(p); const Pickup valid({},p.m_kind,p.m_config,p.m_upgrade); if(p.m_remainingLife<=0 || (p.m_upgrade && !validUpgrade(*p.m_upgrade))) throw std::runtime_error("Item invalido"); }
        if(g.m_gravityField) { const GravityField valid(g.m_gravityField->body.position,g.m_gravityField->m_config); if(!g.m_gravityField->active || g.m_gravityField->m_remainingLife<=0) throw std::runtime_error("Campo invalido"); }
    }
};

namespace {
template<class T> T emptyValue() {
    if constexpr(std::is_same_v<T,Projectile>) return Projectile({}, {1,0},1,1,1);
    else if constexpr(std::is_same_v<T,Asteroid>) return Asteroid({}, {1,0});
    else if constexpr(std::is_same_v<T,GravityField>) return GravityField({});
    else if constexpr(std::is_same_v<T,Pickup>) return Pickup({},PickupKind::Engine);
    else return T{};
}
struct Writer {
    std::ostringstream stream;
    Writer() { stream.imbue(std::locale::classic()); stream << std::setprecision(std::numeric_limits<double>::max_digits10); }
    template<class... T> void operator()(T&... v) { (one(v),...); }
    template<class T> void one(T& v) {
        if constexpr(std::is_enum_v<T>) stream << static_cast<unsigned>(v) << ' ';
        else if constexpr(std::is_integral_v<T>) stream << +v << ' ';
        else if constexpr(std::is_floating_point_v<T>) { if(!std::isfinite(v)) throw std::runtime_error("Numero nao finito"); stream << v << ' '; }
        else StateCodec::fields(*this,v);
    }
    void one(std::string& v) { stream << std::quoted(v) << ' '; }
    void one(std::mt19937& v) { std::ostringstream out; out.imbue(std::locale::classic()); out<<v; auto s=out.str();one(s); }
    template<class T,std::size_t N> void one(std::array<T,N>& v) { for(auto& x:v) one(x); }
    template<class T> void one(std::vector<T>& v) { auto n=v.size();one(n); for(auto& x:v) one(x); }
    template<class T> void one(std::optional<T>& v) { bool has=bool(v);one(has);if(has)one(*v); }
    template<class T> void one(std::unique_ptr<T>& v) { if(!v) throw std::runtime_error("Entidade nula"); one(*v); }
};
struct Reader {
    std::istringstream stream;
    explicit Reader(const std::string& data):stream(data) { if(data.size()>8*1024*1024) throw std::runtime_error("Estado grande demais");stream.imbue(std::locale::classic()); }
    template<class... T> void operator()(T&... v) { (one(v),...); }
    template<class T> void one(T& v) {
        if constexpr(std::is_enum_v<T>) { unsigned x;one(x);v=static_cast<T>(x); }
        else if constexpr(std::is_arithmetic_v<T>) {
            std::string token;if(!(stream>>token) || token.size()>128) throw std::runtime_error("Valor ausente");
            if constexpr(std::is_same_v<T,bool>) { if(token!="0" && token!="1") throw std::runtime_error("Booleano invalido");v=token=="1"; }
            else { auto r=std::from_chars(token.data(),token.data()+token.size(),v);if(r.ec!=std::errc{} || r.ptr!=token.data()+token.size()) throw std::runtime_error("Numero invalido"); if constexpr(std::is_floating_point_v<T>) if(!std::isfinite(v)) throw std::runtime_error("Numero nao finito"); }
        } else StateCodec::fields(*this,v);
    }
    void one(std::string& v) { if(!(stream>>std::quoted(v)) || v.size()>65536) throw std::runtime_error("Texto invalido"); }
    void one(std::mt19937& v) {
        std::string text;one(text);std::istringstream check(text);check.imbue(std::locale::classic());
        std::uint64_t x=0;for(int i=0;i<624;++i) if(!(check>>x) || x>std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("RNG invalido");
        std::string extra;if(!(check>>x) || x>624 || check>>extra) throw std::runtime_error("Indice RNG invalido");
        std::istringstream in(text);in.imbue(std::locale::classic());if(!(in>>v)) throw std::runtime_error("RNG invalido");
    }
    template<class T,std::size_t N> void one(std::array<T,N>& v) { for(auto& x:v)one(x); }
    template<class T> void one(std::vector<T>& v) { std::size_t n;one(n);if(n>100000) throw std::runtime_error("Colecao grande demais");v.clear();v.reserve(n);for(std::size_t i=0;i<n;++i) { auto x=emptyValue<T>();one(x);v.push_back(std::move(x)); } }
    template<class T> void one(std::optional<T>& v) { bool has;one(has);if(has) { auto x=emptyValue<T>();one(x);v=std::move(x); } else v.reset(); }
    template<class T> void one(std::unique_ptr<T>& v) { v=std::make_unique<T>(Vector2D{});one(*v); }
};
}
std::string SaveManager::snapshot(const Game& game) {
    StateCodec::validate(game);
    Writer out;auto& mutableGame=const_cast<Game&>(game);auto config=game.config();auto rows=game.map().rows();
    auto scenario=game.scenario();out(scenario,config,rows);StateCodec::fields(out,mutableGame);return out.stream.str();
}
std::unique_ptr<Game> SaveManager::restore(const std::string& payload,unsigned version) {
    if(version!=1 && version!=2)throw std::runtime_error("Versao de save nao suportada");
    Reader in(payload);GameConfig config;TileMap::Matrix rows;std::optional<ScenarioInfo> scenario;
    if(version==2){ScenarioInfo info;in(info);scenario=std::move(info);}
    in(config,rows);config.validate();
    if(rows.empty() || rows.size()>256 || rows.front().size()>256) throw std::runtime_error("Mapa fora do limite");
    auto game=std::make_unique<Game>(TileMap(rows),config,scenario);StateCodec::fields(in,*game);
    in.stream>>std::ws;if(!in.stream.eof()) throw std::runtime_error("Dados extras");StateCodec::validate(*game);return game;
}
SaveManager::SaveManager(std::filesystem::path directory):m_directory(std::move(directory)) {}
bool SaveManager::save(const Game& game,std::string& error) const {
    try { LocalStorage::write(m_directory/"session.sav","SPACE_ARENA_SAVE",snapshot(game),2);error.clear();return true; }
    catch(const std::exception& e) { error=e.what();return false; }
}
std::unique_ptr<Game> SaveManager::load(std::string& error) const {
    try { unsigned version=0;auto payload=LocalStorage::read(m_directory/"session.sav","SPACE_ARENA_SAVE",&version,2);auto g=restore(payload,version);error.clear();return g; }
    catch(const std::exception& e) { error=e.what();return {}; }
}
bool SaveManager::valid() const { std::string error;return bool(load(error)); }
bool SaveManager::invalidate(std::string& error) const { try { std::filesystem::remove(m_directory/"session.sav");error.clear();return true; } catch(const std::exception& e) {error=e.what();return false;} }
std::filesystem::path SaveManager::defaultDirectory() {
    if(const char* p=std::getenv("XDG_DATA_HOME");p && *p) return std::filesystem::path(p)/"space-arena";
    if(const char* p=std::getenv("HOME");p && *p) return std::filesystem::path(p)/".local/share/space-arena";
    return std::filesystem::temp_directory_path()/"space-arena";
}
