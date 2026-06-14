**An Object-Oriented Programming Semester Project**


---

## 🌌 What Is This Game?

**Space Invaders: Galactic Defense** is a top-down, single-player space shooter built entirely in
C++ using the SFML graphics library. You pilot a spacecraft through a dark, scrolling starfield,
blasting waves of alien enemies, collecting power-ups, dodging asteroid fields, and — if you
survive long enough — facing three escalating boss encounters.

The game was designed from the ground up as a **real-time demonstration of all core OOP
principles**: abstract classes, inheritance chains, polymorphism, composition, strategy pattern,
finite state machines, and manual memory management — all running at 60 fps.

```
Window Resolution : 1000 × 800 px
Language          : C++17
Graphics Library  : SFML 2.5.1
Memory Model      : Raw pointers, new / delete  (no STL containers)
```

> 📺 **Visual Reference:** https://www.youtube.com/watch?v=MU4psw3ccUI

---

## ✨ Features at a Glance

| Category | Details |
|---|---|
| 🎮 **Game Modes** | Arcade (3-boss campaign) & Survival (endless waves) |
| 👾 **Enemy Types** | Drone, Viper, Seeker (3 distinct behaviours) |
| 💀 **Bosses** | The Cruiser · Twin Cannons · The Mothership |
| 🔫 **Weapons** | Standard, Spread (3-way), Piercing (punch-through) |
| ⚡ **Power-Ups** | Spread Module, Pierce Module, Energy Shield, EMP Nuke |
| 🌠 **VFX** | Parallax starfield, particle explosions (object-pool), screen shake, laser beams |
| 🔊 **Audio** | BGM (menu/gameplay/boss), SFX for every action |
| 🛡️ **Player Systems** | Dash, Shield, EMP, Score Multiplier, Invincibility Frames |
| 🧠 **OOP Concepts** | 10 distinct OOP concepts demonstrated (see Architecture section) |

---

## 📸 Screenshot

```
┌─────────────────────────────────────────────────────────────────────┐
│  ★  SCORE: 12,400   ×4   ♥♥♥   [SPREAD]   🛡 ●●   EMP: 2   WAVE 6 │
│  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ MOTHERSHIP ░░░░░░░░░░░░░░░░░░░░░░░ │
│                          ★  ·   ·   ★                               │
│     ·    [VIPER]~~~     ★          ·      [DRONE]                   │
│  ★        ·                ·   ★              ·                     │
│                  💥 [ASTEROID]    ·       [SEEKER]↓                 │
│     ·              ·                  ·                ★            │
│              ·           ·     ·            ·                       │
│                   ·  ★        ·       ★          ·                  │
│                        ↑↑  [PLAYER]  ↑↑                            │
│ ══════════════════════[ DASH COOLDOWN ██████░░░ ]════════════════  │
└─────────────────────────────────────────────────────────────────────┘
         (Place an actual in-game screenshot here as screenshot.png)
```

---

## 🏗️ OOP Architecture

This is the most important section for understanding how and **why** the code is structured the
way it is. Every design decision maps to a real OOP concept.

---

### 📐 Full Class Hierarchy

```
GameObject          ← Pure abstract base (update / draw / onCollision)
│
└── Entity          ← Adds: posX, posY, velX, velY, health, damage, width, height
      │
      ├── Bullet    ← owner tag ("Player"/"Enemy"), hitsLeft for pierce
      │
      ├── PowerUp   ← type: 0=Spread 1=Pierce 2=Shield 3=EMP
      │
      ├── Asteroid  ← health=999, respawns at top, never truly dies
      │
      ├── Player    ← lives, score, multiplier, Weapon*, empCount,
      │               shieldHits, iFrames, dashCooldown
      │
      └── Enemy     ← adds speedScale, fireScale
            │
            ├── Drone    ← falls straight, fires randomly downward
            │
            ├── Viper    ← sinusoidal X movement, fires downward
            │
            ├── Seeker   ← kamikaze: locks player X at spawn,
            │              accelerates to 650 px/s, never fires
            │
            └── Boss     ← adds maxHP, phase 1/2/3, virtual attackPattern()
                  │
                  ├── Cruiser       ← Level 1: laser curtain with safe gap
                  │
                  ├── TwinCannons   ← Level 2: COMPOSES two Turret* objects
                  │
                  └── Mothership    ← Level 3: screen laser + seeker swarms

─────────────────────────────────────────────────────
Turret  ← NOT in the hierarchy. A plain class owned by
          TwinCannons via COMPOSITION (has-a, not is-a).
          Has its own AABB, HP, and fire method.

Weapon  ← Pure abstract: virtual fire()
  ├── StandardModule  ← 1 bullet, dmg=26, green
  ├── SpreadModule    ← 3 bullets, dmg=26, orange
  └── PiercingModule  ← 1 wide bullet, dmg=55, hits=2, purple

Supporting (non-hierarchy):
  CollisionManager  ← AABB checks, removeDeadObjects()
  AudioManager      ← SFX (RAM) + BGM (streamed)
  ExplosionEffect   ← Object pool (30 slots × 10 particles)
  StarField         ← 200 stars across 3 parallax layers
```

