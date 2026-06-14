// ============================================================================
//  SPACE INVADERS : Object-Oriented Galactic Defense
//  OOP Semester Project  |  BCS 2D  |  FAST-NU Lahore
// ============================================================================
//
//  CONTROLS:
//    Arrow Keys    = Move             Space = Fire
//    E + Direction = Dash (0.5s inv)  N     = EMP
//    ESC           = Pause            R     = Resume (paused)
//    Q             = Quit to Menu     1/2   = Mode Select (menu)
//
//  ASSET FILES (place next to .exe):
//    Sprites : player.png  drone.png  enemy.png  seeker.png
//              asteroid.png  boss1.png  boss2.png  boss3.png  explosion.png
//    Audio   : player_fire.wav  enemy_fire.wav  player_hit.wav
//              enemy_explosion.wav  boss_explosion.wav  powerup_pickup.wav
//              emp.wav  shield_break.wav  gameplay_bgm.ogg  boss_bgm.ogg
//    Font    : arial.ttf
//
//  PATCH LOG:
//    BF-1  Dash invincibility = 0.5s; damage invincibility = 0.8s (separate)
//    BF-2  PiercingModule hitsLeft = 2
//    BF-3  Shield-absorbed hits reset multiplier to 1x
//    BF-4  Survival waveFireScale +10%/wave independent of waveSpeedScale +5%
//    BF-5  Each weapon fires a distinct sound
//    BF-6  Player::health kept in sync with lives
//    BF-7  TwinCannons AABB turret collision routing
//    MF-1  Weapon-state visual indicator on player ship
//    MF-2  Shield-break spawns cyan explosion burst
//    MF-3  Screen shake on player damage
//    PNG   Transparency cleanup for all sprite textures
//    NEW   EMP screen shake improvements
//    NEW   Piercing bullet visual
//    NEW   Updated ship/enemy sizes
//    NEW   Boss reworks (Cruiser buffed, TwinCannons, Mothership vertical laser)
//    FIX   RapidModule removed — Standard/Spread/Piercing only
//    FIX   Life loss resets weapon to Standard, does NOT clear world powerups
//    FIX   Enemy sizes increased
//    FIX   Cruiser buffed HP/fire rate, dramatic entry, clean death
//    FIX   Arcade progression is score-based
// ============================================================================

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using namespace sf;
using namespace std;

// ============================================================================
//  CONSTANTS
// ============================================================================
const float WINDOW_W = 1000.0f;
const float WINDOW_H = 800.0f;
const float PLAYER_SPD = 300.0f;
const float BULLET_PLAYER_SPD = 640.0f;
const float BULLET_ENEMY_SPD = 260.0f;
const float DRONE_BASE_SPD = 100.0f;
const float VIPER_BASE_SPD = 90.0f;
const float SEEKER_ACCEL = 160.0f;
const int   MAX_BULLETS = 200;
const int   MAX_ENEMIES = 60;
const int   MAX_ASTEROIDS = 8;
const int   MIN_ASTEROIDS = 3;
const int   FIELD_ASTEROIDS = 5;
const int   MAX_POWERUPS = 20;
const int   MAX_EXPLOSIONS = 30;

// Score thresholds to trigger each arcade boss
const int ARCADE_BOSS1_SCORE = 1500;   // Level 1 boss after ~1500 pts
const int ARCADE_BOSS2_SCORE = 4000;   // Level 2 boss after ~4000 pts
const int ARCADE_BOSS3_SCORE = 7500;   // Level 3 boss after ~7500 pts

// ============================================================================
//  GAME STATE
// ============================================================================
enum GameState {
    STATE_MENU, STATE_ARCADE, STATE_SURVIVAL,
    STATE_PAUSED, STATE_GAMEOVER, STATE_WIN
};

// ============================================================================
//  TEXTURE HELPER — load + transparent background cleanup
// ============================================================================
static bool loadTextureClean(Texture& tex, const char* file) {
    Image img;
    if (!img.loadFromFile(file)) return false;
    Vector2u sz = img.getSize();
    for (unsigned y = 0; y < sz.y; y++) {
        for (unsigned x = 0; x < sz.x; x++) {
            Color c = img.getPixel(x, y);
            if (c.r > 230 && c.g > 230 && c.b > 230)
                img.setPixel(x, y, Color(c.r, c.g, c.b, 0));
        }
    }
    if (!tex.loadFromImage(img)) return false;
    tex.setSmooth(true);
    return true;
}

// ============================================================================
//  AUDIO MANAGER
// ============================================================================
class AudioManager {
    SoundBuffer bFire;
    Sound       sStd, sSpread, sPierce;
    bool        fireOk;

    SoundBuffer bEnemyFire, bPlayerHit, bEnemyExp;
    SoundBuffer bBossExp, bPowerup, bEMP, bShield;
    Sound       sEnemyFire, sPlayerHit, sEnemyExp;
    Sound       sBossExp, sPowerup, sEMP, sShield;
    bool        sfxOk[7];

    Music mGameplay, mBoss;
    bool  musicOk[2];
    bool  bossPlaying;

    void tryLoad(SoundBuffer& buf, Sound& snd, const char* file, int idx) {
        sfxOk[idx] = buf.loadFromFile(file);
        if (sfxOk[idx]) snd.setBuffer(buf);
    }
    void play(Sound& snd, bool ok) { if (ok) { snd.stop(); snd.play(); } }

public:
    AudioManager() : fireOk(false), bossPlaying(false) {
        for (int i = 0; i < 7; i++) sfxOk[i] = false;
        musicOk[0] = musicOk[1] = false;

        fireOk = bFire.loadFromFile("player_fire.wav");
        if (fireOk) {
            sStd.setBuffer(bFire);    sStd.setPitch(1.00f);
            sSpread.setBuffer(bFire); sSpread.setPitch(0.75f);
            sPierce.setBuffer(bFire); sPierce.setPitch(1.45f);
        }

        tryLoad(bEnemyFire, sEnemyFire, "enemy_fire.wav", 0);
        tryLoad(bPlayerHit, sPlayerHit, "player_hit.wav", 1);
        tryLoad(bEnemyExp, sEnemyExp, "enemy_explosion.wav", 2);
        tryLoad(bBossExp, sBossExp, "boss_explosion.wav", 3);
        tryLoad(bPowerup, sPowerup, "powerup_pickup.wav", 4);
        tryLoad(bEMP, sEMP, "emp.wav", 5);
        tryLoad(bShield, sShield, "shield_break.wav", 6);

        musicOk[0] = mGameplay.openFromFile("gameplay_bgm.ogg");
        musicOk[1] = mBoss.openFromFile("boss_bgm.ogg");
        if (musicOk[0]) { mGameplay.setLoop(true); mGameplay.setVolume(55); }
        if (musicOk[1]) { mBoss.setLoop(true);     mBoss.setVolume(65); }
    }

    void playGameplayMusic() {
        if (bossPlaying && musicOk[1]) mBoss.stop();
        bossPlaying = false;
        if (musicOk[0] && mGameplay.getStatus() != Music::Playing) mGameplay.play();
    }
    void playBossMusic() {
        if (bossPlaying) return;
        if (musicOk[0]) mGameplay.stop();
        bossPlaying = true;
        if (musicOk[1]) mBoss.play();
    }
    void stopAll() {
        if (musicOk[0]) mGameplay.stop();
        if (musicOk[1]) mBoss.stop();
        bossPlaying = false;
    }
    // Dramatic boss entry: stop gameplay music, play boss music with delay feel
    void playBossEntry() {
        if (musicOk[0]) mGameplay.stop();
        bossPlaying = true;
        if (musicOk[1]) { mBoss.stop(); mBoss.play(); }
    }

    void playStdFire() { play(sStd, fireOk); }
    void playSpreadFire() { play(sSpread, fireOk); }
    void playPierceFire() { play(sPierce, fireOk); }
    void playEnemyFire() { play(sEnemyFire, sfxOk[0]); }
    void playPlayerHit() { play(sPlayerHit, sfxOk[1]); }
    void playEnemyExp() { play(sEnemyExp, sfxOk[2]); }
    void playBossExp() { play(sBossExp, sfxOk[3]); }
    void playPowerup() { play(sPowerup, sfxOk[4]); }
    void playEMP() { play(sEMP, sfxOk[5]); }
    void playShieldBreak() { play(sShield, sfxOk[6]); }
};

AudioManager* gAudio = nullptr;

// ============================================================================
//  SCROLLING STARFIELD  (3-layer parallax)
// ============================================================================
class StarField {
    struct Star { float x, y, spd; Uint8 br; float r; };
    static const int N = 200;
    Star s[N];
public:
    StarField() {
        for (int i = 0; i < N; i++) {
            s[i].x = (float)(rand() % (int)WINDOW_W);
            s[i].y = (float)(rand() % (int)WINDOW_H);
            int lay = i % 3;
            if (lay == 0) { s[i].spd = 22.0f;  s[i].br = 55;  s[i].r = 1.0f; }
            else if (lay == 1) { s[i].spd = 55.0f;  s[i].br = 140; s[i].r = 1.5f; }
            else { s[i].spd = 95.0f;  s[i].br = 230; s[i].r = 2.0f; }
        }
    }
    void update(float dt) {
        for (int i = 0; i < N; i++) {
            s[i].y += s[i].spd * dt;
            if (s[i].y > WINDOW_H + 4.0f) {
                s[i].y = -4.0f;
                s[i].x = (float)(rand() % (int)WINDOW_W);
            }
        }
    }
    void draw(RenderWindow& win) {
        for (int i = 0; i < N; i++) {
            CircleShape c(s[i].r);
            c.setFillColor(Color(s[i].br, s[i].br, s[i].br));
            c.setPosition(s[i].x - s[i].r, s[i].y - s[i].r);
            win.draw(c);
        }
    }
};