---

### 🧩 OOP Concept Map

Every OOP concept this project teaches, with exactly where to find it in the code:

| # | OOP Concept | Where It Appears | Why It's Done This Way |
|---|---|---|---|
| 1 | **Abstract Class & Pure Virtual Functions** | `GameObject` declares `update()`, `draw()`, `onCollision()` as `= 0` | Forces every game object to implement its own behaviour; you cannot instantiate a `GameObject` directly |
| 2 | **Inheritance Chain** | `GameObject → Entity → Enemy → Boss → Cruiser` | Each level adds new data/behaviour without re-writing the level above it |
| 3 | **Polymorphism** | `boss->draw()` and `boss->handleBulletHit()` work for all 3 boss types through a single `Boss*` pointer | The caller doesn't need to know which boss it's dealing with — the vtable dispatches the right function |
| 4 | **Composition vs Inheritance** | `TwinCannons` **HAS-A** `Turret*` (not IS-A Turret) | A Turret is not a Boss; it's a *part* of a Boss. Composition models "has-a" relationships; inheritance models "is-a" |
| 5 | **Strategy Pattern** | `Weapon*` pointer in `Player`; swapped via `equipWeapon(new SpreadModule())` | Changing the weapon doesn't change `Player` code at all — only the strategy object changes |
| 6 | **Encapsulation** | `Player::takeDamageFromEnemy()` contains all hit logic (iFrames, shield check, life loss, multiplier reset) | Hit logic lives in one place; nothing outside `Player` needs to know the rules |
| 7 | **Object Pool Pattern** | `ExplosionEffect` — 30 pre-allocated slots, reused via `spawn()`/`spawnLarge()` | Avoids runtime `new`/`delete` every frame for particles; dramatically reduces heap fragmentation |
| 8 | **Finite State Machine** | `GameState` enum: `MENU → ARCADE/SURVIVAL → PAUSED → GAMEOVER/WIN` | Clean `switch` in `run()` loop; pausing just skips `updateGameplay()` — no object needs to be frozen |
| 9 | **Raw Pointer Arrays + Manual Memory** | `Enemy* enemies[100]`, explicit `new`/`delete`, no `std::vector` | Teaches real heap ownership; every allocated object must be deleted exactly once |
| 10 | **Virtual Destructor** | `~GameObject()`, `~Enemy()`, `~Boss()` all declared `virtual` | Without this, deleting a `Boss*` that holds a `Mothership` would only call `Boss`'s destructor, leaking `Mothership`'s resources |

---

### 💡 OOP Deep-Dive for Beginners

#### What is an Abstract Class?

An abstract class is a class that **cannot be instantiated** — it exists purely to define a
contract. In this project, `GameObject` is abstract:

```cpp
class GameObject {
public:
    virtual void update(float dt) = 0;   // "= 0" makes this PURE VIRTUAL
    virtual void draw(sf::RenderWindow& window) = 0;
    virtual void onCollision(GameObject* other) = 0;
    virtual ~GameObject() {}             // Virtual destructor — CRITICAL
};
```

> **Why?** You never want a "bare" `GameObject` in the game world — everything must be
> a Bullet, Enemy, Player, etc. The pure virtual functions **force** derived classes to provide
> their own implementations.

---

#### What is Inheritance?

Inheritance lets a class **reuse and extend** another class. Here's the chain from
`Entity` down to `Drone`:

```cpp
// Level 1: Entity adds physical properties to GameObject
class Entity : public GameObject {
public:
    float posX, posY;
    float velX, velY;
    int health, damage;
    float width, height;
    // update() and draw() still not implemented here for Enemy types
};

// Level 2: Enemy adds enemy-specific scaling
class Enemy : public Entity {
public:
    float speedScale;
    float fireScale;
    virtual void shoot() = 0;   // Still abstract
};

// Level 3: Drone finally provides concrete behaviour
class Drone : public Enemy {
public:
    void update(float dt) override { posY += 200.f * speedScale * dt; }
    void draw(sf::RenderWindow& w) override { /* draw sprite */ }
    void shoot() override { /* fire bullet downward */ }
    void onCollision(GameObject* o) override { health = 0; }
};
```

> **Why the chain?** `Drone`, `Viper`, and `Seeker` all share `posX`, `posY`, `health`,
> `speedScale`, etc. Writing those once in `Entity`/`Enemy` and inheriting them saves
> hundreds of duplicate lines.

---

#### What is Polymorphism?

Polymorphism means **one pointer type, many behaviours**. The game loop stores all
enemies in one array and calls the same function on every element:

```cpp
Enemy* enemies[100];
// enemies[0] might be a Drone, enemies[1] a Viper, enemies[2] a Seeker

for (int i = 0; i < enemyCount; i++) {
    enemies[i]->update(dt);   // Drone::update OR Viper::update OR Seeker::update
    enemies[i]->draw(window); // — the right one is called automatically
}
```

> **Why?** The game loop doesn't need `if (isDrone) ... else if (isViper) ...` 
> everywhere. The compiler's **vtable** figures out which function to call at runtime.

---

#### Composition vs Inheritance — The Twin Cannons Example

This is one of the most important design concepts. Ask yourself: **"Is a Turret a kind of Boss?"**
No — a Turret is a *part* of the Twin Cannons boss. That's **composition** ("has-a"):

```cpp
class Turret {             // Plain class — NOT in the inheritance tree
public:
    float posX, posY;
    int hp;                // Each turret has its own HP (350 each)
    sf::FloatRect getAABB() const;
    void fire(/* bullet array */);
};

class TwinCannons : public Boss {
private:
    Turret* leftTurret;    // COMPOSITION — TwinCannons OWNS a Turret
    Turret* rightTurret;   // (not inherits from it)
    int coreHP;            // Core is immune until BOTH turrets are dead

public:
    TwinCannons() {
        leftTurret  = new Turret();
        rightTurret = new Turret();
        coreHP = 350;
    }
    ~TwinCannons() {
        delete leftTurret;   // TwinCannons is responsible for cleanup
        delete rightTurret;
    }
    void handleBulletHit(Bullet* b) override {
        // Route bullet to correct turret based on X position
        if (b->posX < posX + width / 2)
            leftTurret->hp -= b->damage;
        else
            rightTurret->hp -= b->damage;
    }
};
```

---

#### The Strategy Pattern — Weapon System

The **Strategy Pattern** lets you swap an algorithm (the weapon) at runtime without
changing the class that uses it (the Player):

```cpp
class Weapon {                          // Abstract strategy
public:
    virtual void fire(float px, float py,
                      Bullet** bullets, int& count) = 0;
    virtual ~Weapon() {}
};

class SpreadModule : public Weapon {    // Concrete strategy A
public:
    void fire(float px, float py, Bullet** bullets, int& count) override {
        // Spawn 3 bullets: straight + left-angled + right-angled
    }
};

class PiercingModule : public Weapon {  // Concrete strategy B
public:
    void fire(float px, float py, Bullet** bullets, int& count) override {
        // Spawn 1 wide purple bullet with hitsLeft = 2
    }
};

class Player : public Entity {
private:
    Weapon* currentWeapon;              // Pointer to current strategy
public:
    void equipWeapon(Weapon* w) {
        delete currentWeapon;           // Clean up the old one
        currentWeapon = w;              // Swap in the new strategy
    }
    void shoot() {
        currentWeapon->fire(posX, posY, bullets, bulletCount);
        // Player::shoot() never changes — only the strategy does
    }
};
```