// ============================================================================
//  EXPLOSION PARTICLE EFFECT
// ============================================================================
class ExplosionEffect {
    struct Particle {
        float x, y, vx, vy;
        float life, maxLife, sz;
        Color color;
        bool  active;
    };
    static const int PPC = 10;
    Particle pool[MAX_EXPLOSIONS][PPC];
    bool     used[MAX_EXPLOSIONS];
    Texture  expTex;
    bool     expTexOk;

public:
    ExplosionEffect() : expTexOk(false) {
        expTexOk = expTex.loadFromFile("explosion.png");
        if (expTexOk) expTex.setSmooth(true);
        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            used[i] = false;
            for (int j = 0; j < PPC; j++) pool[i][j].active = false;
        }
    }

    void spawn(float cx, float cy, Color col) {
        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            if (used[i]) continue;
            used[i] = true;
            for (int j = 0; j < PPC; j++) {
                float ang = (j / (float)PPC) * 6.2832f;
                float spd = 50.0f + (float)(rand() % 140);
                pool[i][j] = { cx, cy,
                               cosf(ang) * spd, sinf(ang) * spd,
                               0.0f,
                               0.28f + (float)(rand() % 32) / 100.0f,
                               3.0f + (float)(rand() % 7),
                               col, true };
            }
            return;
        }
    }

    // Large centered explosion — for boss clean death
    void spawnLarge(float cx, float cy, Color col) {
        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            if (used[i]) continue;
            used[i] = true;
            for (int j = 0; j < PPC; j++) {
                float ang = (j / (float)PPC) * 6.2832f;
                float spd = 120.0f + (float)(rand() % 200);
                pool[i][j] = { cx, cy,
                               cosf(ang) * spd, sinf(ang) * spd,
                               0.0f,
                               0.55f + (float)(rand() % 30) / 100.0f,
                               8.0f + (float)(rand() % 10),
                               col, true };
            }
            return;
        }
    }

    void update(float dt) {
        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            if (!used[i]) continue;
            bool alive = false;
            for (int j = 0; j < PPC; j++) {
                if (!pool[i][j].active) continue;
                pool[i][j].x += pool[i][j].vx * dt;
                pool[i][j].y += pool[i][j].vy * dt;
                pool[i][j].life += dt;
                if (pool[i][j].life >= pool[i][j].maxLife) pool[i][j].active = false;
                else alive = true;
            }
            if (!alive) used[i] = false;
        }
    }

    void draw(RenderWindow& win) {
        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            if (!used[i]) continue;
            for (int j = 0; j < PPC; j++) {
                if (!pool[i][j].active) continue;
                float ratio = 1.0f - pool[i][j].life / pool[i][j].maxLife;
                float sz = pool[i][j].sz * ratio;
                if (sz < 0.3f) continue;
                Color c = pool[i][j].color;
                c.a = (Uint8)(ratio * 220);
                if (expTexOk) {
                    Sprite sp(expTex);
                    float sc = sz * 2.0f / (float)expTex.getSize().x;
                    sp.setScale(sc, sc);
                    sp.setColor(c);
                    sp.setPosition(pool[i][j].x - sz, pool[i][j].y - sz);
                    win.draw(sp, BlendAlpha);
                }
                else {
                    CircleShape cs(sz);
                    cs.setFillColor(c);
                    cs.setPosition(pool[i][j].x - sz, pool[i][j].y - sz);
                    win.draw(cs);
                }
            }
        }
    }
};

// ============================================================================
//  ABSTRACT BASE CLASSES
// ============================================================================
class GameObject {
public:
    virtual void update(float dt) = 0;
    virtual void draw(RenderWindow& window) = 0;
    virtual void onCollision(GameObject* other) = 0;
    virtual ~GameObject() {}
};

class Entity : public GameObject {
protected:
    float posX, posY;
    float velX, velY;
    float health, damage;
    float height, width;

public:
    Entity(float px, float py, float vx, float vy,
        float h, float d, float ht, float wd)
        : posX(px), posY(py), velX(vx), velY(vy),
        health(h), damage(d), height(ht), width(wd) {
    }

    float getPosX()   const { return posX; }
    float getPosY()   const { return posY; }
    float getWidth()  const { return width; }
    float getHeight() const { return height; }
    float getHealth() const { return health; }
    float getDamage() const { return damage; }

    void setVelocity(float vx, float vy) { velX = vx; velY = vy; }
    void setPosition(float px, float py) { posX = px; posY = py; }
    void setHealth(float h) { health = h; }
    void takeDamage(float d) { health -= d; if (health < 0) health = 0; }

    virtual bool isPlayer() { return false; }
    virtual bool isEnemy() { return false; }
    virtual bool isBoss() { return false; }
    virtual bool isPlayerBullet() { return false; }
    virtual bool isEnemyBullet() { return false; }
    virtual bool isAsteroid() { return false; }
    virtual bool isPowerUp() { return false; }

    virtual ~Entity() {}
};

// ============================================================================
//  BULLET
// ============================================================================
class Bullet : public Entity {
    float  bulletDmg;
    int    hitsLeft;
    string owner;
    RectangleShape shape;
    float  lifeTimer, maxLife;

public:
    Bullet(float px, float py, float dmg, int hits, const string& own)
        : Entity(px, py, 0,
            own == "Player" ? -BULLET_PLAYER_SPD : BULLET_ENEMY_SPD,
            10.0f, dmg,
            own == "Player" ? 22.0f : 16.0f,
            own == "Player" ? 4.0f : 4.0f),
        bulletDmg(dmg), hitsLeft(hits), owner(own),
        lifeTimer(0.0f), maxLife(0.0f) {
        shape.setSize(Vector2f(width, height));
        shape.setFillColor(own == "Player" ? Color(60, 255, 140) : Color(255, 60, 60));
    }

    bool  isPlayerBullet() override { return owner == "Player"; }
    bool  isEnemyBullet()  override { return owner == "Enemy"; }
    float getBulletDmg()   const { return bulletDmg; }

    void makeWide(float w, float h, float lt, Color col) {
        width = w; height = h;
        shape.setSize(Vector2f(w, h));
        shape.setFillColor(col);
        maxLife = lt;
    }
    void consumeHit() { hitsLeft--; if (hitsLeft <= 0) health = 0.0f; }

    void update(float dt) override {
        posX += velX * dt; posY += velY * dt;
        if (maxLife > 0.0f) { lifeTimer += dt; if (lifeTimer >= maxLife) health = 0.0f; }
        if (posY<-100 || posY>WINDOW_H + 100 || posX<-300 || posX>WINDOW_W + 300)
            health = 0.0f;
    }
    void draw(RenderWindow& win) override { shape.setPosition(posX, posY); win.draw(shape); }
    void onCollision(GameObject*) override {}
};

// ============================================================================
//  WEAPON SYSTEM  (Polymorphism) — Standard, Spread, Piercing only
// ============================================================================
class Weapon {
protected:
    float  damage;
    string name;
public:
    Weapon(float d, const string& n) : damage(d), name(n) {}
    string getName() const { return name; }
    virtual void fire(float px, float py, float pw,
        Bullet** bullets, int& cnt, int maxB) = 0;
    virtual ~Weapon() {}
};

class StandardModule : public Weapon {
public:
    StandardModule() : Weapon(26.0f, "Standard") {}
    void fire(float px, float py, float pw,
        Bullet** b, int& cnt, int maxB) override {
        if (cnt < maxB) {
            b[cnt++] = new Bullet(px + pw / 2.0f - 2.0f, py - 14.0f, damage, 1, "Player");
            if (gAudio) gAudio->playStdFire();
        }
    }
};

class SpreadModule : public Weapon {
public:
    SpreadModule() : Weapon(26.0f, "Spread") {}   // full damage per bullet
    void fire(float px, float py, float pw,
        Bullet** b, int& cnt, int maxB) override {
        if (cnt + 3 > maxB) return;
        float cx = px + pw / 2.0f;
        b[cnt++] = new Bullet(cx - 2.0f, py - 14.0f, damage, 1, "Player");
        Bullet* lb = new Bullet(cx - 14.0f, py - 8.0f, damage, 1, "Player");
        lb->setVelocity(-170.0f, -440.0f);
        b[cnt++] = lb;
        Bullet* rb = new Bullet(cx + 10.0f, py - 8.0f, damage, 1, "Player");
        rb->setVelocity(170.0f, -440.0f);
        b[cnt++] = rb;
        if (gAudio) gAudio->playSpreadFire();
    }
};

// BF-2: hitsLeft=2 — passes first enemy, strikes second
class PiercingModule : public Weapon {
public:
    PiercingModule() : Weapon(55.0f, "Piercing") {}
    void fire(float px, float py, float pw,
        Bullet** b, int& cnt, int maxB) override {
        if (cnt < maxB) {
            Bullet* bl = new Bullet(px + pw / 2.0f - 4.0f, py - 14.0f, damage, 2, "Player");
            bl->makeWide(8.0f, 22.0f, 0.0f, Color(180, 0, 255));
            b[cnt++] = bl;
            if (gAudio) gAudio->playPierceFire();
        }
    }
};

// ============================================================================
//  POWERUP  (types: 0=Spread, 1=Piercing, 2=Shield, 3=EMP)
// ============================================================================
class PowerUp : public Entity {
    int   type;
    CircleShape shape;
    Clock pulseClock;
    float rotAngle;
public:
    PowerUp(float px, float py, int t)
        : Entity(px, py, 0.0f, 55.0f, 10.0f, 0.0f, 36.0f, 36.0f),
        type(t), rotAngle(0.0f) {
        shape.setRadius(18.0f);
        shape.setOutlineThickness(4.0f);
        switch (t) {
        case 0: shape.setFillColor(Color(255, 130, 0));  shape.setOutlineColor(Color::Yellow); break;
        case 1: shape.setFillColor(Color(160, 0, 255));  shape.setOutlineColor(Color::Cyan);   break;
        case 2: shape.setFillColor(Color(0, 200, 255));  shape.setOutlineColor(Color::White);  break;
        case 3: shape.setFillColor(Color(255, 220, 0));  shape.setOutlineColor(Color::White);  break;
        default:shape.setFillColor(Color(180, 180, 180)); shape.setOutlineColor(Color::White);  break;
        }
    }
    int  getType()   const { return type; }
    bool isPowerUp() override { return true; }

    void update(float dt) override {
        posY += velY * dt;
        rotAngle += 90.0f * dt;
        if (posY > WINDOW_H + 30) health = 0;
    }

    void draw(RenderWindow& win) override {
        float t = pulseClock.getElapsedTime().asSeconds();
        float p = sinf(t * 5.0f) * 0.5f + 0.5f;
        float gr = 14.0f + p * 14.0f;

        CircleShape glow(gr);
        Color gc = shape.getFillColor(); gc.a = 80;
        glow.setFillColor(gc);
        glow.setPosition(posX + 18.0f - gr, posY + 18.0f - gr);
        win.draw(glow);

        shape.setPosition(posX, posY);
        win.draw(shape);

        // Rotating triangle indicator
        float cx = posX + 18.0f, cy = posY + 18.0f;
        float rad = 26.0f;
        float baseAngle = rotAngle * 3.14159f / 180.0f;
        Color lc = shape.getOutlineColor();
        for (int i = 0; i < 3; i++) {
            float ang = baseAngle + i * 2.0944f;
            float x1 = cx + cosf(ang) * rad, y1 = cy + sinf(ang) * rad;
            float x2 = cx + cosf(ang + 0.4f) * (rad - 8), y2 = cy + sinf(ang + 0.4f) * (rad - 8);
            Vertex line[] = { Vertex(Vector2f(x1,y1),lc), Vertex(Vector2f(x2,y2),lc) };
            win.draw(line, 2, Lines);
        }
    }
    void onCollision(GameObject*) override { health = 0.0f; }
};

// ============================================================================
//  ASTEROID
// ============================================================================
class Asteroid : public Entity {
    Texture     tex;
    Sprite      spr;
    bool        texOk;
    CircleShape fallback;
    float       radius;
public:
    Asteroid(float px, float py)
        : Entity(px, py, 0.0f, 0.0f, 999.0f, 1.0f, 0.0f, 0.0f), texOk(false) {
        float sz = 1.0f + (float)(rand() % 3);
        radius = 18.0f * sz;
        width = height = radius * 2.0f;
        velY = 28.0f + (float)(rand() % 45);
        texOk = loadTextureClean(tex, "asteroid.png");
        if (texOk) {
            spr.setTexture(tex);
            spr.setScale(width / tex.getSize().x, height / tex.getSize().y);
        }
        else {
            fallback.setRadius(radius);
            fallback.setFillColor(Color(110, 95, 80));
            fallback.setOutlineColor(Color(170, 150, 120));
            fallback.setOutlineThickness(2.0f);
        }
    }
    bool isAsteroid() override { return true; }
    void respawn() {
        posX = 30.0f + (float)(rand() % (int)(WINDOW_W - 80));
        posY = -radius * 2.0f - (float)(rand() % 60);
        velY = 28.0f + (float)(rand() % 45);
        health = 999.0f;
    }
    void update(float dt) override { posY += velY * dt; if (posY > WINDOW_H + 100) health = 0.0f; }
    void draw(RenderWindow& win) override {
        if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
        else { fallback.setPosition(posX, posY); win.draw(fallback); }
    }
    void onCollision(GameObject*) override {}
};

// ============================================================================
//  PLAYER
// ============================================================================
class Player : public Entity {
    int     lives;
    int     score;
    int     multiplier;
    float   multiTimer;
    int     shieldHits;
    Weapon* weapon;
    int     empCount;

    Clock   invincClock;
    float   invincDuration;
    bool    invincible;

    Clock   flashClock;
    bool    hitFlash;
    bool    shieldBrokeFlag;
    bool    tookDamageFlag;

    Clock   dashClock;

    Texture tex; Sprite spr; bool texOk;
    Font& font;

public:
    Player(int lv, float px, float py, Font& f)
        : Entity(px, py, 0, 0, 100.0f, 0.0f, 62.0f, 62.0f),
        lives(lv), score(0), multiplier(1), multiTimer(99.0f),
        shieldHits(0), weapon(new StandardModule()),
        empCount(1), invincDuration(0.8f), invincible(false),
        hitFlash(false), shieldBrokeFlag(false), tookDamageFlag(false), font(f) {
        texOk = loadTextureClean(tex, "player.png");
        if (texOk) { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y); }
    }
    ~Player() { delete weapon; }

    int    getLives()      const { return lives; }
    int    getScore()      const { return score; }
    int    getMultiplier() const { return multiplier; }
    int    getEMPCount()   const { return empCount; }
    int    getShieldHits() const { return shieldHits; }
    string getWeaponName() const { return weapon ? weapon->getName() : "None"; }
    bool   isAlive()       const { return lives > 0; }
    bool   isInvincible()  const { return invincible; }
    bool   isPlayer()      override { return true; }

    bool popShieldBroke() { bool b = shieldBrokeFlag; shieldBrokeFlag = false; return b; }
    bool popTookDamage() { bool b = tookDamageFlag;  tookDamageFlag = false;  return b; }

    float getDashCooldown() const {
        float e = dashClock.getElapsedTime().asSeconds();
        return (e < 3.0f) ? (3.0f - e) : 0.0f;
    }

    void addScore(int pts) { score += pts * multiplier; }
    void registerKill() {
        if (multiTimer < 3.0f) {
            if (multiplier == 1) multiplier = 2;
            else if (multiplier == 2) multiplier = 4;
        }
        multiTimer = 0.0f;
    }

    void equipWeapon(Weapon* w) { delete weapon; weapon = w; }
    void activateShield() { shieldHits = 2; }
    void addEMP() { if (empCount < 3) empCount++; }
    bool hasEMP()           const { return empCount > 0; }
    void consumeEMP() { if (empCount > 0) empCount--; }

    void fireWeapon(Bullet** b, int& cnt, int maxB) {
        if (weapon) weapon->fire(posX, posY, width, b, cnt, maxB);
    }

    void dash(float dx, float dy) {
        if (dashClock.getElapsedTime().asSeconds() < 3.0f) return;
        posX += dx * 115.0f; posY += dy * 115.0f;
        posX = max(0.0f, min(posX, WINDOW_W - width));
        posY = max(0.0f, min(posY, WINDOW_H - height));
        dashClock.restart();
        invincDuration = 0.5f;   // BF-1
        invincible = true;
        invincClock.restart();
    }

    // BF-3: multiplier resets on any damage. Life loss resets weapon to Standard.
    // Shield hits do NOT reset weapon. World powerups are NOT touched here.
    void takeDamageFromEnemy() {
        if (invincible) return;

        multiplier = 1;
        multiTimer = 99.0f;
        tookDamageFlag = true;

        if (shieldHits > 0) {
            shieldHits--;
            if (shieldHits == 0) {
                shieldBrokeFlag = true;
                if (gAudio) gAudio->playShieldBreak();
            }
            // Shield absorbed — weapon stays, world powerups untouched
        }
        else {
            lives--;
            hitFlash = true;
            flashClock.restart();
            // FIX: reset equipped weapon to Standard on life loss
            delete weapon;
            weapon = new StandardModule();
            if (gAudio) gAudio->playPlayerHit();
        }

        invincDuration = 0.8f;   // BF-1
        invincible = true;
        invincClock.restart();

        health = (float)(lives * 34);  // BF-6
        if (health < 0) health = 0;
    }

    void update(float dt) override {
        posX += velX * dt; posY += velY * dt;
        posX = max(0.0f, min(posX, WINDOW_W - width));
        posY = max(0.0f, min(posY, WINDOW_H - height));
        if (invincible && invincClock.getElapsedTime().asSeconds() >= invincDuration)
            invincible = false;
        if (hitFlash && flashClock.getElapsedTime().asSeconds() >= 0.12f)
            hitFlash = false;
        multiTimer += dt;
        if (multiTimer >= 3.0f && multiplier > 1) multiplier = 1;
    }

    void draw(RenderWindow& win) override {
        if (shieldHits > 0) {
            CircleShape sg(38.0f);
            sg.setFillColor(Color(0, 160, 255, 45));
            sg.setOutlineColor(Color(0, 220, 255, 180));
            sg.setOutlineThickness(3.0f);
            sg.setPosition(posX + width / 2.0f - 38.0f, posY + height / 2.0f - 38.0f);
            win.draw(sg);
        }
        if (hitFlash) {
            RectangleShape fl(Vector2f(width, height));
            fl.setFillColor(Color(255, 255, 255, 160));
            fl.setPosition(posX, posY); win.draw(fl);
        }
        bool visible = true;
        if (invincible) visible = ((int)(invincClock.getElapsedTime().asSeconds() * 9)) % 2 == 0;

        if (visible) {
            if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
            else {
                RectangleShape r(Vector2f(width, height));
                r.setFillColor(Color(80, 160, 255));
                r.setPosition(posX, posY); win.draw(r);
            }

            // MF-1: weapon-state visual indicator
            if (weapon) {
                Color tipCol;
                string wn = weapon->getName();
                if (wn == "Standard") tipCol = Color(0, 255, 120);
                else if (wn == "Spread")   tipCol = Color(255, 140, 0);
                else                     tipCol = Color(180, 0, 255);

                CircleShape tip(5.0f);
                tip.setFillColor(tipCol);
                tip.setOutlineColor(Color(255, 255, 255, 110));
                tip.setOutlineThickness(1.5f);
                tip.setPosition(posX + width / 2.0f - 5.0f, posY - 9.0f);
                win.draw(tip);

                if (wn == "Spread") {
                    CircleShape lt(3.5f); lt.setFillColor(tipCol);
                    lt.setPosition(posX + 3.0f, posY - 2.0f); win.draw(lt);
                    CircleShape rt(3.5f); rt.setFillColor(tipCol);
                    rt.setPosition(posX + width - 7.0f, posY - 2.0f); win.draw(rt);
                }
                if (wn == "Piercing") {
                    RectangleShape bar(Vector2f(3.0f, 12.0f));
                    bar.setFillColor(Color(200, 0, 255, 200));
                    bar.setPosition(posX + width / 2.0f - 1.5f, posY - 14.0f);
                    win.draw(bar);
                }
            }
        }
    }
    void onCollision(GameObject*) override {}
};

// ============================================================================
//  ENEMY BASE
// ============================================================================
class Enemy : public Entity {
protected:
    float speedScale;
    float fireScale;
public:
    Enemy(float px, float py, float vx, float vy,
        float h, float d, float ht, float wd,
        float ss = 1.0f, float fs = 1.0f)
        : Entity(px, py, vx, vy, h, d, ht, wd), speedScale(ss), fireScale(fs) {
    }
    bool isEnemy() override { return true; }
    virtual void shoot(Bullet** b, int& cnt, int maxB) {}
    virtual ~Enemy() {}
};

// ============================================================================
//  DRONE  — size 54x54 (increased)
// ============================================================================
class Drone : public Enemy {
    Texture tex; Sprite spr; bool texOk;
    Clock   fireClock;
    float   fireInterval;
public:
    Drone(float px, float py, float ss = 1.0f, float fs = 1.0f)
        : Enemy(px, py, 0.0f, DRONE_BASE_SPD* ss, 25.0f, 20.0f, 54.0f, 54.0f, ss, fs) {
        fireInterval = (1.5f + (rand() % 200) / 100.0f) / fs;
        if (fireInterval < 0.4f) fireInterval = 0.4f;
        texOk = loadTextureClean(tex, "drone.png");
        if (texOk) { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y); }
    }
    void shoot(Bullet** b, int& cnt, int maxB) override {
        if (fireClock.getElapsedTime().asSeconds() < fireInterval) return;
        fireClock.restart();
        if (cnt < maxB) { b[cnt++] = new Bullet(posX + width / 2.0f - 2.0f, posY + height, 15.0f, 1, "Enemy"); if (gAudio)gAudio->playEnemyFire(); }
    }
    void update(float dt) override { posY += velY * dt; if (posY > WINDOW_H + 60) health = 0; }
    void draw(RenderWindow& win) override {
        if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
        else { RectangleShape r(Vector2f(width, height)); r.setFillColor(Color(220, 60, 60)); r.setPosition(posX, posY); win.draw(r); }
    }
    void onCollision(GameObject*) override {}
};

// ============================================================================
//  VIPER  — size 56x56 (increased)
// ============================================================================
class Viper : public Enemy {
    float   sinT, baseX;
    Texture tex; Sprite spr; bool texOk;
    Clock   fireClock;
    float   fireInterval;
public:
    Viper(float px, float py, float ss = 1.0f, float fs = 1.0f)
        : Enemy(px, py, 0.0f, VIPER_BASE_SPD* ss, 50.0f, 20.0f, 56.0f, 56.0f, ss, fs),
        sinT(0.0f), baseX(px) {
        fireInterval = (2.0f + (rand() % 150) / 100.0f) / fs;
        if (fireInterval < 0.6f) fireInterval = 0.6f;
        texOk = loadTextureClean(tex, "enemy.png");
        if (texOk) { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y); }
    }
    void shoot(Bullet** b, int& cnt, int maxB) override {
        if (fireClock.getElapsedTime().asSeconds() < fireInterval) return;
        fireClock.restart();
        if (cnt < maxB) { b[cnt++] = new Bullet(posX + width / 2.0f - 2.0f, posY + height, 18.0f, 1, "Enemy"); if (gAudio)gAudio->playEnemyFire(); }
    }
    void update(float dt) override {
        sinT += dt * 2.6f;
        posX = baseX + sinf(sinT) * 78.0f;
        posY += velY * dt;
        if (posX < 0)posX = 0; if (posX > WINDOW_W - width)posX = WINDOW_W - width;
        if (posY > WINDOW_H + 60)health = 0;
    }
    void draw(RenderWindow& win) override {
        if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
        else { RectangleShape r(Vector2f(width, height)); r.setFillColor(Color(180, 60, 220)); r.setPosition(posX, posY); win.draw(r); }
    }
    void onCollision(GameObject*) override {}
};