---

## 🎮 Gameplay Mechanics

### 🕹️ Player Movement & Controls

| Input | Action | Detail |
|---|---|---|
| ← → ↑ ↓ Arrow Keys | Move | Speed: 300 px/s, scaled by delta-time |
| Spacebar | Fire | Cooldown: 0.22s, delegates to `weapon->fire()` |
| E + Direction | Dash | Teleports 115px, grants brief invincibility, 3s cooldown |
| N | EMP Nuke | Destroys all regular enemies; deals 12% of boss max HP |
| ESC | Pause | Stops `updateGameplay()` — no object needs freezing |
| R (paused) | Resume | Resumes the game loop |
| Q (paused) | Quit to Menu | Returns to the main menu |

**Framerate Independence:** All velocities are multiplied by `dt` (delta time), so the game
runs at the same speed regardless of frame rate. Delta time is capped at `0.033s` to prevent
physics "tunnelling" (objects passing through each other) during lag spikes.

---

### ⚡ Dash System

```
E + Direction pressed
       │
       ▼
teleport ship 115px in movement direction
       │
       ▼
grant 0.8s invincibility frames (player blinks)
       │
       ▼
start 3-second cooldown
       │
HUD dash bar fills green ──────► ready again
```

The HUD bar shows a green fill when the dash is ready and an orange fill while cooling down.

---

### 🔢 Score Multiplier Chain

Kill an enemy within **3 seconds** of the previous kill to chain your multiplier:

```
×1  ──(kill within 3s)──►  ×2  ──(kill within 3s)──►  ×4
                                                         │
         ◄────────── (3s pass OR take damage) ──────────┘
                         reset to ×1
```

The active multiplier is always displayed on the HUD.

---

### 🛡️ Shield & Invincibility

- **Energy Shield:** Absorbs exactly **2 hits** before breaking with an explosion effect + sound.
  Picking up a second shield while one is active resets the counter to 2.
- **Invincibility Frames:** After any hit (shielded or not), the player is invincible for **0.8s**
  and visually blinks to indicate this. This also applies after a dash.
- **Life Loss:** Losing a life resets the weapon to `StandardModule` and triggers a fresh 0.8s iFrame window.

---

### 💣 EMP Screen Nuke

| Property | Value |
|---|---|
| Max held | 3 |
| Starting count | 1 |
| Drop chance from enemies | 5% |
| Effect on regular enemies | Instant kill |
| Effect on active boss | 12% of boss `maxHP` as damage |
| Camera shake | 18px amplitude, 0.5s duration |
| Screen flash | Full-screen white overlay |

---

## 🔫 Weapon System

All three weapons fire upward and are visually distinct. The weapon name always appears on the HUD.

| Weapon | Bullets | Damage | Pierce Hits | Colour | Notes |
|---|---|---|---|---|---|
| **Standard Module** | 1 (straight up) | 26 | 1 | 🟢 Green | Default weapon; given back on death |
| **Spread Module** | 3 (fan: left / centre / right) | 26 each | 1 | 🟠 Orange | Side bullets: velX = ±170, velY = −440 |
| **Piercing Module** | 1 (wide beam) | 55 | 2 (pierces 1 enemy) | 🟣 Purple | Second hit has full damage |

Drop rates from defeated enemies: **15% weapon drop** (randomly Spread or Pierce), **5% EMP drop**.

---

## 👾 Enemy Types

### Standard Enemies

| Enemy | Movement | Fires? | Appears In |
|---|---|---|---|
| **Drone** | Straight downward | ✅ Random intervals | All waves |
| **Viper** | Sinusoidal (oscillates left–right) | ✅ Downward | Wave 5+ (Survival) |
| **Seeker** | Locks player X at spawn, accelerates to 650 px/s | ❌ Kamikaze | Wave 8+ (Survival) |

### Survival Wave Scaling

| Property | Change per Wave |
|---|---|
| Enemy count | +2 per wave |
| Movement speed | +5% per wave |
| Fire rate | +10% per wave |
| Viper introduction | Wave 5 |
| Seeker introduction | Wave 8 |

---

## 💀 Boss Guide