// ============================================================================
//  SEEKER  — size 52x52 (increased), locks X at spawn permanently
// ============================================================================
class Seeker : public Enemy {
    float   lockedX;
    Texture tex; Sprite spr; bool texOk;
public:
    Seeker(float px, float py, float playerX, float ss = 1.0f)
        : Enemy(px, py, 0.0f, 55.0f * ss, 30.0f, 30.0f, 52.0f, 52.0f, ss, 1.0f),
        lockedX(playerX) {
        texOk = loadTextureClean(tex, "seeker.png");
        if (texOk) { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y); }
    }
    void update(float dt) override {
        velY += SEEKER_ACCEL * speedScale * dt; if (velY > 650)velY = 650;
        float diff = lockedX - posX;
        if (fabsf(diff) > 2.0f) posX += (diff > 0 ? 1.0f : -1.0f) * 110.0f * dt;
        posY += velY * dt;
        if (posY > WINDOW_H + 60)health = 0;
    }
    void draw(RenderWindow& win) override {
        if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
        else {
            ConvexShape tri; tri.setPointCount(3);
            tri.setPoint(0, Vector2f(width / 2.0f, height));
            tri.setPoint(1, Vector2f(0, 0)); tri.setPoint(2, Vector2f(width, 0));
            tri.setFillColor(Color(255, 80, 0)); tri.setPosition(posX, posY); win.draw(tri);
        }
    }
    void onCollision(GameObject*) override {}
};

// ============================================================================
//  BOSS BASE
// ============================================================================
class Boss : public Enemy {
protected:
    float maxHP;
    int   phase;
    Clock phaseTimer, atkClock;
public:
    Boss(float px, float py, float hp, float d, float ht, float wd)
        : Enemy(px, py, 0, 0, hp, d, ht, wd), maxHP(hp), phase(1) {
    }
    bool  isBoss()   override { return true; }
    bool  isEnemy()  override { return true; }
    int   getPhase() const { return phase; }
    float getHPPct() const { return health / maxHP; }
    float getMaxHP() const { return maxHP; }

    void checkPhase(float p2Thresh = 0.66f, float p3Thresh = 0.33f) {
        int np = (health < maxHP * p3Thresh) ? 3 : (health < maxHP * p2Thresh) ? 2 : 1;
        if (np != phase) { phase = np; phaseTimer.restart(); }
    }
    void drawHPBar(RenderWindow& win) {
        float bw = 500, bx = WINDOW_W / 2 - bw / 2, by = 12;
        RectangleShape bg(Vector2f(bw, 20)); bg.setFillColor(Color(50, 0, 0));
        bg.setOutlineColor(Color::White); bg.setOutlineThickness(2); bg.setPosition(bx, by); win.draw(bg);
        Color fc = (phase == 1) ? Color(0, 210, 80) : (phase == 2) ? Color(255, 200, 0) : Color(255, 30, 0);
        float pct = getHPPct(); if (pct < 0)pct = 0;
        RectangleShape fg(Vector2f(bw * pct, 20)); fg.setFillColor(fc); fg.setPosition(bx, by); win.draw(fg);
    }
    virtual void handleBulletHit(Bullet* b) { takeDamage(b->getBulletDmg()); }
    virtual void attackPattern(Bullet** b, int& cnt, int maxB, float plrX, float plrY) = 0;
    virtual bool  isLaserActive()  const { return false; }
    virtual float getLaserY()      const { return -1.0f; }
    virtual bool  isLaser2Active() const { return false; }
    virtual float getLaser2Y()     const { return -1.0f; }
    virtual bool  wantsSeeker() { return false; }
    virtual int   seekerSpawnCount() { return 1; }
    virtual ~Boss() {}
};

// ============================================================================
//  CRUISER  (Level 1 Boss)
//  HP=700 (buffed), phase2@40%, phase3@20%
//  Firing: tighter delay for more pressure
// ============================================================================
class Cruiser : public Boss {
    float   moveDir;
    bool    entryDone;   // tracks dramatic entry flag
    float   entryTimer;
    Texture tex; Sprite spr; bool texOk;
public:
    Cruiser(float px, float py)
        : Boss(px, py, 700, 30, 85, 180), moveDir(1),
        entryDone(false), entryTimer(0.0f) {
        texOk = loadTextureClean(tex, "boss1.png");
        if (texOk) { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y); }
    }
    bool isEntryDone() const { return entryDone; }

    void update(float dt) override {
        // Brief dramatic entry freeze (0.9s before moving/shooting)
        if (!entryDone) {
            entryTimer += dt;
            if (entryTimer >= 0.9f) entryDone = true;
            return;
        }
        float spd = (phase == 1) ? 200.0f : (phase == 2) ? 260.0f : 320.0f;
        posX += moveDir * spd * dt;
        if (posX < 20)moveDir = 1;
        if (posX > WINDOW_W - width - 20)moveDir = -1;
        checkPhase(0.40f, 0.20f);
    }
    void attackPattern(Bullet** b, int& cnt, int maxB, float, float) override {
        if (!entryDone) return;
        // Buffed: tighter intervals
        float delay = (phase == 1) ? 1.6f : (phase == 2) ? 1.1f : 0.75f;
        if (atkClock.getElapsedTime().asSeconds() < delay) return;
        atkClock.restart();
        if (phase < 3) {
            int gap = rand() % 10; float sw = WINDOW_W / 10;
            for (int i = 0; i < 10; i++) {
                if (i == gap) continue; if (cnt >= maxB) break;
                b[cnt++] = new Bullet(i * sw + sw / 2 - 2, posY + height + 4, 22, 1, "Enemy");
            }
        }
        else {
            // phase3: two rows, no gap
            float sw = WINDOW_W / 10;
            for (int row = 0; row < 2; row++)
                for (int i = 0; i < 10 && cnt < maxB; i++)
                    b[cnt++] = new Bullet(i * sw + sw / 2 - 2, posY + height + 4 + row * 18, 22, 1, "Enemy");
        }
        if (gAudio) gAudio->playEnemyFire();
    }
    void draw(RenderWindow& win) override {
        if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
        else {
            RectangleShape body(Vector2f(width, height));
            body.setFillColor(Color(160, 0, 200)); body.setPosition(posX, posY); win.draw(body);
        }
        // Entry flash overlay
        if (!entryDone) {
            float p = sinf(entryTimer * 18.0f) * 0.5f + 0.5f;
            RectangleShape fl(Vector2f(width, height));
            fl.setFillColor(Color(255, 50, 50, (Uint8)(p * 180)));
            fl.setPosition(posX, posY); win.draw(fl);
        }
    }
    void onCollision(GameObject*) override {}
};

// ============================================================================
//  TURRET  (Composition member of TwinCannons)
// ============================================================================
class Turret {
    float posX, posY, health, maxHP;
    float w, h;
    Clock fireClock;
public:
    Turret(float px, float py, float hp)
        : posX(px), posY(py), health(hp), maxHP(hp), w(28.0f), h(28.0f) {
    }
    float getPosX()  const { return posX; }
    float getPosY()  const { return posY; }
    float getW()     const { return w; }
    float getH()     const { return h; }
    float getHP()    const { return health; }
    float getMaxHP() const { return maxHP; }
    bool  isAlive()  const { return health > 0; }
    void  setPos(float px, float py) { posX = px; posY = py; }
    void  takeDamage(float d) { health -= d; if (health < 0)health = 0; }
    void fire(Bullet** b, int& cnt, int maxB, float delay) {
        if (!isAlive() || fireClock.getElapsedTime().asSeconds() < delay) return;
        fireClock.restart();
        if (cnt < maxB) { b[cnt++] = new Bullet(posX + w / 2 - 2, posY + h, 20, 1, "Enemy"); if (gAudio)gAudio->playEnemyFire(); }
    }
    bool overlaps(float ex, float ey, float ew, float eh) const {
        return ex<posX + w && ex + ew>posX && ey<posY + h && ey + eh>posY;
    }
    void draw(RenderWindow& win) {
        CircleShape tc(14);
        tc.setFillColor(isAlive() ? Color(0, 200, 220) : Color(70, 70, 70));
        tc.setOutlineColor(Color::White); tc.setOutlineThickness(2);
        tc.setPosition(posX - 14, posY - 14); win.draw(tc);
        RectangleShape hbg(Vector2f(28, 4)); hbg.setFillColor(Color(80, 0, 0));
        hbg.setPosition(posX - 14, posY - 26); win.draw(hbg);
        float hpct = health / maxHP; if (hpct < 0)hpct = 0;
        RectangleShape hfg(Vector2f(28 * hpct, 4)); hfg.setFillColor(Color(0, 220, 80));
        hfg.setPosition(posX - 14, posY - 26); win.draw(hfg);
    }
};