Boss encounters pause all standard enemy spawning. Each boss has a multi-phase HP system;
behaviour intensifies with each phase transition.

---

### 🚀 Level 1 — THE CRUISER

```
HP: 900   Damage: 30   Size: 200×85 px
```

| Phase | Speed | Attack Interval | Special |
|---|---|---|---|
| Phase 1 (HP 900→600) | 250 px/s | 3.5s | 10 bullet columns, 1 random gap |
| Phase 2 (HP 600→300) | 310 px/s | 2.5s | Faster curtain |
| Phase 3 (HP 300→0) | 380 px/s | 1.8s | 2 staggered rows per burst |

**Attack Cycle:**
```
1. Warning phase  → thin 5px pulsing strips mark each column (including gap)
2. 1-second delay → position yourself in the ONE safe gap column
3. 2.5s barrage   → columns of bullets rain down; gap column is clear
```

> 💡 **Tip:** Watch the warning strips carefully. The gap is randomised each cycle.
> Hug the centre of the gap — the Cruiser moves faster in Phase 3.

**HP Bar:** Green (Phase 1) → Yellow (Phase 2) → Red (Phase 3)

---

### ⚙️ Level 2 — TWIN CANNONS

```
Left Turret HP: 350   Right Turret HP: 350   Core HP: 350
Core is fully immune until BOTH turrets are destroyed.
```

**The Composition Boss** — the turrets are separate objects owned by the boss via composition,
each with their own AABB hitbox, HP pool, and fire rate:

```
              ┌──────────────────────────┐
              │      TWIN CANNONS        │
              │  [L-Turret] [CORE] [R-Turret]  │
              │    hp=350   IMMUNE  hp=350   │
              └──────────────────────────┘

Bullet hits left of centre X  →  damages Left Turret
Bullet hits right of centre X →  damages Right Turret
One turret destroyed           →  survivor fires at 2× rate, takes 1.5× damage
Both turrets destroyed         →  Core opens, takes full damage
```

| Phase | Turret Fire Delay |
|---|---|
| Phase 1 | 1.1s per turret |
| Phase 2 | 0.75s per turret |
| Phase 3 | 0.5s per turret |
| Lone surviving turret | delay × 0.45 (≈2× faster) |

**HP Bar:** Cyan (combined turret HP) → switches to Green→Yellow→Red (core HP)

> 💡 **Tip:** Focus one turret completely before switching to the other. The survivor
> speeds up massively when alone, so don't let it live long.

---

### 🛸 Level 3 — THE MOTHERSHIP

```
HP: 900   Damage: 40   Size: 260×110 px
HP Bar: 620px wide, purple outline
```

The Mothership runs **three attack systems simultaneously:**

#### Attack System 1 — Primary Laser

```
Beam width: 60% of screen   (safe zones = edge strips on each side)

Warn  → thin 6px strip appears at beam position
       + green tint on safe edge zones (visual hint)
1 sec → position yourself in a safe zone!
7 sec → laser ACTIVE — damage pulses every 0.3s while overlapping
End   → Seeker Phase begins (5 seconds, 1 new Seeker every 1.4s)
```

The beam has a bright core strip (18% of beam width) and an animated shimmer. A large
**"LASER INCOMING"** warning text appears at screen centre during the warn phase.

#### Attack System 2 — Secondary Laser (Phase 2+)

```
Beam width: 50% of screen   (independent timing from primary)
After it ends: spawns +1 extra Seeker
```

#### Attack System 3 — Background Seeker Spawning

| Phase | Seeker Spawn Rate |
|---|---|
| Phase 1 | 1 Seeker every 9s |
| Phase 2 | 1 Seeker every 6.5s |
| Phase 3 | 1 Seeker every 4s |
| During Seeker Phase | +1 or +2 per 1.4s (Phase 3: 2 per spawn) |

> 💡 **Tip:** The safe zones are the strips to the **left and right** of the beam, hinted with
> a green tint. Dash there the moment you see the warning strip. EMPs are most valuable
> here — they wipe incoming Seekers and chunk the Mothership's HP.

---

## 🌠 Visual & Audio Systems

### StarField (Parallax Background)

```
Layer 1 (back)   → 67 stars, slow speed, dim, tiny
Layer 2 (middle) → 67 stars, medium speed, medium brightness
Layer 3 (front)  → 66 stars, fast speed, bright, large
Total            → 200 stars, wrap at bottom → creates depth illusion
```

### ExplosionEffect (Object Pool)

Instead of calling `new` and `delete` every time an enemy dies (which is slow and fragments
memory), an **object pool** pre-allocates 30 explosion slots at startup:

```cpp
// Pool of 30 explosions, each with 10 particles
// spawn()      → small enemies
// spawnLarge() → boss deaths
// Particles expand outward, then shrink+fade by ratio = (1 - life/maxLife)
// When a slot finishes, it becomes available again — no heap allocation
```

### AudioManager

| Resource Type | Loading Method | Examples |
|---|---|---|
| Sound Effects | Loaded to RAM (`SoundBuffer`) | Fire, explosion, hit, EMP, power-up |
| Background Music | Streamed from disk (`Music`) | Menu BGM, Gameplay BGM, Boss BGM |

Three distinct weapon sounds come from a single buffer played at different pitches:
`Standard = 1.0`, `Spread = 0.75`, `Piercing = 1.45`.

BGM swaps to the boss track on boss entry, and reverts when the boss is defeated.

### Screen Shake

| Event | Amplitude | Duration |
|---|---|---|
| Player hit | 9px | 0.3s |
| EMP activation | 18px | 0.5s |

Shake works by offsetting the SFML `View` by a random `(ox, oy)` vector that decays to zero
over the duration.

---

## ⚙️ Collision System

`CollisionManager` runs **AABB** (Axis-Aligned Bounding Box) rectangle overlap checks every
frame in this priority order:

```
1. Player bullets   vs  Enemies       → enemy.takeDamage() + bullet.consumeHit()
2. Player bullets   vs  Boss          → boss->handleBulletHit() (polymorphic routing)
3. Any bullet       vs  Asteroid      → bullet dies
4. Enemy bullets    vs  Player        → player->takeDamageFromEnemy() + bullet dies
5. Enemies          vs  Player        → takeDamageFromEnemy() + enemy dies
6. Boss             vs  Player        → takeDamageFromEnemy()
7. Asteroids        vs  Player        → takeDamageFromEnemy()
8. Player           vs  PowerUp       → equipWeapon() / activateShield() / addEMP()
```

After all checks, `removeDeadObjects()` compacts each pointer array:
surviving pointers shift to the front, dead objects are `delete`d, asteroids respawn at the top,
and power-ups may drop from dead enemies (15% weapon / 5% EMP).

---

## 🗺️ Game State Machine

The entire game flow is controlled by a single `GameState` enum. The `run()` loop switches on
this state every frame:

```
                   ┌─────────────┐
                   │    MENU     │◄────────────────────────────┐
                   └──────┬──────┘                             │
                          │ Select mode                        │ Q (quit)
              ┌───────────┴───────────┐                        │
              ▼                       ▼                        │
         ┌────────┐             ┌──────────┐                   │
         │ ARCADE │             │ SURVIVAL │                   │
         └───┬────┘             └────┬─────┘                   │
             │                      │                          │
             └──────────┬───────────┘                          │
                        │ ESC                                   │
                        ▼                                       │
                   ┌────────┐                                   │
                   │ PAUSED │───── R (resume) ──► back to game  │
                   └────────┘                                   │
                        │ Q                                     │
                        └───────────────────────────────────────┘
                        
    ARCADE + SURVIVAL can also transition to:
    ┌──────────┐      ┌─────┐
    │ GAME OVER│      │ WIN │   (Arcade only: defeat all 3 bosses)
    └──────────┘      └─────┘
```

> **Why a state machine?** Pausing is trivially implemented — the `PAUSED` state simply
> skips calling `updateGameplay()`. No enemy, bullet, or player object needs a "frozen" flag.

---

## 🎮 Controls Reference

| Input | Action |
|---|---|
| ← ↑ → ↓ Arrow Keys | Move ship (300 px/s) |
| Spacebar | Fire current weapon (0.22s cooldown) |
| E + Arrow Key | Dash in movement direction (115px, 3s cooldown) |
| N | Activate EMP Screen Nuke |
| ESC | Pause game |
| R | Resume (from pause screen) |
| Q | Quit to Main Menu (from pause screen) |