// ============================================================================
//  TWIN CANNONS  (Level 2 Boss — COMPOSITION with Turret)
//  BF-7: AABB turret routing
//  Turrets: 350 HP each. Core: 320 HP active after both die. Dense spread fire.
// ============================================================================
class TwinCannons : public Boss {
    Turret* leftTurret;
    Turret* rightTurret;
    float   sinPhase;
    bool    coreVulnerable;
    float   coreHP, coreMaxHP;
    Clock   coreFirClock;
    bool    leftSeekersSpawned, rightSeekersSpawned;
    int     pendingSeekers;
    Texture tex; Sprite spr; bool texOk;
public:
    TwinCannons(float px, float py)
        : Boss(px, py, 0.001f, 25, 100, 220), sinPhase(0),
        coreVulnerable(false), coreHP(320.0f), coreMaxHP(320.0f),
        leftSeekersSpawned(false), rightSeekersSpawned(false), pendingSeekers(0) {
        health = 0.001f;
        leftTurret = new Turret(px + 30, py + 70, 350.0f);
        rightTurret = new Turret(px + 190, py + 70, 350.0f);
        texOk = loadTextureClean(tex, "boss2.png");
        if (texOk) { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y); }
    }
    ~TwinCannons() override { delete leftTurret; delete rightTurret; }

    bool isDead() const {
        if (!coreVulnerable) return false;
        return coreHP <= 0;
    }

    void update(float dt) override {
        sinPhase += dt * 0.85f; posY = 55.0f + sinf(sinPhase) * 28.0f;
        leftTurret->setPos(posX + 30, posY + 70);
        rightTurret->setPos(posX + 190, posY + 70);

        bool wasVuln = coreVulnerable;
        coreVulnerable = (!leftTurret->isAlive() && !rightTurret->isAlive());
        if (!wasVuln && coreVulnerable) { health = coreHP; maxHP = coreMaxHP; }

        if (!leftSeekersSpawned && !leftTurret->isAlive()) {
            leftSeekersSpawned = true; pendingSeekers += 3;
        }
        if (!rightSeekersSpawned && !rightTurret->isAlive()) {
            rightSeekersSpawned = true; pendingSeekers += 3;
        }

        if (coreVulnerable) {
            int np = (coreHP < coreMaxHP * 0.33f) ? 3 : (coreHP < coreMaxHP * 0.66f) ? 2 : 1;
            if (np != phase) { phase = np; phaseTimer.restart(); }
        }
    }

    void attackPattern(Bullet** b, int& cnt, int maxB, float, float) override {
        // Turret fire — faster than before
        float delay = (phase == 1) ? 1.4f : (phase == 2) ? 1.0f : 0.70f;
        if (leftTurret->isAlive())  leftTurret->fire(b, cnt, maxB, delay);
        if (rightTurret->isAlive()) rightTurret->fire(b, cnt, maxB, delay);
        if (!leftTurret->isAlive() && rightTurret->isAlive())
            rightTurret->fire(b, cnt, maxB, delay * 0.4f);
        if (!rightTurret->isAlive() && leftTurret->isAlive())
            leftTurret->fire(b, cnt, maxB, delay * 0.4f);

        // Core: dense rapid spread when both turrets dead
        if (coreVulnerable && coreFirClock.getElapsedTime().asSeconds() >= 0.55f) {
            coreFirClock.restart();
            float cx = posX + width / 2.0f, cy = posY + height;
            float angles[] = { -0.45f,-0.22f,0.0f,0.22f,0.45f };
            for (int i = 0; i < 5 && cnt < maxB; i++) {
                Bullet* bl = new Bullet(cx, cy, 22, 1, "Enemy");
                bl->setVelocity(sinf(angles[i]) * BULLET_ENEMY_SPD,
                    BULLET_ENEMY_SPD * cosf(angles[i]));
                b[cnt++] = bl;
            }
            if (gAudio) gAudio->playEnemyFire();
        }
    }

    // BF-7: AABB turret routing
    void handleBulletHit(Bullet* b) override {
        float bx = b->getPosX(), by = b->getPosY();
        float bw = b->getWidth(), bh = b->getHeight();
        float dmg = b->getBulletDmg();
        if (leftTurret->isAlive() && leftTurret->overlaps(bx, by, bw, bh)) {
            leftTurret->takeDamage(dmg * 1.5f); return;
        }
        if (rightTurret->isAlive() && rightTurret->overlaps(bx, by, bw, bh)) {
            rightTurret->takeDamage(dmg * 1.5f); return;
        }
        if (coreVulnerable) {
            coreHP -= dmg; if (coreHP < 0)coreHP = 0;
            health = coreHP;
        }
    }

    bool wantsSeeker() override {
        if (pendingSeekers > 0) { pendingSeekers--; return true; } return false;
    }

    void drawHPBar(RenderWindow& win) {
        float bw = 500, bx = WINDOW_W / 2 - bw / 2, by = 12;
        RectangleShape bg(Vector2f(bw, 20)); bg.setFillColor(Color(50, 0, 0));
        bg.setOutlineColor(Color::White); bg.setOutlineThickness(2); bg.setPosition(bx, by); win.draw(bg);
        float pct;
        if (!coreVulnerable) {
            float combined = leftTurret->getHP() + rightTurret->getHP();
            pct = combined / 700.0f;
        }
        else {
            pct = coreHP / coreMaxHP;
        }
        if (pct < 0)pct = 0;
        Color fc = coreVulnerable ? Color(255, 30, 0) : Color(0, 210, 200);
        RectangleShape fg(Vector2f(bw * pct, 20)); fg.setFillColor(fc); fg.setPosition(bx, by); win.draw(fg);
    }

    void draw(RenderWindow& win) override {
        if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
        else {
            Color bc = coreVulnerable ? Color(255, 80, 0) : Color(0, 150, 200);
            RectangleShape body(Vector2f(width, height)); body.setFillColor(bc); body.setPosition(posX, posY); win.draw(body);
            if (!coreVulnerable) {
                RectangleShape sh(Vector2f(width, height)); sh.setFillColor(Color(0, 100, 255, 60));
                sh.setOutlineColor(Color(0, 200, 255)); sh.setOutlineThickness(3); sh.setPosition(posX, posY); win.draw(sh);
            }
        }
        leftTurret->draw(win); rightTurret->draw(win);
    }
    void onCollision(GameObject*) override {}
};

// ============================================================================
//  MOTHERSHIP  (Level 3 Boss)
//  HP=900, width=260, height=110
//  Vertical full-screen laser (70% width, centered) + seekers after laser ends
//  NO bullet attacks — only laser + seekers
// ============================================================================
class Mothership : public Boss {
    float moveDir;

    // Laser 1
    bool  laserWarn, laserActive;
    float laserYStart;   // always posY+height, extends to WINDOW_H
    Clock laserWarnClock, laserActiveClock, laserCycleClock;
    float laserDmgTimer;

    // Laser 2 (phase2+) — slight X offset, delayed cycle
    bool  laser2Warn, laser2Active;
    Clock laser2WarnClock, laser2ActiveClock, laser2CycleClock;
    float laser2DmgTimer;

    // Seekers
    Clock seekerClock;
    int   pendingSeekers;
    bool  seekerBurstPending;  // spawns seekers right as laser disappears

    Texture tex; Sprite spr; bool texOk;

    float laserW() const { return WINDOW_W * 0.70f; }
    float laserX() const { return posX + width / 2.0f - laserW() / 2.0f; }

public:
    Mothership(float px, float py)
        : Boss(px, py, 900, 40, 110, 260), moveDir(1),
        laserWarn(false), laserActive(false), laserYStart(200), laserDmgTimer(0),
        laser2Warn(false), laser2Active(false), laser2DmgTimer(0),
        pendingSeekers(0), seekerBurstPending(false) {
        texOk = loadTextureClean(tex, "boss3.png");
        if (texOk) { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y); }
    }

    bool  isLaserActive()  const override { return laserActive; }
    float getLaserY()      const override { return laserYStart; }
    bool  isLaser2Active() const override { return laser2Active; }
    float getLaser2Y()     const override { return laserYStart; }  // same Y for both lasers in this impl

    float getLaserX() const { return laserX(); }
    float getLaserWd() const { return laserW(); }

    bool wantsSeeker() override {
        if (pendingSeekers > 0) { pendingSeekers--; return true; } return false;
    }
    int seekerSpawnCount() override { return (phase >= 3) ? 2 : 1; }

    void tickLaserDamage(float dt) { laserDmgTimer += dt; laser2DmgTimer += dt; }
    bool consumeLaserDamage() { if (laserDmgTimer >= 0.3f) { laserDmgTimer = 0; return true; }return false; }
    bool consumeLaser2Damage() { if (laser2DmgTimer >= 0.3f) { laser2DmgTimer = 0; return true; }return false; }

    void update(float dt) override {
        posX += moveDir * 75 * dt;
        if (posX < 20)moveDir = 1; if (posX > WINDOW_W - width - 20)moveDir = -1;

        laserYStart = posY + height;

        // Laser 1 state machine
        if (laserWarn && laserWarnClock.getElapsedTime().asSeconds() >= 1.5f) {
            laserWarn = false; laserActive = true;
            laserActiveClock.restart(); laserDmgTimer = 0;
        }
        if (laserActive && laserActiveClock.getElapsedTime().asSeconds() >= 2.0f) {
            laserActive = false;
            seekerBurstPending = true;  // spawn seekers right when laser ends
        }

        // Laser 2 (phase2+)
        if (phase >= 2) {
            if (laser2Warn && laser2WarnClock.getElapsedTime().asSeconds() >= 1.5f) {
                laser2Warn = false; laser2Active = true;
                laser2ActiveClock.restart(); laser2DmgTimer = 0;
            }
            if (laser2Active && laser2ActiveClock.getElapsedTime().asSeconds() >= 2.0f) {
                laser2Active = false;
                pendingSeekers += (phase >= 3) ? 2 : 1;
            }
        }

        // Seeker burst after laser 1 ends
        if (seekerBurstPending) {
            seekerBurstPending = false;
            pendingSeekers += (phase >= 3) ? 3 : 2;
        }

        // Periodic additional seekers
        float si = (phase == 1) ? 6.0f : (phase == 2) ? 4.0f : 2.5f;
        if (seekerClock.getElapsedTime().asSeconds() >= si) {
            pendingSeekers += (phase >= 3) ? 2 : 1;
            seekerClock.restart();
        }

        checkPhase();
    }

    void attackPattern(Bullet** b, int& cnt, int maxB, float, float) override {
        // Laser 1 cycle trigger
        float lc = (phase == 1) ? 8.0f : (phase == 2) ? 5.5f : 3.5f;
        if (!laserWarn && !laserActive && laserCycleClock.getElapsedTime().asSeconds() >= lc) {
            laserWarn = true;
            laserWarnClock.restart(); laserCycleClock.restart();
        }
        // Laser 2 cycle trigger (phase2+, offset)
        if (phase >= 2) {
            float lc2 = lc + 2.5f;
            if (!laser2Warn && !laser2Active && laser2CycleClock.getElapsedTime().asSeconds() >= lc2) {
                laser2Warn = true;
                laser2WarnClock.restart(); laser2CycleClock.restart();
            }
        }
    }

    void draw(RenderWindow& win) override {
        float lx = laserX(), lw = laserW();
        float ly = laserYStart;

        // Laser 1 warning blink (full vertical bar from boss to bottom)
        if (laserWarn) {
            float t = laserWarnClock.getElapsedTime().asSeconds();
            Uint8 a = (Uint8)((sinf(t * 20.0f) + 1.0f) * 0.5f * 210);
            float barH = WINDOW_H - ly;
            if (barH > 0) {
                RectangleShape ws(Vector2f(lw, barH));
                ws.setFillColor(Color(255, 30, 30, a)); ws.setPosition(lx, ly); win.draw(ws);
            }
        }
        // Laser 1 active
        if (laserActive) {
            float barH = WINDOW_H - ly;
            if (barH > 0) {
                RectangleShape beam(Vector2f(lw, barH));
                beam.setFillColor(Color(255, 30, 30, 230));
                beam.setOutlineColor(Color(255, 200, 0)); beam.setOutlineThickness(3);
                beam.setPosition(lx, ly); win.draw(beam);
                // Bright core line
                RectangleShape core(Vector2f(lw * 0.18f, barH));
                core.setFillColor(Color(255, 255, 200, 200));
                core.setPosition(lx + lw * 0.41f, ly); win.draw(core);
            }
        }
        // Laser 2 (phase2+) — slightly narrower, orange tint
        if (phase >= 2) {
            float lx2 = lx + lw * 0.08f, lw2 = lw * 0.84f;
            if (laser2Warn) {
                float t = laser2WarnClock.getElapsedTime().asSeconds();
                Uint8 a = (Uint8)((sinf(t * 20.0f) + 1.0f) * 0.5f * 190);
                float barH = WINDOW_H - ly;
                if (barH > 0) {
                    RectangleShape ws(Vector2f(lw2, barH));
                    ws.setFillColor(Color(255, 130, 0, a)); ws.setPosition(lx2, ly); win.draw(ws);
                }
            }
            if (laser2Active) {
                float barH = WINDOW_H - ly;
                if (barH > 0) {
                    RectangleShape beam(Vector2f(lw2, barH));
                    beam.setFillColor(Color(255, 100, 0, 200));
                    beam.setOutlineColor(Color(255, 220, 0)); beam.setOutlineThickness(2);
                    beam.setPosition(lx2, ly); win.draw(beam);
                }
            }
        }

        if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
        else {
            RectangleShape body(Vector2f(width, height)); body.setFillColor(Color(30, 30, 70));
            body.setOutlineColor(Color(80, 80, 255)); body.setOutlineThickness(3);
            body.setPosition(posX, posY); win.draw(body);
            CircleShape core(28); core.setFillColor(Color(180, 0, 255));
            core.setPosition(posX + width / 2 - 28, posY + height / 2 - 28); win.draw(core);
        }
    }
    void onCollision(GameObject*) override {}
};

// ============================================================================
//  COLLISION MANAGER
// ============================================================================
class CollisionManager {
    bool aabb(Entity* a, Entity* b) {
        return (a->getPosX() < b->getPosX() + b->getWidth() &&
            a->getPosY() < b->getPosY() + b->getHeight() &&
            a->getPosX() + a->getWidth() > b->getPosX() &&
            a->getPosY() + a->getHeight() > b->getPosY());
    }
public:
    void checkAll(Player* player,
        Bullet** bullets, int bulletCnt,
        Enemy** enemies, int enemyCnt,
        Asteroid** asteroids, int asteroidCnt,
        PowerUp** powerups, int powerUpCnt,
        Boss* boss) {

        for (int i = 0; i < bulletCnt; i++) {
            Bullet* blt = bullets[i];
            if (!blt || blt->getHealth() <= 0) continue;

            if (blt->isPlayerBullet()) {
                for (int j = 0; j < enemyCnt && blt->getHealth()>0; j++) {
                    Enemy* en = enemies[j];
                    if (!en || en->getHealth() <= 0) continue;
                    if (aabb(blt, en)) { en->takeDamage(blt->getBulletDmg()); blt->consumeHit(); }
                }
                if (boss && boss->getHealth() > 0 && blt->getHealth() > 0 && aabb(blt, boss)) {
                    boss->handleBulletHit(blt); blt->consumeHit();
                }
            }

            if (blt->getHealth() > 0)
                for (int k = 0; k < asteroidCnt; k++)
                    if (asteroids[k] && aabb(blt, asteroids[k])) { blt->setHealth(0); break; }

            if (blt->isEnemyBullet() && blt->getHealth() > 0 && player && !player->isInvincible() && aabb(blt, player)) {
                player->takeDamageFromEnemy(); blt->setHealth(0);
            }
        }

        if (!player) return;

        for (int i = 0; i < enemyCnt; i++) {
            Enemy* en = enemies[i];
            if (!en || en->getHealth() <= 0) continue;
            if (!player->isInvincible() && aabb(player, en)) { player->takeDamageFromEnemy(); en->setHealth(0); }
        }
        if (boss && boss->getHealth() > 0 && !player->isInvincible() && aabb(player, boss))
            player->takeDamageFromEnemy();
        for (int i = 0; i < asteroidCnt; i++)
            if (asteroids[i] && !player->isInvincible() && aabb(player, asteroids[i])) { player->takeDamageFromEnemy(); break; }

        for (int i = 0; i < powerUpCnt; i++) {
            PowerUp* pu = powerups[i];
            if (!pu || pu->getHealth() <= 0) continue;
            if (aabb(player, pu)) {
                int t = pu->getType();
                if (t == 0) player->equipWeapon(new SpreadModule());
                else if (t == 1) player->equipWeapon(new PiercingModule());
                else if (t == 2) player->activateShield();
                else if (t == 3) player->addEMP();
                if (gAudio) gAudio->playPowerup();
                pu->setHealth(0);
            }
        }
    }

    int removeDeadObjects(Player* player,
        Enemy** enemies, int& enemyCnt,
        Bullet** bullets, int& bulletCnt,
        Asteroid** asteroids, int& asteroidCnt,
        PowerUp** powerups, int& powerUpCnt,
        ExplosionEffect& fx) {
        int kills = 0, alive = 0;

        for (int i = 0; i < enemyCnt; i++) {
            if (enemies[i] && enemies[i]->getHealth() > 0) { enemies[alive++] = enemies[i]; }
            else if (enemies[i]) {
                if (player) { player->addScore(100); player->registerKill(); }
                kills++;
                fx.spawn(enemies[i]->getPosX() + enemies[i]->getWidth() / 2,
                    enemies[i]->getPosY() + enemies[i]->getHeight() / 2, Color(255, 140, 0));
                if (gAudio) gAudio->playEnemyExp();
                // Drop table: 0=Spread,1=Piercing,2=Shield,3=EMP (no Rapid)
                if (powerUpCnt < MAX_POWERUPS) {
                    int roll = rand() % 100;
                    float dx = enemies[i]->getPosX() + enemies[i]->getWidth() / 2 - 18;
                    float dy = enemies[i]->getPosY();
                    if (roll < 15) powerups[powerUpCnt++] = new PowerUp(dx, dy, rand() % 3); // 0/1/2
                    else if (roll < 20) powerups[powerUpCnt++] = new PowerUp(dx, dy, 3);        // EMP
                }
                delete enemies[i]; enemies[i] = nullptr;
            }
        }
        enemyCnt = alive; alive = 0;

        for (int i = 0; i < bulletCnt; i++) {
            if (bullets[i] && bullets[i]->getHealth() > 0) bullets[alive++] = bullets[i];
            else if (bullets[i]) { delete bullets[i]; bullets[i] = nullptr; }
        }
        bulletCnt = alive;

        for (int i = 0; i < asteroidCnt; i++)
            if (asteroids[i] && asteroids[i]->getHealth() <= 0) asteroids[i]->respawn();

        alive = 0;
        for (int i = 0; i < powerUpCnt; i++) {
            if (powerups[i] && powerups[i]->getHealth() > 0) powerups[alive++] = powerups[i];
            else if (powerups[i]) { delete powerups[i]; powerups[i] = nullptr; }
        }
        powerUpCnt = alive;
        return kills;
    }
};

// ============================================================================
//  GAME CLASS
// ============================================================================
class Game {
    RenderWindow    window;
    Font            font;
    bool            fontOk;
    GameState       state, prevPlayState;
    StarField       stars;
    ExplosionEffect explosions;
    CollisionManager colMgr;

    Player* player;
    Bullet* bullets[MAX_BULLETS];    int bulletCnt;
    Enemy* enemies[MAX_ENEMIES];    int enemyCnt;
    Asteroid* asteroids[MAX_ASTEROIDS]; int asteroidCnt;
    PowerUp* powerups[MAX_POWERUPS];  int powerUpCnt;
    Boss* boss;

    // Arcade — score-based progression
    int  arcadeLevel;
    bool bossSpawned;
    bool bossEntryTriggered;  // dramatic entry flag for Cruiser

    // Survival
    int   wave, waveTotal, waveSpawned;
    float waveSpeedScale, waveFireScale;
    bool  waveClearFlag;
    float waveClearTimer;

    // VFX
    bool  empFlashOn;
    Clock empFlashClock;
    bool  empShake;
    float shakeAmt;
    Clock shakeClock;

    Clock deltaClock, spawnClock, shootClock;
    int   menuSel;

    void initArrays() {
        bulletCnt = enemyCnt = asteroidCnt = powerUpCnt = 0; boss = nullptr;
        for (int i = 0; i < MAX_BULLETS; i++)   bullets[i] = nullptr;
        for (int i = 0; i < MAX_ENEMIES; i++)   enemies[i] = nullptr;
        for (int i = 0; i < MAX_ASTEROIDS; i++) asteroids[i] = nullptr;
        for (int i = 0; i < MAX_POWERUPS; i++)  powerups[i] = nullptr;
    }
    void clearAll() {
        for (int i = 0; i < bulletCnt; i++) { delete bullets[i]; bullets[i] = nullptr; }
        for (int i = 0; i < enemyCnt; i++) { delete enemies[i]; enemies[i] = nullptr; }
        for (int i = 0; i < asteroidCnt; i++) { delete asteroids[i]; asteroids[i] = nullptr; }
        for (int i = 0; i < powerUpCnt; i++) { delete powerups[i]; powerups[i] = nullptr; }
        if (boss) { delete boss; boss = nullptr; }
        bulletCnt = enemyCnt = asteroidCnt = powerUpCnt = 0;
    }

    void spawnAsteroidField() {
        int n = MIN_ASTEROIDS + rand() % (FIELD_ASTEROIDS - MIN_ASTEROIDS + 1);
        for (int i = 0; i < n && asteroidCnt < MAX_ASTEROIDS; i++)
            asteroids[asteroidCnt++] = new Asteroid(30 + (float)(rand() % (int)(WINDOW_W - 80)),
                (float)(rand() % (int)(WINDOW_H - 150)));
    }
    void ensureAsteroids() {
        while (asteroidCnt < MIN_ASTEROIDS && asteroidCnt < MAX_ASTEROIDS)
            asteroids[asteroidCnt++] = new Asteroid(30 + (float)(rand() % (int)(WINDOW_W - 80)), -80);
    }

    void inputGameplay(float) {
        float mx = 0, my = 0;
        if (Keyboard::isKeyPressed(Keyboard::Left))  mx = -PLAYER_SPD;
        if (Keyboard::isKeyPressed(Keyboard::Right)) mx = PLAYER_SPD;
        if (Keyboard::isKeyPressed(Keyboard::Up))    my = -PLAYER_SPD;
        if (Keyboard::isKeyPressed(Keyboard::Down))  my = PLAYER_SPD;
        player->setVelocity(mx, my);

        if (Keyboard::isKeyPressed(Keyboard::E) && (mx != 0 || my != 0)) {
            float dx = (mx > 0) ? 1 : (mx < 0 ? -1 : 0);
            float dy = (my > 0) ? 1 : (my < 0 ? -1 : 0);
            player->dash(dx, dy);
        }
        if (Keyboard::isKeyPressed(Keyboard::Space) &&
            shootClock.getElapsedTime().asSeconds() >= 0.22f) {
            player->fireWeapon(bullets, bulletCnt, MAX_BULLETS);
            shootClock.restart();
        }
        static bool nWas = false;
        bool nNow = Keyboard::isKeyPressed(Keyboard::N);
        if (nNow && !nWas && player->hasEMP()) activateEMP();
        nWas = nNow;
    }

    void activateEMP() {
        player->consumeEMP();
        for (int i = 0; i < enemyCnt; i++) if (enemies[i]) enemies[i]->setHealth(0);
        if (boss) boss->takeDamage(boss->getMaxHP() * 0.12f);
        empFlashOn = true; empFlashClock.restart();
        shakeAmt = 18.0f; shakeClock.restart(); empShake = true;
        if (gAudio) gAudio->playEMP();
    }