---

## 📁 Project Structure

```
SpaceInvaders/
│
├── src/
│   ├── main.cpp                  ← Entry point; creates Game object, calls run()
│   ├── Game.h / Game.cpp         ← Main loop, state machine, all system orchestration
│   │
│   ├── GameObject.h              ← Pure abstract base class
│   ├── Entity.h / Entity.cpp     ← Physical properties (pos, vel, health, size)
│   │
│   ├── Player.h / Player.cpp     ← Player logic, dash, shield, EMP, multiplier
│   ├── Bullet.h / Bullet.cpp     ← Projectile, owner tag, pierce counter
│   ├── PowerUp.h / PowerUp.cpp   ← Drop types, float behaviour
│   ├── Asteroid.h / Asteroid.cpp ← Indestructible, respawning hazard
│   │
│   ├── Weapon.h                  ← Abstract Weapon base (Strategy pattern)
│   ├── StandardModule.cpp        ← 1 bullet, green
│   ├── SpreadModule.cpp          ← 3-way spread, orange
│   ├── PiercingModule.cpp        ← Piercing beam, purple
│   │
│   ├── Enemy.h / Enemy.cpp       ← Abstract enemy base
│   ├── Drone.h / Drone.cpp       ← Straight fall, random fire
│   ├── Viper.h / Viper.cpp       ← Sinusoidal movement
│   ├── Seeker.h / Seeker.cpp     ← Kamikaze, X-lock on spawn
│   │
│   ├── Boss.h / Boss.cpp         ← Abstract boss base, HP bar, phase logic
│   ├── Cruiser.h / Cruiser.cpp   ← Level 1 boss
│   ├── Turret.h / Turret.cpp     ← Composed part of TwinCannons
│   ├── TwinCannons.h / .cpp      ← Level 2 boss (composition)
│   ├── Mothership.h / Mothership.cpp ← Level 3 boss
│   │
│   ├── CollisionManager.h / .cpp ← AABB checks, removeDeadObjects()
│   ├── AudioManager.h / .cpp     ← SFX + BGM management
│   ├── ExplosionEffect.h / .cpp  ← Particle pool (30 slots × 10 particles)
│   └── StarField.h / StarField.cpp ← 200-star parallax background
│
├── assets/
│   ├── textures/
│   │   ├── player.png            ← Player ship sprite
│   │   ├── drone.png             ← Drone enemy sprite
│   │   ├── viper.png             ← Viper enemy sprite
│   │   ├── seeker.png            ← Seeker enemy sprite
│   │   ├── cruiser.png           ← Cruiser boss sprite
│   │   ├── twincannons.png       ← Twin Cannons boss sprite
│   │   ├── turret.png            ← Turret sprite (used by TwinCannons)
│   │   ├── mothership.png        ← Mothership boss sprite
│   │   ├── bullet_player.png     ← Player bullet sprite
│   │   ├── bullet_enemy.png      ← Enemy bullet sprite
│   │   ├── asteroid.png          ← Asteroid sprite
│   │   └── powerup_sheet.png     ← Power-up icons (sprite sheet)
│   │
│   ├── audio/
│   │   ├── bgm_menu.ogg          ← Menu background music (streamed)
│   │   ├── bgm_gameplay.ogg      ← Gameplay BGM (streamed)
│   │   ├── bgm_boss.ogg          ← Boss encounter BGM (streamed)
│   │   ├── sfx_fire_standard.wav ← Standard weapon fire
│   │   ├── sfx_fire_spread.wav   ← (or pitch-shifted from standard)
│   │   ├── sfx_fire_pierce.wav   ← (or pitch-shifted from standard)
│   │   ├── sfx_explosion.wav     ← Enemy/boss explosion
│   │   ├── sfx_player_hit.wav    ← Player takes damage
│   │   ├── sfx_shield_break.wav  ← Energy Shield breaking
│   │   ├── sfx_powerup.wav       ← Collecting a power-up
│   │   └── sfx_emp.wav           ← EMP activation
│   │
│   └── fonts/
│       └── arcade.ttf            ← Retro font for HUD and menus
│
├── README.md                     ← This file
└── Makefile / CMakeLists.txt     ← Build configuration
```