    // ---- Arcade ----
    void startArcade() {
        clearAll(); delete player;
        player = new Player(3, WINDOW_W / 2 - 31, WINDOW_H - 120, font);
        arcadeLevel = 1; bossSpawned = false; bossEntryTriggered = false;
        spawnAsteroidField(); spawnClock.restart();
        if (gAudio) gAudio->playGameplayMusic();
        state = STATE_ARCADE; prevPlayState = STATE_ARCADE;
    }

    void spawnArcadeEnemy() {
        if (bossSpawned || enemyCnt >= MAX_ENEMIES) return;
        if (spawnClock.getElapsedTime().asSeconds() < 1.7f) return;
        spawnClock.restart();
        float ss = 1.0f + (arcadeLevel - 1) * 0.25f;
        float sx = 20 + (float)(rand() % (int)(WINDOW_W - 60));
        int roll = rand() % 10;
        if (arcadeLevel >= 3 && roll < 3) enemies[enemyCnt++] = new Seeker(sx, -40, player->getPosX(), ss);
        else if (arcadeLevel >= 2 && roll < 5) enemies[enemyCnt++] = new Viper(sx, -40, ss);
        else enemies[enemyCnt++] = new Drone(sx, -40, ss);
    }

    // Score-based boss spawn thresholds
    int bossScoreThreshold() const {
        if (arcadeLevel == 1) return ARCADE_BOSS1_SCORE;
        if (arcadeLevel == 2) return ARCADE_BOSS2_SCORE;
        return ARCADE_BOSS3_SCORE;
    }

    void trySpawnBoss() {
        if (bossSpawned) return;
        if (player->getScore() < bossScoreThreshold()) return;
        if (enemyCnt > 0) return;   // wait for screen clear
        bossSpawned = true;
        bossEntryTriggered = false;
        float bx = WINDOW_W / 2 - 110;
        if (arcadeLevel == 1)      boss = new Cruiser(bx, 30);
        else if (arcadeLevel == 2) boss = new TwinCannons(bx - 10, 40);
        else                    boss = new Mothership(bx - 30, 20);
        // Dramatic entry: stop gameplay music, play boss music
        if (gAudio) gAudio->playBossEntry();
    }

    void updateArcade(float dt) {
        if (!bossSpawned) { spawnArcadeEnemy(); trySpawnBoss(); }
        if (boss) {
            boss->update(dt);
            boss->attackPattern(bullets, bulletCnt, MAX_BULLETS, player->getPosX(), player->getPosY());

            // Seeker spawning from boss
            while (boss->wantsSeeker() && enemyCnt < MAX_ENEMIES) {
                int sc = boss->seekerSpawnCount();
                for (int s = 0; s < sc && enemyCnt < MAX_ENEMIES; s++)
                    enemies[enemyCnt++] = new Seeker(20 + (float)(rand() % (int)(WINDOW_W - 60)),
                        -40, player->getPosX(), 1.3f);
                break;
            }

            // Mothership laser damage
            Mothership* ms = dynamic_cast<Mothership*>(boss);
            if (ms) {
                ms->tickLaserDamage(dt);
                float lx = ms->getLaserX(), lw = ms->getLaserWd();
                float ly = ms->getLaserY();
                float barH = WINDOW_H - ly;
                if (ms->isLaserActive() && !player->isInvincible() && barH > 0) {
                    bool xOvlp = (player->getPosX() + player->getWidth() > lx && player->getPosX() < lx + lw);
                    bool yOvlp = (player->getPosY() + player->getHeight() > ly && player->getPosY() < ly + barH);
                    if (xOvlp && yOvlp && ms->consumeLaserDamage())
                        player->takeDamageFromEnemy();
                }
                if (ms->isLaser2Active() && !player->isInvincible() && barH > 0) {
                    bool xOvlp = (player->getPosX() + player->getWidth() > lx && player->getPosX() < lx + lw);
                    bool yOvlp = (player->getPosY() + player->getHeight() > ly && player->getPosY() < ly + barH);
                    if (xOvlp && yOvlp && ms->consumeLaser2Damage())
                        player->takeDamageFromEnemy();
                }
            }

            // TwinCannons death check
            TwinCannons* tc = dynamic_cast<TwinCannons*>(boss);
            bool bossKilled = false;
            if (tc)  bossKilled = tc->isDead();
            else    bossKilled = (boss->getHealth() <= 0);

            if (bossKilled) {
                // Clean single large explosion for Cruiser; standard for others
                Cruiser* cr = dynamic_cast<Cruiser*>(boss);
                if (cr)
                    explosions.spawnLarge(boss->getPosX() + boss->getWidth() / 2,
                        boss->getPosY() + boss->getHeight() / 2, Color(255, 200, 0));
                else
                    explosions.spawn(boss->getPosX() + boss->getWidth() / 2,
                        boss->getPosY() + boss->getHeight() / 2, Color(255, 200, 0));

                if (gAudio) { gAudio->playBossExp(); gAudio->playGameplayMusic(); }
                player->addScore(1000 * arcadeLevel);
                delete boss; boss = nullptr;
                if (arcadeLevel >= 3) { state = STATE_WIN; return; }
                arcadeLevel++; bossSpawned = false; bossEntryTriggered = false;
                spawnClock.restart();
            }
        }
    }

    // ---- Survival ----
    void startSurvival() {
        clearAll(); delete player;
        player = new Player(3, WINDOW_W / 2 - 31, WINDOW_H - 120, font);
        wave = 1; waveTotal = 5; waveSpawned = 0;
        waveSpeedScale = 1.0f; waveFireScale = 1.0f;
        waveClearFlag = false; waveClearTimer = 0;
        spawnAsteroidField(); spawnClock.restart();
        if (gAudio) gAudio->playGameplayMusic();
        state = STATE_SURVIVAL; prevPlayState = STATE_SURVIVAL;
    }
    void spawnSurvivalEnemy() {
        if (waveClearFlag || waveSpawned >= waveTotal || enemyCnt >= MAX_ENEMIES) return;
        if (spawnClock.getElapsedTime().asSeconds() < 1.4f) return;
        spawnClock.restart();
        float sx = 20 + (float)(rand() % (int)(WINDOW_W - 60));
        int roll = rand() % 100;
        if (wave >= 8 && roll < 25)     enemies[enemyCnt++] = new Seeker(sx, -40, player->getPosX(), waveSpeedScale);
        else if (wave >= 5 && roll < 45) enemies[enemyCnt++] = new Viper(sx, -40, waveSpeedScale, waveFireScale);
        else                      enemies[enemyCnt++] = new Drone(sx, -40, waveSpeedScale, waveFireScale);
        waveSpawned++;
    }
    void updateSurvival(float dt) {
        if (waveClearFlag) {
            waveClearTimer += dt;
            if (waveClearTimer >= 2.5f) {
                wave++; waveTotal = 5 + (wave - 1) * 2; waveSpawned = 0;
                waveSpeedScale = 1.0f + (wave - 1) * 0.05f;
                waveFireScale = 1.0f + (wave - 1) * 0.10f;
                if (waveSpeedScale > 2.5f)waveSpeedScale = 2.5f;
                if (waveFireScale > 3.5f)waveFireScale = 3.5f;
                waveClearFlag = false; waveClearTimer = 0; spawnClock.restart();
            }
        }
        else {
            spawnSurvivalEnemy();
            if (waveSpawned >= waveTotal && enemyCnt == 0) {
                waveClearFlag = true; waveClearTimer = 0; player->addScore(500 * wave);
            }
        }
    }

    // ---- Common update ----
    void updateGameplay(float dt) {
        player->update(dt);
        explosions.update(dt);
        for (int i = 0; i < bulletCnt; i++)   if (bullets[i])   bullets[i]->update(dt);
        for (int i = 0; i < enemyCnt; i++)    if (enemies[i]) { enemies[i]->update(dt); enemies[i]->shoot(bullets, bulletCnt, MAX_BULLETS); }
        for (int i = 0; i < asteroidCnt; i++) if (asteroids[i]) asteroids[i]->update(dt);
        for (int i = 0; i < powerUpCnt; i++)  if (powerups[i])  powerups[i]->update(dt);

        colMgr.checkAll(player, bullets, bulletCnt, enemies, enemyCnt,
            asteroids, asteroidCnt, powerups, powerUpCnt, boss);

        // MF-2: shield break cyan burst
        if (player->popShieldBroke())
            explosions.spawn(player->getPosX() + player->getWidth() / 2,
                player->getPosY() + player->getHeight() / 2, Color(0, 200, 255));

        // MF-3: screen shake
        if (player->popTookDamage() && !empShake) { shakeAmt = 9.0f; shakeClock.restart(); }

        colMgr.removeDeadObjects(player, enemies, enemyCnt, bullets, bulletCnt,
            asteroids, asteroidCnt, powerups, powerUpCnt, explosions);

        ensureAsteroids();
        if (state == STATE_ARCADE)   updateArcade(dt);
        if (state == STATE_SURVIVAL) updateSurvival(dt);
        if (player && !player->isAlive()) { state = STATE_GAMEOVER; if (gAudio)gAudio->stopAll(); }
    }

    // ---- Draw ----
    void drawGameplayFrame() {
        window.clear(Color(4, 4, 16));

        // Screen shake (MF-3 / EMP)
        if (shakeAmt > 0) {
            float t = shakeClock.getElapsedTime().asSeconds();
            float dur = empShake ? 0.50f : 0.30f;
            if (t < dur) {
                float decay = 1.0f - t / dur;
                float ox = ((float)(rand() % 101 - 50) / 50.0f) * shakeAmt * decay;
                float oy = ((float)(rand() % 101 - 50) / 50.0f) * shakeAmt * decay;
                View v = window.getDefaultView(); v.move(ox, oy); window.setView(v);
            }
            else {
                shakeAmt = 0; empShake = false;
                window.setView(window.getDefaultView());
            }
        }

        stars.draw(window);
        for (int i = 0; i < asteroidCnt; i++) if (asteroids[i]) asteroids[i]->draw(window);
        for (int i = 0; i < powerUpCnt; i++)  if (powerups[i])  powerups[i]->draw(window);
        for (int i = 0; i < enemyCnt; i++)    if (enemies[i])   enemies[i]->draw(window);
        if (boss) boss->draw(window);
        for (int i = 0; i < bulletCnt; i++)   if (bullets[i])   bullets[i]->draw(window);
        player->draw(window);
        explosions.draw(window);

        if (empFlashOn) {
            float t = empFlashClock.getElapsedTime().asSeconds();
            if (t < 0.45f) {
                RectangleShape fl(Vector2f(WINDOW_W, WINDOW_H));
                fl.setFillColor(Color(80, 200, 255, (Uint8)((1 - t / 0.45f) * 210)));
                window.draw(fl);
            }
            else empFlashOn = false;
        }

        window.setView(window.getDefaultView());
        drawHUD();

        if (boss) {
            TwinCannons* tc = dynamic_cast<TwinCannons*>(boss);
            if (tc) tc->drawHPBar(window);
            else   boss->drawHPBar(window);
        }
        window.display();
    }