> **Missing asset fallback:** `loadTextureClean()` removes white backgrounds (sets pixels
> with r, g, b > 230 to alpha = 0). If a texture file is missing entirely, a coloured geometric
> fallback shape is drawn instead — the game will still run.

---

## 🔨 Build Instructions

### Prerequisites

Install SFML 2.5.1 for your platform:

- **Windows:** Download from https://www.sfml-dev.org/download.php
- **Linux:** `sudo apt-get install libsfml-dev`
- **macOS:** `brew install sfml`

---

### Building with g++ (Linux / macOS)

```bash
# Clone the project
git clone https://github.com/your-username/space-invaders-galactic-defense.git
cd space-invaders-galactic-defense

# Compile all source files
g++ -std=c++17 src/*.cpp \
    -I/usr/include/SFML \
    -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio \
    -o SpaceInvaders

# Run
./SpaceInvaders
```

---

### Building with g++ (Windows — MinGW)

```bat
g++ -std=c++17 src\*.cpp ^
    -I"C:\SFML-2.5.1\include" ^
    -L"C:\SFML-2.5.1\lib" ^
    -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio ^
    -o SpaceInvaders.exe

SpaceInvaders.exe
```

---

### Building with Visual Studio (Windows)

1. Create a new **Empty C++ Project** in Visual Studio 2019/2022.
2. Add all `.cpp` files from the `src/` folder to the project.
3. Open **Project → Properties → VC++ Directories:**
   - **Include Directories:** Add `C:\SFML-2.5.1\include`
   - **Library Directories:** Add `C:\SFML-2.5.1\lib`
4. Open **Linker → Input → Additional Dependencies:** Add:
   ```
   sfml-graphics.lib
   sfml-window.lib
   sfml-system.lib
   sfml-audio.lib
   ```
5. Copy the SFML `.dll` files from `C:\SFML-2.5.1\bin` into your project's output directory.
6. Build and run (`F5`).

---

## 📦 Asset List & Fallback Behaviour

All assets live in the `assets/` folder relative to the executable. The game includes graceful
fallbacks so it runs even without external files — useful during early development:

| Asset File | Used By | If Missing |
|---|---|---|
| `textures/player.png` | Player ship | White rectangle drawn |
| `textures/drone.png` | Drone enemy | Coloured rectangle drawn |
| `textures/viper.png` | Viper enemy | Coloured rectangle drawn |
| `textures/seeker.png` | Seeker enemy | Coloured triangle drawn |
| `textures/cruiser.png` | Cruiser boss | Large grey rectangle drawn |
| `textures/twincannons.png` | Twin Cannons boss | Large cyan rectangle drawn |
| `textures/mothership.png` | Mothership boss | Large purple rectangle drawn |
| `audio/bgm_gameplay.ogg` | In-game music | Silent (no crash) |
| `audio/bgm_boss.ogg` | Boss music | Falls back to gameplay BGM |
| `audio/sfx_*.wav` | Sound effects | Silent (no crash) |
| `fonts/arcade.ttf` | All HUD text | SFML default font used |

> ℹ️ `loadTextureClean()` automatically removes white backgrounds from sprites by setting
> any pixel where r > 230, g > 230, and b > 230 to alpha = 0. Always store sprites on
> a white background for this to work correctly.

---

## 🎓 Academic Information

| Field | Detail |
|---|---|
| **Course** | Object Oriented Programming (OOP) |
| **Project** | Space Invaders: Galactic Defense |
| **Institution** | FAST-NU Lahore, Department of Computer Science |
| **Section** | BCS 2D |
| **Semester** | Spring 2026 |
| **Instructor** | Usama Hassan |
| **Teaching Assistant** | Abdul Moiz Khan |
| **Total Marks** | 150 |

### Grading Rubric

| Category | Marks |
|---|---|
| Graphics & Visual Polish | 20 |
| Audio & Sound Effects | 10 |
| Core OOP Architecture & Memory Management | 50 |
| Core Gameplay & Game Modes | 40 |
| Advanced Mechanics & Power-Ups | 30 |
| **Total** | **150** |

---

## 📜 License

This project is licensed under the **MIT License** — see `LICENSE` for details.

---

<div align="center">



</div>