    void drawHUD() {
        if (!fontOk) return;
        float y = 8;
        auto txt = [&](const string& s, unsigned sz, Color c, float x, float yp) {
            Text t(s, font, sz); t.setFillColor(c); t.setPosition(x, yp); window.draw(t);
            };
        txt("Score: " + to_string(player->getScore()), 20, Color::White, 10, y); y += 24;
        int mul = player->getMultiplier();
        txt("x" + to_string(mul), 20, mul > 1 ? Color::Yellow : Color(140, 140, 140), 10, y); y += 24;
        string ls = "Lives: "; for (int i = 0; i < player->getLives(); i++) ls += "<3 ";
        txt(ls, 20, Color(255, 80, 80), 10, y); y += 24;
        txt("Wpn: " + player->getWeaponName(), 20, Color(60, 255, 140), 10, y); y += 24;
        int sh = player->getShieldHits();
        txt(sh > 0 ? ("Shield: [" + to_string(sh) + " hits]") : "Shield: ---", 20, sh > 0 ? Color::Cyan : Color(100, 100, 100), 10, y); y += 24;
        string es = "EMP: ";
        for (int i = 0; i < player->getEMPCount(); i++) es += "* ";
        for (int i = player->getEMPCount(); i < 3; i++) es += "o ";
        txt(es, 20, Color(255, 220, 0), 10, y); y += 24;
        txt("Dash: ", 20, Color::White, 10, y);
        RectangleShape dbg(Vector2f(90, 13)); dbg.setFillColor(Color(40, 40, 40)); dbg.setPosition(72, y + 4); window.draw(dbg);
        float dr = player->getDashCooldown(), dfw = (dr <= 0) ? 90 : (1 - dr / 3.0f) * 90;
        RectangleShape dfg(Vector2f(dfw, 13)); dfg.setFillColor(dr <= 0 ? Color(0, 210, 80) : Color(200, 140, 0)); dfg.setPosition(72, y + 4); window.draw(dfg);

        if (state == STATE_SURVIVAL) {
            Text wt("Wave " + to_string(wave), font, 24); wt.setFillColor(Color(100, 210, 255));
            wt.setPosition(WINDOW_W / 2 - wt.getLocalBounds().width / 2, 8); window.draw(wt);
            if (waveClearFlag) {
                Text ct("WAVE CLEAR!", font, 38); ct.setFillColor(Color::Yellow);
                ct.setPosition(WINDOW_W / 2 - ct.getLocalBounds().width / 2, WINDOW_H / 2 - 22); window.draw(ct);
            }
        }
        if (state == STATE_ARCADE) {
            Text lt("Level " + to_string(arcadeLevel), font, 22); lt.setFillColor(Color(255, 180, 80));
            lt.setPosition(WINDOW_W / 2 - lt.getLocalBounds().width / 2, 8); window.draw(lt);
            if (bossSpawned && boss) {
                Text bt("!! BOSS !!", font, 22); bt.setFillColor(Color(255, 30, 30));
                bt.setPosition(WINDOW_W / 2 - bt.getLocalBounds().width / 2, 36); window.draw(bt);
            }
            else if (!bossSpawned) {
                // Show score progress toward boss
                int need = bossScoreThreshold();
                int cur = player->getScore();
                int pct = (int)(min(1.0f, (float)cur / (float)need) * 100);
                Text pt("Boss at " + to_string(need) + " pts  [" + to_string(pct) + "%]", font, 16);
                pt.setFillColor(Color(160, 160, 160));
                pt.setPosition(WINDOW_W / 2 - pt.getLocalBounds().width / 2, 36); window.draw(pt);
            }
        }
    }

    void drawMenu() {
        window.clear(Color(4, 4, 20)); stars.draw(window);
        if (!fontOk) { window.display(); return; }
        auto ctr = [&](const string& s, unsigned sz, Color c, float y) {
            Text t(s, font, sz); t.setFillColor(c);
            t.setPosition(WINDOW_W / 2 - t.getLocalBounds().width / 2, y); window.draw(t);
            };
        ctr("SPACE INVADERS", 56, Color(80, 200, 255), 125);
        ctr("Galactic Defense", 26, Color(140, 210, 255), 196);
        Color sc(255, 255, 0), dc(200, 200, 200);
        ctr("[1]  Arcade Mode", 32, menuSel == 0 ? sc : dc, 308);
        ctr("[2]  Survival Mode", 32, menuSel == 1 ? sc : dc, 368);
        ctr("Arrow Keys: Navigate  |  Enter / 1 / 2: Start", 17, Color(100, 100, 100), 458);
        ctr("Move:Arrows | Fire:Space | Dash:E+Dir | EMP:N | Pause:ESC", 15, Color(70, 70, 70), 490);
        window.display();
    }
    void inputMenu() {
        static bool uW = false, dW = false, eW = false;
        bool uN = Keyboard::isKeyPressed(Keyboard::Up);
        bool dN = Keyboard::isKeyPressed(Keyboard::Down);
        bool eN = Keyboard::isKeyPressed(Keyboard::Return);
        if ((uN && !uW) || (dN && !dW)) menuSel = 1 - menuSel;
        if (Keyboard::isKeyPressed(Keyboard::Num1)) { menuSel = 0; startArcade(); }
        if (Keyboard::isKeyPressed(Keyboard::Num2)) { menuSel = 1; startSurvival(); }
        if (eN && !eW) { if (menuSel == 0)startArcade(); else startSurvival(); }
        uW = uN; dW = dN; eW = eN;
    }

    void drawPause() {
        RectangleShape dim(Vector2f(WINDOW_W, WINDOW_H));
        dim.setFillColor(Color(0, 0, 0, 155)); window.draw(dim);
        if (fontOk) {
            auto ctr = [&](const string& s, unsigned sz, Color c, float y) {
                Text t(s, font, sz); t.setFillColor(c);
                t.setPosition(WINDOW_W / 2 - t.getLocalBounds().width / 2, y); window.draw(t);
                };
            ctr("PAUSED", 52, Color::White, 268);
            ctr("[R]  Resume", 28, Color(180, 255, 180), 358);
            ctr("[Q]  Quit to Menu", 28, Color(255, 140, 140), 408);
        }
        window.display();
    }
    void inputPause() {
        static bool rW = false, qW = false;
        bool rN = Keyboard::isKeyPressed(Keyboard::R), qN = Keyboard::isKeyPressed(Keyboard::Q);
        if (rN && !rW) { state = prevPlayState; if (gAudio) { if (boss)gAudio->playBossMusic(); else gAudio->playGameplayMusic(); } }
        if (qN && !qW) { clearAll(); delete player; player = nullptr; state = STATE_MENU; if (gAudio)gAudio->stopAll(); }
        rW = rN; qW = qN;
    }

    void drawGameOver() {
        window.clear(Color(10, 0, 0)); stars.draw(window);
        if (fontOk) {
            auto ctr = [&](const string& s, unsigned sz, Color c, float y) {
                Text t(s, font, sz); t.setFillColor(c);
                t.setPosition(WINDOW_W / 2 - t.getLocalBounds().width / 2, y); window.draw(t);
                };
            ctr("GAME OVER", 60, Color(255, 60, 60), 225);
            ctr("Final Score: " + to_string(player ? player->getScore() : 0), 32, Color::White, 315);
            ctr("[Enter]  Return to Menu", 24, Color(160, 160, 160), 398);
        }
        window.display();
        static bool eW = false; bool eN = Keyboard::isKeyPressed(Keyboard::Return);
        if (eN && !eW) { clearAll(); delete player; player = nullptr; state = STATE_MENU; }
        eW = eN;
    }
    void drawWin() {
        window.clear(Color(0, 10, 20)); stars.draw(window);
        if (fontOk) {
            auto ctr = [&](const string& s, unsigned sz, Color c, float y) {
                Text t(s, font, sz); t.setFillColor(c);
                t.setPosition(WINDOW_W / 2 - t.getLocalBounds().width / 2, y); window.draw(t);
                };
            ctr("YOU WIN!", 64, Color(100, 255, 120), 185);
            ctr("Mothership Destroyed!", 30, Color(200, 200, 200), 275);
            ctr("Score: " + to_string(player ? player->getScore() : 0), 34, Color::Yellow, 330);
            ctr("[Enter]  Return to Menu", 24, Color(130, 130, 130), 412);
        }
        window.display();
        static bool eW = false; bool eN = Keyboard::isKeyPressed(Keyboard::Return);
        if (eN && !eW) { clearAll(); delete player; player = nullptr; state = STATE_MENU; }
        eW = eN;
    }

public:
    Game()
        : window(VideoMode((int)WINDOW_W, (int)WINDOW_H),
            "Space Invaders: Galactic Defense", Style::Close),
        fontOk(false), state(STATE_MENU), prevPlayState(STATE_ARCADE),
        player(nullptr),
        bulletCnt(0), enemyCnt(0), asteroidCnt(0), powerUpCnt(0), boss(nullptr),
        arcadeLevel(1), bossSpawned(false), bossEntryTriggered(false),
        wave(1), waveTotal(5), waveSpawned(0),
        waveSpeedScale(1.0f), waveFireScale(1.0f),
        waveClearFlag(false), waveClearTimer(0),
        empFlashOn(false), empShake(false), shakeAmt(0.0f),
        menuSel(0) {
        window.setFramerateLimit(60);
        initArrays();
        fontOk = font.loadFromFile("arial.ttf");
        if (!fontOk) cout << "Warning: arial.ttf not found\n";
    }
    ~Game() { clearAll(); delete player; }

    void run() {
        while (window.isOpen()) {
            Event ev;
            while (window.pollEvent(ev)) {
                if (ev.type == Event::Closed) window.close();
                if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape)
                    if (state == STATE_ARCADE || state == STATE_SURVIVAL) {
                        prevPlayState = state; state = STATE_PAUSED;
                        if (gAudio) gAudio->stopAll();
                    }
            }
            float dt = deltaClock.restart().asSeconds();
            if (dt > 0.05f)dt = 0.05f;
            stars.update(dt);

            switch (state) {
            case STATE_MENU:
                drawMenu(); inputMenu(); break;
            case STATE_ARCADE:
            case STATE_SURVIVAL:
                inputGameplay(dt);
                updateGameplay(dt);
                drawGameplayFrame();
                break;
            case STATE_PAUSED:
                window.clear(Color(4, 4, 16)); stars.draw(window);
                for (int i = 0; i < asteroidCnt; i++) if (asteroids[i]) asteroids[i]->draw(window);
                for (int i = 0; i < powerUpCnt; i++)  if (powerups[i])  powerups[i]->draw(window);
                for (int i = 0; i < enemyCnt; i++)    if (enemies[i])   enemies[i]->draw(window);
                if (boss) boss->draw(window);
                for (int i = 0; i < bulletCnt; i++)   if (bullets[i])   bullets[i]->draw(window);
                if (player) player->draw(window);
                drawPause(); inputPause(); break;
            case STATE_GAMEOVER: drawGameOver(); break;
            case STATE_WIN:      drawWin();      break;
            }
        }
    }
};

// ============================================================================
//  ENTRY POINT
// ============================================================================
int main() {
    srand((unsigned int)time(nullptr));
    AudioManager audio;
    gAudio = &audio;
    Game game;
    game.run();
    gAudio = nullptr;
    return 0;
}