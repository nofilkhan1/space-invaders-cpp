
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using namespace sf;
using namespace std;

//  CONSTANTS}

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

const int   ARCADE_BOSS1_SCORE = 3000;
const int   ARCADE_BOSS2_SCORE = 7500;
const int   ARCADE_BOSS3_SCORE = 15000;


//  GAME STATE
enum GameState {
    STATE_MENU, STATE_INSTRUCTIONS, STATE_CONTROLS,
    STATE_ARCADE, STATE_SURVIVAL,
    STATE_PAUSED, STATE_GAMEOVER, STATE_WIN
};

//  TEXTURE HELPER

static bool loadTextureClean(Texture& tex, const char* file)
{
    Image img;
    if (!img.loadFromFile(file)) return false;
    Vector2u sz = img.getSize();
    for (unsigned y = 0; y < sz.y; ++y)
        for (unsigned x = 0; x < sz.x; ++x) {
            Color c = img.getPixel(x, y);
            if (c.r > 230 && c.g > 230 && c.b > 230)
                img.setPixel(x, y, Color(c.r, c.g, c.b, 0));
        }
    if (!tex.loadFromImage(img)) return false;
    tex.setSmooth(true);
    return true;
}

//  AUDIO MANAGER

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

    //called at game start so menu has music immediately

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
    void playBossEntry() {
        if (musicOk[0]) mGameplay.stop();
        bossPlaying = true;
        if (musicOk[1]) { mBoss.stop(); mBoss.play(); }
    }
    void stopAll() {
        if (musicOk[0]) mGameplay.stop();
        if (musicOk[1]) mBoss.stop();
        bossPlaying = false;
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


//  SCROLLING STARFIELD


class StarField 
{
    struct Star { float x, y, spd; Uint8 br; float r; };
    static const int N = 200;
    Star s[N];
public:
    StarField() {// speed density radius of stars
        for (int i = 0; i < N; i++)
        {
            s[i].x = (float)(rand() % (int)WINDOW_W);
            s[i].y = (float)(rand() % (int)WINDOW_H);
            int lay = i % 3;
            if (lay == 0) { s[i].spd = 22.0f; s[i].br = 55;  s[i].r = 1.0f; }
            else if (lay == 1) { s[i].spd = 55.0f; s[i].br = 140; s[i].r = 1.5f; }
            else { s[i].spd = 95.0f; s[i].br = 230; s[i].r = 2.0f; }
        }
    }
    void update(float dt) {// regenration of stars
        for (int i = 0; i < N; i++)
        {
            s[i].y += s[i].spd * dt;
            if (s[i].y > WINDOW_H + 4.0f) {
                s[i].y = -4.0f;
                s[i].x = (float)(rand() % (int)WINDOW_W);
            }
        }
    }
    void draw(RenderWindow& win) // stars shapes 
    {
        for (int i = 0; i < N; i++) {
            CircleShape c(s[i].r);
            c.setFillColor(Color(s[i].br, s[i].br, s[i].br));
            c.setPosition(s[i].x - s[i].r, s[i].y - s[i].r);
            win.draw(c);
        }
    }
};

//  EXPLOSION EFFECT

class ExplosionEffect {
    struct Particle { float x, y, vx, vy, life, maxLife, sz; Color color; bool active; };
    static const int PPC = 10;
    Particle pool[MAX_EXPLOSIONS][PPC];
    bool     used[MAX_EXPLOSIONS];
    Texture  expTex; bool expTexOk;

    void spawnI(float cx, float cy, Color col, float sMin, float sMax, float szMin, float szMax, float lMin) //basix enemies
    {
        for (int i = 0; i < MAX_EXPLOSIONS; i++) 
        {
            if (used[i]) continue; used[i] = true;
            for (int j = 0; j < PPC; j++) 
            {
                float ang = (j / (float)PPC) * 6.2832f;
                float spd = sMin + (float)(rand() % (int)(sMax - sMin + 1));
                pool[i][j] = { cx,cy,cosf(ang) * spd,sinf(ang) * spd,0.0f,
                    lMin + (float)(rand() % 32) / 100.0f,
                    szMin + (float)(rand() % (int)(szMax - szMin + 1)),col,true };
            }
            return;
        }
    }
public:
    ExplosionEffect() : expTexOk(false)
    {
        expTexOk = expTex.loadFromFile("explosion.png");
        if (expTexOk) expTex.setSmooth(true);
        for (int i = 0; i < MAX_EXPLOSIONS; i++) 
        {
            used[i] = false;
            for (int j = 0; j < PPC; j++)
                pool[i][j].active = false;
        }
    }
    void spawn(float cx, float cy, Color col)
    { 
        spawnI(cx, cy, col, 50, 190, 3, 9, 0.28f); 
    }
    void spawnLarge(float cx, float cy, Color col)//for boss
    {
        spawnI(cx, cy, col, 120, 300, 8, 18, 0.50f);
    }
    void update(float dt)//change in time is dt
    {
        for (int i = 0; i < MAX_EXPLOSIONS; i++)
        {
            if (!used[i]) continue;
            bool alive = false;
            for (int j = 0; j < PPC; j++)
            {
                if (!pool[i][j].active)//pool wo area jahan pa exploision horha ha
                    continue;
                pool[i][j].x += pool[i][j].vx * dt;
                pool[i][j].y += pool[i][j].vy * dt;
                pool[i][j].life += dt;
                if (pool[i][j].life >= pool[i][j].maxLife)
                    pool[i][j].active = false;
                else
                    alive = true;
            }
            if (!alive)
                used[i] = false;
        }
    }
    void draw(RenderWindow& win) {
        for (int i = 0; i < MAX_EXPLOSIONS; i++)
        {
            if (!used[i]) 
                continue;
            for (int j = 0; j < PPC; j++)
            {
                if (!pool[i][j].active)
                    continue;
                float ratio = 1.0f - pool[i][j].life / pool[i][j].maxLife;
                float sz = pool[i][j].sz * ratio; 
                if (sz < 0.3f)
                    continue;
                Color c = pool[i][j].color; c.a = (Uint8)(ratio * 220);
                if (expTexOk) 
                {
                    Sprite sp(expTex); 
                    float sc = sz * 2.0f / (float)expTex.getSize().x;
                    sp.setScale(sc, sc);
                    sp.setColor(c);
                    sp.setPosition(pool[i][j].x - sz, pool[i][j].y - sz); 
                    win.draw(sp, BlendAlpha);
                }
                else
                {
                    CircleShape cs(sz);
                    cs.setFillColor(c);
                    cs.setPosition(pool[i][j].x - sz, pool[i][j].y - sz);
                    win.draw(cs);
                }
            }
        }
    }
};

//  ABSTRACT BASE CLASSES

class GameObject {
public:
    virtual void update(float dt) = 0;
    virtual void draw(RenderWindow& window) = 0;
    virtual void onCollision(GameObject* other) = 0;
    virtual ~GameObject() {}
};

class Entity : public GameObject {
protected:
    float posX, posY, velX, velY, health, damage, height, width;
public:
    Entity(float px, float py, float vx, float vy, float h, float d, float ht, float wd)
        : posX(px), posY(py), velX(vx), velY(vy), health(h), damage(d), height(ht), width(wd)
    {
    }
    float getPosX()  const
    { 
        return posX;
    }
    float getPosY()  const
    { 
        return posY;
    }
    float getWidth() const 
    { 
        return width;
    }
    float getHeight()const 
    { 
        return height;
    }
    float getHealth()const
    {
        return health;
    }
    float getDamage()const 
    {
        return damage;
    }
    void setVelocity(float vx, float vy)
    {
        velX = vx; velY = vy;
    }
    void setPosition(float px, float py)
    {
        posX = px; posY = py; 
    }
    void setHealth(float h)
    {
        health = h;
    }
    void takeDamage(float d) 
    { 
        health -= d; 
        if (health < 0)
            health = 0;
    }
    virtual bool isPlayer()
    {
        return false; 
    }
    virtual bool isEnemy()
    {
        return false;
    }
    virtual bool isBoss() 
    { 
        return false;
    }
    virtual bool isPlayerBullet()
    {
        return false;
    }
    virtual bool isEnemyBullet() 
    {
        return false;
    }
    virtual bool isAsteroid() 
    {
        return false;
    }
    virtual bool isPowerUp()
    {
        return false;
    }
    virtual ~Entity()
    {}
};


//  BULLET


class Bullet : public Entity// vector 2u mai +ve values only 2f mai dono +ve -ve dono saktin hain
{
    float  bulletDmg; int hitsLeft; string owner;//string owner bata rha ha ka bullet kis ki ha
    RectangleShape shape; float lifeTimer, maxLife;
public:
    Bullet(float px, float py, float dmg, int hits, const string& own)
        : Entity(px, py, 0, own == "Player" ? -BULLET_PLAYER_SPD : BULLET_ENEMY_SPD,
            10.0f, dmg, own == "Player" ? 22.0f : 16.0f, own == "Player" ? 4.0f : 4.0f),
        bulletDmg(dmg), hitsLeft(hits), owner(own), lifeTimer(0.0f), maxLife(0.0f)
    {
        shape.setSize(Vector2f(width, height));
        shape.setFillColor(own == "Player" ? Color(60, 255, 140) : Color(255, 60, 60));
    }
    bool  isPlayerBullet() override
    {
        return owner == "Player"; 
    }
    bool  isEnemyBullet()  override 
    {
        return owner == "Enemy";
    }
    float getBulletDmg()   const
    {
        return bulletDmg;
    }
    void makeWide(float w, float h, float lt, Color col)
    {
        width = w;
        height = h;
        shape.setSize(Vector2f(w, h));
        shape.setFillColor(col);
        maxLife = lt;
    }
    void consumeHit() 
    {
        hitsLeft--; 
        if (hitsLeft <= 0) 
            health = 0.0f;
    }
    void setVelXY(float vx, float vy)
    {
        velX = vx;
        velY = vy;
    }
    void update(float dt) override
    {
        posX += velX * dt;
        posY += velY * dt;
        if (maxLife > 0.0f)
        { 
            lifeTimer += dt; 
        if (lifeTimer >= maxLife)
            health = 0.0f;
        }
        if (posY<-100 || posY>WINDOW_H + 100 || posX<-300 || posX>WINDOW_W + 300)
            health = 0.0f;
    }
    void draw(RenderWindow& win) override
    { 
        shape.setPosition(posX, posY);
    win.draw(shape);
    }
    void onCollision(GameObject*) override
    {}
};


//  WEAPON SYSTEM is a abstract class

class Weapon {
protected:
    float damage; string name;
public:
    Weapon(float d, const string& n) :damage(d), name(n)
    {}
    string getName() const
    { 
        return name; 
    }
    virtual void fire(float px, float py, float pw, Bullet** bullets, int& cnt, int maxB) = 0;
    virtual ~Weapon() 
    {}
};

class StandardModule : public Weapon
{
public:
    StandardModule() :Weapon(26.0f, "Standard")
    {}
    void fire(float px, float py, float pw, Bullet** b, int& cnt, int maxB) override
    {
        if (cnt < maxB)
        { 
            b[cnt++] = new Bullet(px + pw / 2.0f - 2.0f, py - 14.0f, damage, 1, "Player"); 
            if (gAudio)
                gAudio->playStdFire();
        }
    }
};

class SpreadModule : public Weapon// cx jo ha wo bata rha middle sa left right wali bullet kitna fasla oa hogi
{
public:
    SpreadModule() :Weapon(26.0f, "Spread")
    {}
    void fire(float px, float py, float pw, Bullet** b, int& cnt, int maxB) override 
    {
        if (cnt + 3 > maxB)
            return;
        float cx = px + pw / 2.0f;
        b[cnt++] = new Bullet(cx - 2.0f, py - 14.0f, damage, 1, "Player");
        Bullet* lb = new Bullet(cx - 14.0f, py - 8.0f, damage, 1, "Player"); 
        lb->setVelocity(-170.0f, -440.0f); 
        b[cnt++] = lb;
        Bullet* rb = new Bullet(cx + 10.0f, py - 8.0f, damage, 1, "Player");
        rb->setVelocity(170.0f, -440.0f); 
        b[cnt++] = rb;
        if (gAudio)
            gAudio->playSpreadFire();
    }
};

class PiercingModule : public Weapon
{
public:
    PiercingModule() :Weapon(55.0f, "Piercing") 
    {}
    void fire(float px, float py, float pw, Bullet** b, int& cnt, int maxB) override
    {
        if (cnt < maxB)
        {
            Bullet* bl = new Bullet(px + pw / 2.0f - 4.0f, py - 14.0f, damage, 2, "Player");
            bl->makeWide(8.0f, 22.0f, 0.0f, Color(180, 0, 255));
            b[cnt++] = bl;
            if (gAudio) 
                gAudio->playPierceFire();
        }
    }
};

//  POWERUP  (0=Spread  1=Piercing  2=Shield  3=EMP)

class PowerUp : public Entity {
    int type; CircleShape shape; Clock pulseClock; float rotAngle;
public:
    PowerUp(float px, float py, int t)
        : Entity(px, py, 0.0f, 55.0f, 10.0f, 0.0f, 36.0f, 36.0f), type(t), rotAngle(0.0f) 
    {
        shape.setRadius(18.0f); 
        shape.setOutlineThickness(4.0f);
        switch (t) 
        {
        case 0:
            shape.setFillColor(Color(255, 130, 0));
            shape.setOutlineColor(Color::Yellow);
            break;
        case 1:
            shape.setFillColor(Color(160, 0, 255));
            shape.setOutlineColor(Color::Cyan); 
            break;
        case 2:
            shape.setFillColor(Color(0, 200, 255)); 
            shape.setOutlineColor(Color::White); 
            break;
        case 3: 
            shape.setFillColor(Color(255, 220, 0));
            shape.setOutlineColor(Color::White);
            break;
        default:
            shape.setFillColor(Color(180, 180, 180));
            shape.setOutlineColor(Color::White);
            break;
        }
    }
    int  getType()   const 
    {
        return type;
    }
    bool isPowerUp() override 
    {
        return true;
    }
    void update(float dt) override
    {
        posY += velY * dt; rotAngle += 90.0f * dt;
        if (posY > WINDOW_H + 30)
            health = 0; 
    }
    void draw(RenderWindow& win) override 
    {
        float t = pulseClock.getElapsedTime().asSeconds();
        float p = sinf(t * 5.0f) * 0.5f + 0.5f; 
        float gr = 14.0f + p * 14.0f;
        CircleShape glow(gr); 
        Color gc = shape.getFillColor();
        gc.a = 80;
        glow.setFillColor(gc);
        glow.setPosition(posX + 18.0f - gr, posY + 18.0f - gr); 
        win.draw(glow);
        shape.setPosition(posX, posY);
        win.draw(shape);
        float cx = posX + 18.0f, cy = posY + 18.0f, rad = 26.0f;
        float base = rotAngle * 3.14159f / 180.0f; 
        Color lc = shape.getOutlineColor();
        for (int i = 0; i < 3; i++)
        {
            float ang = base + i * 2.0944f;
            float x1 = cx + cosf(ang) * rad, y1 = cy + sinf(ang) * rad;
            float x2 = cx + cosf(ang + 0.4f) * (rad - 8), y2 = cy + sinf(ang + 0.4f) * (rad - 8);
            Vertex line[] = { Vertex(Vector2f(x1,y1),lc),Vertex(Vector2f(x2,y2),lc) };// aik line mai move krwana ka lia ha
            win.draw(line, 2, Lines);
        }
    }
    void onCollision(GameObject*) override { health = 0.0f; }
};

//  ASTEROID

class Asteroid : public Entity
{
    Texture tex; Sprite spr; bool texOk; CircleShape fallback; float radius;
public:
    Asteroid(float px, float py) :Entity(px, py, 0, 0, 999.0f, 1.0f, 0, 0), texOk(false) 
    {
        float sz = 1.0f + (float)(rand() % 3);
        radius = 18.0f * sz; 
        width = height = radius * 2.0f;
        velY = 28.0f + (float)(rand() % 45);
        texOk = loadTextureClean(tex, "asteroid.png");
        if (texOk)
        {
            spr.setTexture(tex);
            spr.setScale(width / tex.getSize().x, height / tex.getSize().y); 
        }
        else 
        {
            fallback.setRadius(radius);
            fallback.setFillColor(Color(110, 95, 80));
            fallback.setOutlineColor(Color(170, 150, 120)); 
            fallback.setOutlineThickness(2.0f); }
    }
    bool isAsteroid() override
    {
        return true;
    }
    void respawn() 
    {
        posX = 30.0f + (float)(rand() % (int)(WINDOW_W - 80));
        posY = -radius * 2.0f - (float)(rand() % 60);
        velY = 28.0f + (float)(rand() % 45);
        health = 999.0f;
    }
    void update(float dt) override
    {
        posY += velY * dt;
        if (posY > WINDOW_H + 100) 
            health = 0.0f; 
    }
    void draw(RenderWindow& win) override 
    {
        if (texOk) 
        { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha);
        }
        else 
        {
            fallback.setPosition(posX, posY); 
            win.draw(fallback);
        }
    }
    void onCollision(GameObject*) override {}
};


//  PLAYER


class Player : public Entity {
    int lives, score, multiplier; float multiTimer; int shieldHits;
    Weapon* weapon;
    int empCount;
    Clock invincClock;
    float invincDuration; 
    bool invincible;
    Clock flashClock; bool hitFlash;
    bool shieldBrokeFlag, tookDamageFlag;
    Clock dashClock;
    Texture tex; Sprite spr;
    bool texOk;
    Font& font;
public:
    Player(int lv, float px, float py, Font& f)
        : Entity(px, py, 0, 0, 100.0f, 0.0f, 62.0f, 62.0f),
        lives(lv), score(0), multiplier(1), multiTimer(99.0f),
        shieldHits(0), weapon(new StandardModule()),
        empCount(1), invincDuration(0.8f), invincible(false),
        hitFlash(false), shieldBrokeFlag(false), tookDamageFlag(false), font(f) 
    {
        texOk = loadTextureClean(tex, "player.png");
        if (texOk)
        { 
            spr.setTexture(tex); 
            spr.setScale(width / tex.getSize().x, height / tex.getSize().y);
        }
    }
    ~Player()
    {
        delete weapon; 
    }
    int getLives() const 
    {
        return lives;
    }
    int    getScore() const 
    {
        return score;
    }
    int    getMultiplier() const 
    {
        return multiplier;
    }
    int    getEMPCount() const
    {
        return empCount; 
    }
    int    getShieldHits() const 
    {
        return shieldHits; 
    }
    string getWeaponName() const 
    {
        return weapon ? weapon->getName() : "None";
    }
    bool   isAlive() const 
    {
        return lives > 0;
    }
    bool   isInvincible()  const 
    {
        return invincible;
    }
    bool   isPlayer() override 
    {
        return true;
    }
    bool popShieldBroke() 
    {
        bool b = shieldBrokeFlag;
        shieldBrokeFlag = false; return b;
    }
    bool popTookDamage() 
    {
        bool b = tookDamageFlag;
        tookDamageFlag = false;  return b;
    }
    float getDashCooldown() const 
    { 
        float e = dashClock.getElapsedTime().asSeconds();
        return (e < 3.0f) ? (3.0f - e) : 0.0f; 
    }
    void addScore(int pts) 
    {
        score += pts * multiplier;
    }
    void registerKill() 
    {
        if (multiTimer < 3.0f) 
        {
            if (multiplier == 1) multiplier = 2;
            else if (multiplier == 2) multiplier = 4;
        }
        multiTimer = 0.0f;
    }
    void equipWeapon(Weapon* w) 
    {
        delete weapon;
        weapon = w;
    }
    void activateShield() 
    {
        shieldHits = 2;
    }
    void addEMP() {
        if (empCount < 3) empCount++;
    }
    bool hasEMP() const 
    {
        return empCount > 0;
    }
    void consumeEMP() 
    {
        if (empCount > 0)
            empCount--;
    }
    void fireWeapon(Bullet** b, int& cnt, int maxB) 
    { 
        if (weapon) weapon->fire(posX, posY, width, b, cnt, maxB);
    }
    void dash(float dx, float dy) 
    {
        if (dashClock.getElapsedTime().asSeconds() < 3.0f) 
            return;
        posX += dx * 115.0f; 
        posY += dy * 115.0f;
        posX = max(0.0f, min(posX, WINDOW_W - width));
        posY = max(0.0f, min(posY, WINDOW_H - height));
        dashClock.restart(); invincDuration = 0.5f;
        invincible = true;
        invincClock.restart();
    }
    void takeDamageFromEnemy() 
    {
        if (invincible) 
            return;
        multiplier = 1;
        multiTimer = 99.0f; 
        tookDamageFlag = true;
        if (shieldHits > 0) 
        {
            shieldHits--;
            if (shieldHits == 0) 
            { shieldBrokeFlag = true; 
            if (gAudio) 
                gAudio->playShieldBreak(); 
            }
        }
        else {
            lives--;
            delete weapon; 
            weapon = new StandardModule();
            hitFlash = true;
            flashClock.restart();
            if (gAudio)
                gAudio->playPlayerHit();
        }
        invincDuration = 0.8f; 
        invincible = true;
        invincClock.restart();
        health = (float)(lives * 34);
        if (health < 0) 
            health = 0;
    }
    void update(float dt) override
    {
        posX += velX * dt; 
        posY += velY * dt;
        posX = max(0.0f, min(posX, WINDOW_W - width));
        posY = max(0.0f, min(posY, WINDOW_H - height));
        if (invincible && invincClock.getElapsedTime().asSeconds() >= invincDuration)
            invincible = false;
        if (hitFlash && flashClock.getElapsedTime().asSeconds() >= 0.12f) 
            hitFlash = false;
        multiTimer += dt;
        if (multiTimer >= 3.0f && multiplier > 1)
            multiplier = 1;
    }
    void draw(RenderWindow& win) override
    {
        if (shieldHits > 0)
        {
            CircleShape sg(38.0f);
            sg.setFillColor(Color(0, 160, 255, 45));
            sg.setOutlineColor(Color(0, 220, 255, 180));
            sg.setOutlineThickness(3.0f);
            sg.setPosition(posX + width / 2.0f - 38.0f, posY + height / 2.0f - 38.0f);
            win.draw(sg);
        }
        if (hitFlash) 
        {
            RectangleShape fl(Vector2f(width, height)); 
            fl.setFillColor(Color(255, 255, 255, 160));
            fl.setPosition(posX, posY); win.draw(fl);
        }
        bool visible = true;
        if (invincible)
            visible = ((int)(invincClock.getElapsedTime().asSeconds() * 9)) % 2 == 0;
        if (visible)
        {
            if (texOk)
            { 
                spr.setPosition(posX, posY); win.draw(spr, BlendAlpha);
            }
            else 
            {
                RectangleShape r(Vector2f(width, height));
                r.setFillColor(Color(80, 160, 255));
                r.setPosition(posX, posY); win.draw(r);
            }
            if (weapon) 
            {
                string wn = weapon->getName();
                Color tc = (wn == "Standard") ? Color(0, 255, 120) : (wn == "Spread") ? Color(255, 140, 0) : Color(180, 0, 255);
                CircleShape tip(5.0f);
                tip.setFillColor(tc);
                tip.setOutlineColor(Color(255, 255, 255, 110)); 
                tip.setOutlineThickness(1.5f);
                tip.setPosition(posX + width / 2.0f - 5.0f, posY - 9.0f);
                win.draw(tip);
                if (wn == "Spread") 
                {
                    CircleShape lt(3.5f);
                    lt.setFillColor(tc);
                    lt.setPosition(posX + 3.0f, posY - 2.0f); 
                    win.draw(lt);
                    CircleShape rt(3.5f);
                    rt.setFillColor(tc);
                    rt.setPosition(posX + width - 7.0f, posY - 2.0f); 
                    win.draw(rt);
                }
                if (wn == "Piercing") 
                {
                    RectangleShape bar(Vector2f(3.0f, 12.0f)); 
                    bar.setFillColor(Color(200, 0, 255, 200));
                    bar.setPosition(posX + width / 2.0f - 1.5f, posY - 14.0f); 
                    win.draw(bar);
                }
            }
        }
    }
    void onCollision(GameObject*) override
    {}
};



//  ENEMY BASE



class Enemy : public Entity 
{
protected:
    float speedScale, fireScale;
public:
    Enemy(float px, float py, float vx, float vy, float h, float d, float ht, float wd, float ss = 1.0f, float fs = 1.0f)
        : Entity(px, py, vx, vy, h, d, ht, wd), speedScale(ss), fireScale(fs) {
    }
    bool isEnemy() override 
    {
        return true;
    }
    virtual void shoot(Bullet** b, int& cnt, int maxB)
    {}
    virtual ~Enemy() {}
};


//  DRONE


class Drone : public Enemy 
{
    Texture tex; Sprite spr; bool texOk; Clock fireClock; float fireInterval;
public:
    Drone(float px, float py, float ss = 1.0f, float fs = 1.0f)
        : Enemy(px, py, 0.0f, DRONE_BASE_SPD* ss, 25.0f, 20.0f, 54.0f, 54.0f, ss, fs) 
    {
        fireInterval = (1.5f + (rand() % 200) / 100.0f) / fs;
        if (fireInterval < 0.4f)
            fireInterval = 0.4f;
        texOk = loadTextureClean(tex, "drone.png");
        if (texOk)
        {
            spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y);
        }
    }
    void shoot(Bullet** b, int& cnt, int maxB) override 
    {
        if (fireClock.getElapsedTime().asSeconds() < fireInterval)
            return;
        fireClock.restart();
        if (cnt < maxB)
        {
            b[cnt++] = new Bullet(posX + width / 2.0f - 2.0f, posY + height, 15.0f, 1, "Enemy");
            if (gAudio)
                gAudio->playEnemyFire();
        }
    }
    void update(float dt) override 
    {
        posY += velY * dt;
        if (posY > WINDOW_H + 60) health = 0;
    }
    void draw(RenderWindow& win) override
    {
        if (texOk) 
        { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); 
        }
        else 
        { 
            RectangleShape r(Vector2f(width, height)); 
            r.setFillColor(Color(220, 60, 60));
            r.setPosition(posX, posY);
            win.draw(r);
        }
    }
    void onCollision(GameObject*) override
    {}
};


//  VIPER


class Viper : public Enemy 
{
    float sinT, baseX; Texture tex; Sprite spr; bool texOk; Clock fireClock; float fireInterval;
public:
    Viper(float px, float py, float ss = 1.0f, float fs = 1.0f)
        : Enemy(px, py, 0.0f, VIPER_BASE_SPD* ss, 50.0f, 20.0f, 56.0f, 56.0f, ss, fs), sinT(0.0f), baseX(px) 
    {
        fireInterval = (2.0f + (rand() % 150) / 100.0f) / fs;
        if (fireInterval < 0.6f)
            fireInterval = 0.6f;
        texOk = loadTextureClean(tex, "enemy.png");
        if (texOk)
        { 
            spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y);
        }
    }
    void shoot(Bullet** b, int& cnt, int maxB) override
    {
        if (fireClock.getElapsedTime().asSeconds() < fireInterval)
            return;
        fireClock.restart();
        if (cnt < maxB) 
        {
            b[cnt++] = new Bullet(posX + width / 2.0f - 2.0f, posY + height, 18.0f, 1, "Enemy"); 
        if (gAudio) gAudio->playEnemyFire();
        }
    }
    void update(float dt) override 
    {
        sinT += dt * 2.6f; posX = baseX + sinf(sinT) * 78.0f; posY += velY * dt;
        if (posX < 0)
            posX = 0;
        if (posX > WINDOW_W - width)
            posX = WINDOW_W - width;
        if (posY > WINDOW_H + 60)
            health = 0;
    }
    void draw(RenderWindow& win) override 
    {
        if (texOk) 
        {
            spr.setPosition(posX, posY);
            win.draw(spr, BlendAlpha); 
        }
        else 
        {
            RectangleShape r(Vector2f(width, height));
        
        r.setFillColor(Color(180, 60, 220));
        r.setPosition(posX, posY); win.draw(r);
        }
    }
    void onCollision(GameObject*) override {}
};


//  SEEKER


class Seeker : public Enemy 
{
    float lockedX; Texture tex; Sprite spr; bool texOk;
public:
    Seeker(float px, float py, float playerX, float ss = 1.0f)
        : Enemy(px, py, 0.0f, 55.0f * ss, 30.0f, 30.0f, 52.0f, 52.0f, ss, 1.0f), lockedX(playerX)
    {
        texOk = loadTextureClean(tex, "seeker.png");
        if (texOk) 
        { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y);
        }
    }
    void update(float dt) override
    {
        velY += SEEKER_ACCEL * speedScale * dt;
        if (velY > 650) 
            velY = 650;
        float diff = lockedX - posX;
        if (fabsf(diff) > 2.0f)
            posX += (diff > 0 ? 1.0f : -1.0f) * 110.0f * dt;
        posY += velY * dt;
        if (posY > WINDOW_H + 60)
            health = 0;
    }
    void draw(RenderWindow& win) override 
    {
        if (texOk) 
        { spr.setPosition(posX, posY);
        win.draw(spr, BlendAlpha); 
        }
        else
        {
            ConvexShape tri; tri.setPointCount(3); tri.setPoint(0, Vector2f(width / 2.0f, height)); tri.setPoint(1, Vector2f(0, 0));
            tri.setPoint(2, Vector2f(width, 0)); tri.setFillColor(Color(255, 80, 0));
            tri.setPosition(posX, posY); win.draw(tri); 
        }
    }
    void onCollision(GameObject*) override {}
};


//  BOSS BASE
//  : handleBulletHit now returns bool.
//        true  = damage dealt,  consume the bullet.
//        false = immune body,   do NOT consume the bullet.


class Boss : public Enemy {
protected:
    float maxHP; int phase; Clock phaseTimer, atkClock;
public:
    Boss(float px, float py, float hp, float d, float ht, float wd)
        : Enemy(px, py, 0, 0, hp, d, ht, wd), maxHP(hp), phase(1) 
    {
    }
    bool  isBoss()   override
    { 
        return true; 
    }
    bool  isEnemy()  override
    {
        return true;
    }
    int   getPhase() const 
    {
        return phase;
    }
    float getHPPct() const 
    {
        return (maxHP > 0) ? health / maxHP : 0;
    }
    float getMaxHP() const
    {
        return maxHP;
    }
    void checkPhase(float p2 = 0.66f, float p3 = 0.33f) 
    {
        int np = (health < maxHP * p3) ? 3 : (health < maxHP * p2) ? 2 : 1;
        if (np != phase) 
        {
            phase = np; 
        phaseTimer.restart();
        }
    }
    virtual void drawHPBar(RenderWindow& win)
    {
        float bw = 500, bx = WINDOW_W / 2 - bw / 2, by = 8;
        RectangleShape bg(Vector2f(bw, 22)); bg.setFillColor(Color(40, 0, 0)); bg.setOutlineColor(Color(80, 40, 80, 180)); bg.setOutlineThickness(1.5f); bg.setPosition(bx, by);
        win.draw(bg);
        Color fc = (phase == 1) ? Color(0, 210, 80) : (phase == 2) ? Color(255, 200, 0) : Color(255, 30, 0);
        float pct = getHPPct();
        if (pct < 0)
            pct = 0;
        RectangleShape fg(Vector2f(bw * pct, 22)); fg.setFillColor(fc); fg.setPosition(bx, by); win.draw(fg);
    }
    
    // returns true if damage was actually applied (bullet should be consumed)
    virtual bool handleBulletHit(Bullet* b) 
    {
        takeDamage(b->getBulletDmg());
        return true;
    }
    virtual void attackPattern(Bullet** b, int& cnt, int maxB, float plrX, float plrY) = 0;
    virtual bool  isLaserActive()  const 
    { 
        return false;
    }
    virtual float getLaserY()      const 
    { 
        return -1.0f;
    }
    virtual bool  isLaser2Active() const
    {
        return false; 
    }
    virtual bool  wantsSeeker() 
    {
        return false;
    }
    virtual int   seekerSpawnCount() 
    {
        return 1;
    }
    virtual ~Boss() {}
};


//  CRUISER  (Level 1 Boss)  — HP P-06: 1170

class Cruiser : public Boss
{
    float moveDir; bool entryDone; float entryTimer;
    bool atkWarn, atkActive; int atkGap;
    Clock atkWarnClock, atkActiveClock, atkCycleClock, atkStreamClock;
    Texture tex; Sprite spr; bool texOk;
public:
    
    Cruiser(float px, float py) :Boss(px, py, 1170, 30, 85, 200), moveDir(1), entryDone(false), entryTimer(0.0f), atkWarn(false), atkActive(false), atkGap(0) 
    {
        texOk = loadTextureClean(tex, "boss1.png");
        if (texOk)
        { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y);
        }
    }
    void update(float dt) override
    {
        if (!entryDone) 
        {
            entryTimer += dt;
        if (entryTimer >= 1.0f) 
            entryDone = true;
        return;
         }
        float spd = (phase == 1) ? 250.0f : (phase == 2) ? 310.0f : 380.0f;
        posX += moveDir * spd * dt;
        if (posX < 10)
            moveDir = 1;
        if (posX > WINDOW_W - width - 10) 
            moveDir = -1;
        if (atkWarn && atkWarnClock.getElapsedTime().asSeconds() >= 1.0f)
        {
            atkWarn = false; atkActive = true; atkActiveClock.restart(); atkStreamClock.restart(); 
        }
        if (atkActive && atkActiveClock.getElapsedTime().asSeconds() >= 2.5f) 
            atkActive = false;
        checkPhase(0.40f, 0.20f);
    }
    void attackPattern(Bullet** b, int& cnt, int maxB, float, float) override 
    {
        if (!entryDone)
            return;
        float lc = (phase == 1) ? 3.5f : (phase == 2) ? 2.5f : 1.8f;
        if (!atkWarn && !atkActive && atkCycleClock.getElapsedTime().asSeconds() >= lc)
        {
            atkWarn = true; atkGap = rand() % 10; atkWarnClock.restart(); atkCycleClock.restart();
        }
        if (atkActive && atkStreamClock.getElapsedTime().asSeconds() >= 0.18f) 
        {
            atkStreamClock.restart();
            float sw = WINDOW_W / 10.0f;
            for (int i = 0; i < 10; i++) 
            {
                if (i == atkGap || cnt >= maxB)
                    continue; 
                b[cnt++] = new Bullet(i * sw + sw / 2.0f - 2.0f, posY + height + 4.0f, 24, 1, "Enemy");
            }
            if (phase >= 3)
            {
                for (int i = 0; i < 10 && cnt < maxB; i++) 
                {
                    if (i == atkGap)
                        continue; 
                    b[cnt++] = new Bullet(i * sw + sw / 2.0f - 2.0f, posY + height + 26.0f, 24, 1, "Enemy");
                }
            }
            if (gAudio)
                gAudio->playEnemyFire();
        }
    }
    void draw(RenderWindow& win) override
    {
        // thin slot warning strips only (no solid full-height box)
        if (atkWarn) {
            float t = atkWarnClock.getElapsedTime().asSeconds();
            Uint8 a = (Uint8)((sinf(t * 16.0f) + 1.0f) * 0.5f * 220);
            float sw = WINDOW_W / 10.0f;
            for (int i = 0; i < 10; i++) {
                if (i == atkGap) continue;
                RectangleShape wm(Vector2f(sw - 6.0f, 5.0f)); // 5px thin strip
                wm.setFillColor(Color(255, 80, 0, a));
                wm.setPosition(i * sw + 3.0f, posY + height + 2.0f); win.draw(wm);
            }
        }
        if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
        else { RectangleShape body(Vector2f(width, height)); body.setFillColor(Color(160, 0, 200)); body.setPosition(posX, posY); win.draw(body); }
    }
    void onCollision(GameObject*) override {}
};

//  TURRET  (Composition member of TwinCannons)
//
//  P-04: overlaps() uses an 8-pixel tolerance border around the visual circle
//  so the hit area matches the rendered sprite closely without being oversized.
//  posX/posY = TOP-LEFT of the 32x32 visual circle bounding box.


class Turret {
    float posX, posY, health, maxHP;
    static const int VIS = 32;   // visual diameter
    static const int PAD = 8;    // extra tolerance pixels on each side
    Clock fireClock;
public:
    Turret(float px, float py, float hp) :posX(px), posY(py), health(hp), maxHP(hp) {}
    float getPosX()  const { return posX; }
    float getPosY()  const { return posY; }
    float getHP()    const { return health; }
    float getMaxHP() const { return maxHP; }
    bool  isAlive()  const { return health > 0; }
    void  setPos(float px, float py) { posX = px; posY = py; }
    void  takeDamage(float d) { health -= d; if (health < 0) health = 0; }

    void fire(Bullet** b, int& cnt, int maxB, float delay, float playerX) {
        if (!isAlive() || fireClock.getElapsedTime().asSeconds() < delay) 
            return;
        fireClock.restart(); 
        if (cnt >= maxB)
            return;
        float bx = posX + VIS / 2.0f - 2.0f, by_ = posY + VIS;
        Bullet* bl = new Bullet(bx, by_, 20, 1, "Enemy");
        float dx = playerX - bx, dy = 300.0f; float len = sqrtf(dx * dx + dy * dy);
        if (len > 0) 
        {
            dx /= len; dy /= len;
        }
        bl->setVelXY(dx * BULLET_ENEMY_SPD, fabsf(dy * BULLET_ENEMY_SPD) > 20 ? dy * BULLET_ENEMY_SPD : BULLET_ENEMY_SPD);
        b[cnt++] = bl;
        if (gAudio) 
            gAudio->playEnemyFire();
    }

    // tight AABB with tolerance border — matches visual, not oversized
    bool overlaps(float ex, float ey, float ew, float eh) const {
        float hx = posX - PAD, hy = posY - PAD;
        float hw = VIS + PAD * 2, hh = VIS + PAD * 2;
        return ex<hx + hw && ex + ew>hx && ey<hy + hh && ey + eh>hy;
    }

    void draw(RenderWindow& win)
    {
        float cx = posX + VIS / 2.0f, cy = posY + VIS / 2.0f, r = VIS / 2.0f;
        CircleShape tc(r);
        tc.setFillColor(isAlive() ? Color(0, 200, 220) : Color(70, 70, 70));
        tc.setOutlineColor(Color::White);
        tc.setOutlineThickness(2);
        tc.setPosition(cx - r, cy - r);
        win.draw(tc);
        RectangleShape hbg(Vector2f((float)VIS, 5)); 
        hbg.setFillColor(Color(80, 0, 0));
        hbg.setPosition(posX, posY - 9);
        win.draw(hbg);
        float hpct = health / maxHP;
        if (hpct < 0)
            hpct = 0;
        RectangleShape hfg(Vector2f(VIS * hpct, 5));
        hfg.setFillColor(Color(0, 220, 80));
        hfg.setPosition(posX, posY - 9); win.draw(hfg);
    }
};

//  TWIN CANNONS  (Level 2 Boss — COMPOSITION with Turret)
//
//  P-04: handleBulletHit uses PRECISE turret overlaps() only.
//        Bullets hitting the immune body between/around turrets are NOT
//        consumed and deal NO damage (passes through without effect).
//        Returns false on immune-body hit so CollisionManager skips consumeHit.
//
//  P-07: Core HP 455, Turret HP 455 each

class TwinCannons : public Boss 
{
    Turret* leftTurret;
    Turret* rightTurret;
    float   sinPhase, moveDir;
    bool    coreVulnerable;
    float   coreHP, coreMaxHP;
    Texture tex; Sprite spr; bool texOk;
    float   lastPlayerX;

    static constexpr float CORE_HP_MAX = 455.0f;
public:
    TwinCannons(float px, float py)
        : Boss(px, py, 1.0f, 25, 100, 220), sinPhase(0), moveDir(1),
        coreVulnerable(false), coreHP(CORE_HP_MAX), coreMaxHP(CORE_HP_MAX), lastPlayerX(WINDOW_W / 2) 
    {
        // Turret top-left at (px+24, py+56) and (px+164, py+56); visual 32x32
        leftTurret = new Turret(px + 24, py + 56, 455.0f); 
        rightTurret = new Turret(px + 164, py + 56, 455.0f); 
        texOk = loadTextureClean(tex, "boss2.png");
        if (texOk)
        { spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y); 
        }
    }
    ~TwinCannons() override
    {
        delete leftTurret; delete rightTurret;
    }

    bool isDead() const 
    {
        return coreVulnerable && coreHP <= 0;
    }

    void update(float dt) override
    {
        float spd = (phase == 1) ? 160.0f : (phase == 2) ? 210.0f : 270.0f;
        posX += moveDir * spd * dt;
        if (posX < 15)
            moveDir = 1;
        if (posX > WINDOW_W - width - 15)
            moveDir = -1;
        sinPhase += dt * 0.9f; posY = 50.0f + sinf(sinPhase) * 22.0f;
        leftTurret->setPos(posX + 24, posY + 56);
        rightTurret->setPos(posX + 164, posY + 56);
        bool wasVuln = coreVulnerable;
        coreVulnerable = (!leftTurret->isAlive() && !rightTurret->isAlive());
        if (!wasVuln && coreVulnerable)
        {
            health = coreHP; maxHP = coreMaxHP;
        }
        if (coreVulnerable)
        {
            int np = (coreHP < coreMaxHP * 0.33f) ? 3 : (coreHP < coreMaxHP * 0.66f) ? 2 : 1;
            if (np != phase)
            { 
                phase = np; phaseTimer.restart(); 
            }
        }
    }
    void attackPattern(Bullet** b, int& cnt, int maxB, float plrX, float) override 
    {
        lastPlayerX = plrX;
        float delay = (phase == 1) ? 1.1f : (phase == 2) ? 0.75f : 0.50f;
        if (leftTurret->isAlive()) 
            leftTurret->fire(b, cnt, maxB, delay, plrX);
        if (rightTurret->isAlive()) 
            rightTurret->fire(b, cnt, maxB, delay, plrX);
        if (!leftTurret->isAlive() && rightTurret->isAlive()) 
            rightTurret->fire(b, cnt, maxB, delay * 0.45f, plrX);
        if (!rightTurret->isAlive() && leftTurret->isAlive()) 
            leftTurret->fire(b, cnt, maxB, delay * 0.45f, plrX);
    }

    // Precise turret AABB. Immune body returns false (no consumeHit).
    bool handleBulletHit(Bullet* b) override {
        float bx = b->getPosX(), by_ = b->getPosY();
        float bw = b->getWidth(), bh = b->getHeight();
        float dmg = b->getBulletDmg();
        if (leftTurret->isAlive() && leftTurret->overlaps(bx, by_, bw, bh)) {
            leftTurret->takeDamage(dmg * 1.5f); return true;
        }
        if (rightTurret->isAlive() && rightTurret->overlaps(bx, by_, bw, bh)) {
            rightTurret->takeDamage(dmg * 1.5f); return true;
        }
        if (coreVulnerable) { coreHP -= dmg; if (coreHP < 0) coreHP = 0; health = coreHP; return true; }
        // Immune body hit — bullet is NOT consumed, deals no damage
        return false;
    }

    void drawHPBar(RenderWindow& win) override
    {
        float bw = 500, bx = WINDOW_W / 2 - bw / 2, by = 8;
        RectangleShape bg(Vector2f(bw, 22)); bg.setFillColor(Color(40, 0, 0)); bg.setOutlineColor(Color(60, 40, 80, 180)); bg.setOutlineThickness(1.5f); bg.setPosition(bx, by); win.draw(bg);
        float pct; Color fc;
        if (!coreVulnerable)
        {
            float comb = leftTurret->getHP() + rightTurret->getHP(); float maxC = leftTurret->getMaxHP() + rightTurret->getMaxHP(); pct = (maxC > 0) ? comb / maxC : 0; fc = Color(0, 210, 200);
        }
        else 
        {
            pct = (coreMaxHP > 0) ? coreHP / coreMaxHP : 0; fc = (phase == 1) ? Color(0, 210, 80) : (phase == 2) ? Color(255, 200, 0) : Color(255, 30, 0);
        }
        if (pct < 0) 
            pct = 0;
        RectangleShape fg(Vector2f(bw * pct, 22)); fg.setFillColor(fc); fg.setPosition(bx, by); win.draw(fg);
    }

    void draw(RenderWindow& win) override 
    {
        if (texOk) 
        {
            spr.setPosition(posX, posY); win.draw(spr, BlendAlpha);
        }
        else 
        {
            Color bc = coreVulnerable ? Color(255, 80, 0) : Color(0, 150, 200);
            RectangleShape body(Vector2f(width, height)); body.setFillColor(bc); body.setPosition(posX, posY); win.draw(body);
            if (!coreVulnerable) 
            {
                RectangleShape sh(Vector2f(width, height)); sh.setFillColor(Color(0, 100, 255, 55)); sh.setOutlineColor(Color(0, 200, 255)); sh.setOutlineThickness(3); sh.setPosition(posX, posY); win.draw(sh); 
            }
        }
        leftTurret->draw(win); rightTurret->draw(win);
    }
    void onCollision(GameObject*) override 
    {}
};

//  MOTHERSHIP  (Level 3 Boss)  — HP P-06: 1170



class Mothership : public Boss {
    float moveDir;
    bool  laserWarn, laserActive; float fixedLaserX, fixedLaserY; Clock laserWarnClock, laserActiveClock, laserCycleClock; float laserDmgTimer;
    bool  laser2Warn, laser2Active; float fixedLaser2X; Clock laser2WarnClock, laser2ActiveClock, laser2CycleClock; float laser2DmgTimer;
    bool  seekerPhaseActive; Clock seekerPhaseTimer, seekerSpawnTimer; Clock seekerClock; int pendingSeekers;
    Texture tex; Sprite spr; bool texOk;
    float laserW()  const
    {
        return WINDOW_W * 0.60f;
    }
    float laser2W() const 
    {
        return WINDOW_W * 0.50f;
    }
    float laserBaseX()  const 
    {
        return WINDOW_W / 2.0f - laserW() / 2.0f;
    }
    float laser2BaseX() const 
    {
        return WINDOW_W / 2.0f - laser2W() / 2.0f;
    }
public:
    Mothership(float px, float py)
        : Boss(px, py, 1170, 40, 110, 260), moveDir(1),
        laserWarn(false), laserActive(false), fixedLaserX(laserBaseX()), fixedLaserY(WINDOW_H / 2), laserDmgTimer(0),
        laser2Warn(false), laser2Active(false), fixedLaser2X(laser2BaseX()), laser2DmgTimer(0),
        seekerPhaseActive(false), pendingSeekers(0) 
    {
        texOk = loadTextureClean(tex, "boss3.png");
        if (texOk)
        { 
            spr.setTexture(tex); spr.setScale(width / tex.getSize().x, height / tex.getSize().y);
        }
    }
    bool  isLaserActive()  const override 
    {
        return laserActive; 
    }
    float getLaserY()      const override 
    {
        return fixedLaserY;
    }
    bool  isLaser2Active() const override 
    {
        return laser2Active; 
    }
    bool  isWarning()      const 
    { 
        return laserWarn || laser2Warn; 
    }
    float getLaserX()    const 
    { 
        return fixedLaserX; 
    }
    float getLaserWd()   const 
    {
        return laserW();
    }
    float getLaser2X()   const 
    { 
        return fixedLaser2X;
    }
    float getLaser2Wd()  const 
    {
        
        return laser2W();
    }
    bool wantsSeeker() override 
    { 
        if (pendingSeekers > 0)
        {
            pendingSeekers--;
            return true; 
        }
        return false;
    }
    int  seekerSpawnCount() override { 
        return (phase >= 3) ? 2 : 1;
    }
    void tickLaserDamage(float dt) {
        laserDmgTimer += dt; laser2DmgTimer += dt;
    }
    bool consumeLaserDamage() { 
        if (laserDmgTimer >= 0.3f) 
        {
            laserDmgTimer = 0;
            return true; }
        return false; 
    }
    bool consumeLaser2Damage() { 
        if (laser2DmgTimer >= 0.3f)
        {
            laser2DmgTimer = 0;
            return true; }
        return false; }

    void update(float dt) override {
        posX += moveDir * 75 * dt;
        if (posX < 20) moveDir = 1;
        if (posX > WINDOW_W - width - 20)
            moveDir = -1;
        if (seekerPhaseActive) 
        {
            if (seekerPhaseTimer.getElapsedTime().asSeconds() >= 5.0f)
            {
                seekerPhaseActive = false; laserCycleClock.restart(); 
            }
            else if (seekerSpawnTimer.getElapsedTime().asSeconds() >= 1.4f)
            {
                seekerSpawnTimer.restart(); pendingSeekers += (phase >= 3) ? 2 : 1; 
            }
        }
        if (laserWarn && laserWarnClock.getElapsedTime().asSeconds() >= 1.0f) 
        {
            laserWarn = false; laserActive = true; fixedLaserY = posY + height; fixedLaserX = laserBaseX(); laserActiveClock.restart(); laserDmgTimer = 0;
        }
        if (laserActive && laserActiveClock.getElapsedTime().asSeconds() >= 7.0f) 
        
        {
            laserActive = false; 
        
        if (!seekerPhaseActive) 
        {
            seekerPhaseActive = true; seekerPhaseTimer.restart()
                ; seekerSpawnTimer.restart(); } 
        }
        if (phase >= 2)
        {
            if (laser2Warn && laser2WarnClock.getElapsedTime().asSeconds() >= 1.0f) 
            { laser2Warn = false; laser2Active = true; laser2ActiveClock.restart(); laser2DmgTimer = 0;
            }
            if (laser2Active && laser2ActiveClock.getElapsedTime().asSeconds() >= 5.0f)
            { laser2Active = false; pendingSeekers += 1; laser2CycleClock.restart();
            }
        }
        float si = (phase == 1) ? 9.0f : (phase == 2) ? 6.5f : 4.0f;
        if (seekerClock.getElapsedTime().asSeconds() >= si) 
        {
            pendingSeekers += 1; seekerClock.restart();
        }
        checkPhase();
    }
    void attackPattern(Bullet** b, int& cnt, int maxB, float, float) override {
        float lc = (phase == 1) ? 8.0f : (phase == 2) ? 5.5f : 3.5f;
        if (!laserWarn && !laserActive && !seekerPhaseActive && laserCycleClock.getElapsedTime().asSeconds() >= lc)
        {
            laserWarn = true; fixedLaserX = laserBaseX(); laserWarnClock.restart(); laserCycleClock.restart();
        }
        if (phase >= 2) 
        {
            float lc2 = lc + 3.0f;
            if
                (!laser2Warn && !laser2Active && laser2CycleClock.getElapsedTime().asSeconds() >= lc2) 
            { laser2Warn = true; fixedLaser2X = laser2BaseX(); laser2WarnClock.restart(); laser2CycleClock.restart();
            }
        }
    }
    void drawHPBar(RenderWindow& win) override 
    {
        float bw = 620, bx = WINDOW_W / 2 - bw / 2, by = 8;
        RectangleShape bg(Vector2f(bw, 24)); bg.setFillColor(Color(20, 0, 0)); bg.setOutlineColor(Color(120, 0, 180, 180)); bg.setOutlineThickness(2); bg.setPosition(bx, by); win.draw(bg);
        Color fc = (phase == 1) ? Color(80, 255, 80) : (phase == 2) ? Color(255, 200, 0) : Color(255, 30, 0);
        float pct = getHPPct(); if (pct < 0) pct = 0;
        RectangleShape fg(Vector2f(bw * pct, 24)); fg.setFillColor(fc); fg.setPosition(bx, by); win.draw(fg);
    }
    // P-08: thin warning strip only — no ugly solid full-height rectangle
    void draw(RenderWindow& win) override {
        float lx = fixedLaserX, lw = laserW();
        float bottomY = posY + height;
        if (laserWarn) {
            float t = laserWarnClock.getElapsedTime().asSeconds();
            Uint8 a = (Uint8)((sinf(t * 18.0f) + 1.0f) * 0.5f * 255);
            // 6px thin warning strip only (P-08)
            RectangleShape ws(Vector2f(lw, 6.0f)); ws.setFillColor(Color(255, 60, 0, a)); ws.setPosition(lx, bottomY); win.draw(ws);
            // Faint safe-edge hint strips
            if (lx > 4) { RectangleShape le(Vector2f(lx, 4.0f)); le.setFillColor(Color(0, 255, 80, (Uint8)(a / 5))); le.setPosition(0, bottomY); win.draw(le); }
            float re = lx + lw; if (re < WINDOW_W - 4) { RectangleShape rs(Vector2f(WINDOW_W - re, 4.0f)); rs.setFillColor(Color(0, 255, 80, (Uint8)(a / 5))); rs.setPosition(re, bottomY); win.draw(rs); }
        }
        if (laserActive) {
            float beamH = WINDOW_H - bottomY; if (beamH > 0) {
                float t = laserActiveClock.getElapsedTime().asSeconds();
                Uint8 al = (Uint8)(200 + sinf(t * 30.0f) * 30);
                RectangleShape beam(Vector2f(lw, beamH)); beam.setFillColor(Color(255, 30, 30, al)); beam.setOutlineColor(Color(255, 180, 0)); beam.setOutlineThickness(2); beam.setPosition(lx, bottomY); win.draw(beam);
                RectangleShape core(Vector2f(lw * 0.18f, beamH)); core.setFillColor(Color(255, 255, 220, (Uint8)(180 + sinf(t * 40.0f) * 40))); core.setPosition(lx + lw * 0.41f, bottomY); win.draw(core);
                if (lx > 0)
                { 
                    RectangleShape le(Vector2f(lx, beamH)); le.setFillColor(Color(0, 255, 80, 30)); le.setPosition(0, bottomY); win.draw(le); 
                }
                if (lx + lw < WINDOW_W) 
                
                {
                    RectangleShape rs(Vector2f(WINDOW_W - (lx + lw), beamH)); rs.setFillColor(Color(0, 255, 80, 30)); rs.setPosition(lx + lw, bottomY); win.draw(rs); 
                }
            }
        }
        if (phase >= 2)
        {
            float lx2 = fixedLaser2X, lw2 = laser2W();
            if (laser2Warn) 
            { float t = laser2WarnClock.getElapsedTime().asSeconds(); Uint8 a = (Uint8)((sinf(t * 18.0f) + 1.0f) * 0.5f * 200); RectangleShape ws2(Vector2f(lw2, 6.0f)); ws2.setFillColor(Color(255, 130, 0, a)); ws2.setPosition(lx2, bottomY); win.draw(ws2); }
            if (laser2Active) 
            { float beamH = WINDOW_H - bottomY; if (beamH > 0) { float t2 = laser2ActiveClock.getElapsedTime().asSeconds(); Uint8 a2 = (Uint8)(190 + sinf(t2 * 30.0f) * 30); RectangleShape beam2(Vector2f(lw2, beamH)); beam2.setFillColor(Color(255, 100, 0, a2)); beam2.setOutlineColor(Color(255, 220, 0)); beam2.setOutlineThickness(2); beam2.setPosition(lx2, bottomY); win.draw(beam2); } }
        }
        if (texOk) { spr.setPosition(posX, posY); win.draw(spr, BlendAlpha); }
        else { RectangleShape body(Vector2f(width, height)); body.setFillColor(Color(30, 30, 70)); body.setOutlineColor(Color(80, 80, 255)); body.setOutlineThickness(3); body.setPosition(posX, posY); win.draw(body); CircleShape core(28); core.setFillColor(Color(180, 0, 255)); core.setPosition(posX + width / 2 - 28, posY + height / 2 - 28); win.draw(core); }
    }
    void onCollision(GameObject*) override {}
};

//  COLLISION MANAGER
//  P-05: Only call consumeHit() when handleBulletHit returns true.


class CollisionManager {
    bool aabb(Entity* a, Entity* b) {
        return (a->getPosX() < b->getPosX() + b->getWidth() && a->getPosY() < b->getPosY() + b->getHeight() && a->getPosX() + a->getWidth() > b->getPosX() && a->getPosY() + a->getHeight() > b->getPosY());
    }
public:
    void checkAll(Player* player, Bullet** bullets, int bulletCnt, Enemy** enemies, int enemyCnt, Asteroid** asteroids, int asteroidCnt, PowerUp** powerups, int powerUpCnt, Boss* boss) {
        for (int i = 0; i < bulletCnt; i++) {
            Bullet* blt = bullets[i]; if (!blt || blt->getHealth() <= 0) continue;
            if (blt->isPlayerBullet()) {
                for (int j = 0; j < enemyCnt && blt->getHealth()>0; j++) { Enemy* en = enemies[j]; if (!en || en->getHealth() <= 0) continue; if (aabb(blt, en)) { en->takeDamage(blt->getBulletDmg()); blt->consumeHit(); } }
                // P-05: only consume bullet if actual damage was applied
                if (boss && boss->getHealth() > 0 && blt->getHealth() > 0 && aabb(blt, boss)) 
                {
                    if (boss->handleBulletHit(blt)) 
                        blt->consumeHit();
                }
            }
            if (blt->getHealth() > 0) 
                for (int k = 0; k < asteroidCnt; k++) 
                {
                    if (asteroids[k] && aabb(blt, asteroids[k])) { blt->setHealth(0);
                    break; } 
                }
            if (blt->isEnemyBullet() && blt->getHealth() > 0 && player && !player->isInvincible() && aabb(blt, player)) 
            {
                player->takeDamageFromEnemy(); blt->setHealth(0);
            }
        }
        if (!player) return;
        for (int i = 0; i < enemyCnt; i++) 
        { Enemy* en = enemies[i]; if (!en || en->getHealth() <= 0)
            continue;
        if (!player->isInvincible() && aabb(player, en)) 
        { 
            player->takeDamageFromEnemy(); en->setHealth(0); 
        }
        }
        if (boss && boss->getHealth() > 0 && !player->isInvincible() && aabb(player, boss)) player->takeDamageFromEnemy();
        for (int i = 0; i < asteroidCnt; i++) { if (asteroids[i] && !player->isInvincible() && aabb(player, asteroids[i])) { player->takeDamageFromEnemy(); break; } }
        for (int i = 0; i < powerUpCnt; i++) { PowerUp* pu = powerups[i]; if (!pu || pu->getHealth() <= 0) continue; if (aabb(player, pu)) { int t = pu->getType(); if (t == 0) player->equipWeapon(new SpreadModule()); else if (t == 1) player->equipWeapon(new PiercingModule()); else if (t == 2) player->activateShield(); else if (t == 3) player->addEMP(); if (gAudio) gAudio->playPowerup(); pu->setHealth(0); } }
    }
    int removeDeadObjects(Player* player, Enemy** enemies, int& enemyCnt, Bullet** bullets, int& bulletCnt, Asteroid** asteroids, int& asteroidCnt, PowerUp** powerups, int& powerUpCnt, ExplosionEffect& fx) {
        int kills = 0, alive = 0;
        for (int i = 0; i < enemyCnt; i++) { if (enemies[i] && enemies[i]->getHealth() > 0) { enemies[alive++] = enemies[i]; } else if (enemies[i]) { if (player) { player->addScore(100); player->registerKill(); }kills++; fx.spawn(enemies[i]->getPosX() + enemies[i]->getWidth() / 2, enemies[i]->getPosY() + enemies[i]->getHeight() / 2, Color(255, 140, 0)); if (gAudio) gAudio->playEnemyExp(); if (powerUpCnt < MAX_POWERUPS) { int roll = rand() % 100; float dx = enemies[i]->getPosX() + enemies[i]->getWidth() / 2 - 18, dy = enemies[i]->getPosY(); if (roll < 15) powerups[powerUpCnt++] = new PowerUp(dx, dy, rand() % 3); else if (roll < 20) powerups[powerUpCnt++] = new PowerUp(dx, dy, 3); }delete enemies[i]; enemies[i] = nullptr; } }
        enemyCnt = alive; alive = 0;
        for (int i = 0; i < bulletCnt; i++) { if (bullets[i] && bullets[i]->getHealth() > 0) bullets[alive++] = bullets[i]; else if (bullets[i]) { delete bullets[i]; bullets[i] = nullptr; } }
        bulletCnt = alive;
        for (int i = 0; i < asteroidCnt; i++) if (asteroids[i] && asteroids[i]->getHealth() <= 0) asteroids[i]->respawn();
        alive = 0;
        for (int i = 0; i < powerUpCnt; i++) { if (powerups[i] && powerups[i]->getHealth() > 0) powerups[alive++] = powerups[i]; else if (powerups[i]) { delete powerups[i]; powerups[i] = nullptr; } }
        powerUpCnt = alive; return kills;
    }
};

// ============================================================================
//  GAME CLASS
// ============================================================================
class Game {
    RenderWindow    window;
    Font            font; bool fontOk;
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

    int  arcadeLevel; bool bossWaiting, bossSpawned;
    int  wave, waveTotal, waveSpawned; float waveSpeedScale, waveFireScale;
    bool waveClearFlag; float waveClearTimer;
    bool empFlashOn; Clock empFlashClock; bool empShake;
    float shakeAmt; Clock shakeClock;
    Clock deltaClock, spawnClock, shootClock;
    int menuSel; // 0=Arcade 1=Survival 2=Instructions 3=Controls 4=Exit

    void initArrays() { bulletCnt = enemyCnt = asteroidCnt = powerUpCnt = 0; boss = nullptr; for (int i = 0; i < MAX_BULLETS; i++)bullets[i] = nullptr; for (int i = 0; i < MAX_ENEMIES; i++)enemies[i] = nullptr; for (int i = 0; i < MAX_ASTEROIDS; i++)asteroids[i] = nullptr; for (int i = 0; i < MAX_POWERUPS; i++)powerups[i] = nullptr; }
    void clearAll() { for (int i = 0; i < bulletCnt; i++) { delete bullets[i]; bullets[i] = nullptr; }for (int i = 0; i < enemyCnt; i++) { delete enemies[i]; enemies[i] = nullptr; }for (int i = 0; i < asteroidCnt; i++) { delete asteroids[i]; asteroids[i] = nullptr; }for (int i = 0; i < powerUpCnt; i++) { delete powerups[i]; powerups[i] = nullptr; }if (boss) { delete boss; boss = nullptr; }bulletCnt = enemyCnt = asteroidCnt = powerUpCnt = 0; }
    void spawnAsteroidField() { int n = MIN_ASTEROIDS + rand() % (FIELD_ASTEROIDS - MIN_ASTEROIDS + 1); for (int i = 0; i < n && asteroidCnt < MAX_ASTEROIDS; i++)asteroids[asteroidCnt++] = new Asteroid(30 + (float)(rand() % (int)(WINDOW_W - 80)), (float)(rand() % (int)(WINDOW_H - 150))); }
    void ensureAsteroids() { while (asteroidCnt < MIN_ASTEROIDS && asteroidCnt < MAX_ASTEROIDS)asteroids[asteroidCnt++] = new Asteroid(30 + (float)(rand() % (int)(WINDOW_W - 80)), -80); }

    void inputGameplay(float) {
        float mx = 0, my = 0;
        if (Keyboard::isKeyPressed(Keyboard::Left))  mx = -PLAYER_SPD;
        if (Keyboard::isKeyPressed(Keyboard::Right)) mx = PLAYER_SPD;
        if (Keyboard::isKeyPressed(Keyboard::Up))    my = -PLAYER_SPD;
        if (Keyboard::isKeyPressed(Keyboard::Down))  my = PLAYER_SPD;
        player->setVelocity(mx, my);
        if (Keyboard::isKeyPressed(Keyboard::E) && (mx != 0 || my != 0)) { float dx = (mx > 0) ? 1 : (mx < 0 ? -1 : 0), dy = (my > 0) ? 1 : (my < 0 ? -1 : 0); player->dash(dx, dy); }
        if (Keyboard::isKeyPressed(Keyboard::Space) && shootClock.getElapsedTime().asSeconds() >= 0.22f) { player->fireWeapon(bullets, bulletCnt, MAX_BULLETS); shootClock.restart(); }
        static bool nWas = false; bool nNow = Keyboard::isKeyPressed(Keyboard::N);
        if (nNow && !nWas && player->hasEMP()) activateEMP();
        nWas = nNow;
    }
    void activateEMP() {
        player->consumeEMP();
        for (int i = 0; i < enemyCnt; i++) if (enemies[i]) enemies[i]->setHealth(0);
        if (boss) boss->takeDamage(boss->getMaxHP() * 0.12f);
        empFlashOn = true; empFlashClock.restart(); shakeAmt = 18.0f; shakeClock.restart(); empShake = true;
        if (gAudio) gAudio->playEMP();
    }
    void startArcade() {
        clearAll(); delete player; player = new Player(3, WINDOW_W / 2 - 31, WINDOW_H - 120, font);
        arcadeLevel = 1; bossWaiting = false; bossSpawned = false; spawnAsteroidField(); spawnClock.restart();
        if (gAudio) gAudio->playGameplayMusic();
        state = STATE_ARCADE; prevPlayState = STATE_ARCADE;
    }
    void spawnArcadeEnemy() {
        if (bossWaiting || bossSpawned || enemyCnt >= MAX_ENEMIES) return;
        if (spawnClock.getElapsedTime().asSeconds() < 1.7f) return;
        spawnClock.restart();
        float ss = 1.0f + (arcadeLevel - 1) * 0.25f; float sx = 20 + (float)(rand() % (int)(WINDOW_W - 60)); int roll = rand() % 10;
        if (arcadeLevel == 1)       enemies[enemyCnt++] = new Drone(sx, -40, ss);
        else if (arcadeLevel == 2) { if (roll < 5) enemies[enemyCnt++] = new Viper(sx, -40, ss); else enemies[enemyCnt++] = new Drone(sx, -40, ss); }
        else { if (roll < 3) enemies[enemyCnt++] = new Seeker(sx, -40, player->getPosX(), ss); else if (roll < 6) enemies[enemyCnt++] = new Viper(sx, -40, ss); else enemies[enemyCnt++] = new Drone(sx, -40, ss); }
    }
    int bossScoreThreshold() const { if (arcadeLevel == 1) return ARCADE_BOSS1_SCORE; if (arcadeLevel == 2) return ARCADE_BOSS2_SCORE; return ARCADE_BOSS3_SCORE; }
    void trySpawnBoss() {
        if (bossSpawned) return;
        if (!bossWaiting && player->getScore() >= bossScoreThreshold()) bossWaiting = true;
        if (bossWaiting && enemyCnt == 0) {
            int ab = 0; for (int i = 0; i < bulletCnt; i++) { if (bullets[i] && bullets[i]->isPlayerBullet()) bullets[ab++] = bullets[i]; else if (bullets[i]) { delete bullets[i]; bullets[i] = nullptr; } }bulletCnt = ab;
            bossSpawned = true; bossWaiting = false;
            float bx = WINDOW_W / 2 - 110;
            if (arcadeLevel == 1) boss = new Cruiser(bx, 30);
            else if (arcadeLevel == 2) boss = new TwinCannons(bx - 10, 40);
            else boss = new Mothership(bx - 30, 20);
            if (gAudio) gAudio->playBossEntry();
        }
    }
    void updateArcade(float dt) {
        if (!bossSpawned) { spawnArcadeEnemy(); trySpawnBoss(); }
        if (boss) {
            boss->attackPattern(bullets, bulletCnt, MAX_BULLETS, player->getPosX(), player->getPosY());
            boss->update(dt);
            while (boss->wantsSeeker() && enemyCnt < MAX_ENEMIES) 
            { int sc = boss->seekerSpawnCount(); 
            for (int s = 0; s < sc && enemyCnt < MAX_ENEMIES; s++)
                enemies[enemyCnt++] = new Seeker(20 + (float)(rand() % (int)(WINDOW_W - 60)), -40, player->getPosX(), 1.3f);
            break;
            }
            Mothership* ms = dynamic_cast<Mothership*>(boss);
            if (ms) {
                ms->tickLaserDamage(dt);
                auto checkLaser = [&](bool active, float lx, float lw, bool(Mothership::* consume)()) {
                    if (!active || player->isInvincible()) 
                        return;
                    float barH = WINDOW_H - ms->getLaserY();
                    bool xo = (player->getPosX() + player->getWidth() > lx && player->getPosX() < lx + lw);
                    bool yo = (player->getPosY() + player->getHeight() > ms->getLaserY() && player->getPosY() < ms->getLaserY() + barH);
                    if (xo && yo && (ms->*consume)()) player->takeDamageFromEnemy();
                    };
                checkLaser(ms->isLaserActive(), ms->getLaserX(), ms->getLaserWd(), &Mothership::consumeLaserDamage);
                checkLaser(ms->isLaser2Active(), ms->getLaser2X(), ms->getLaser2Wd(), &Mothership::consumeLaser2Damage);
            }
            TwinCannons* tc = dynamic_cast<TwinCannons*>(boss);
            bool killed = tc ? tc->isDead() : (boss->getHealth() <= 0);
            if (killed) 
            {
                explosions.spawnLarge(boss->getPosX() + boss->getWidth() / 2, boss->getPosY() + boss->getHeight() / 2, Color(255, 200, 0));
                if (gAudio) 
                {
                    gAudio->playBossExp();
                    gAudio->playGameplayMusic();
                }
                player->addScore(1000 * arcadeLevel); 
                delete boss;
                boss = nullptr;
                if (arcadeLevel >= 3)
                {
                    state = STATE_WIN;
                    return;
                }
                arcadeLevel++; bossSpawned = false; bossWaiting = false; spawnClock.restart();
            }
        }
    }
    void startSurvival()
    {
        clearAll();
        delete player;
        player = new Player(3, WINDOW_W / 2 - 31, WINDOW_H - 120, font);
        wave = 1; waveTotal = 5; waveSpawned = 0; waveSpeedScale = 1.0f; waveFireScale = 1.0f;
        waveClearFlag = false; waveClearTimer = 0; spawnAsteroidField(); spawnClock.restart();
        if (gAudio)
            gAudio->playGameplayMusic();
        state = STATE_SURVIVAL; prevPlayState = STATE_SURVIVAL;
    }
    void spawnSurvivalEnemy() 
    {
        if (waveClearFlag || waveSpawned >= waveTotal || enemyCnt >= MAX_ENEMIES) 
            return;
        if (spawnClock.getElapsedTime().asSeconds() < 1.4f)
            return;
        spawnClock.restart();
        float sx = 20 + (float)(rand() % (int)(WINDOW_W - 60)); int roll = rand() % 100;
        if (wave >= 8 && roll < 25)
            enemies[enemyCnt++] = new Seeker(sx, -40, player->getPosX(), waveSpeedScale);
        else if 
            (wave >= 5 && roll < 45) 
            enemies[enemyCnt++] = new Viper(sx, -40, waveSpeedScale, waveFireScale);
        else
            enemies[enemyCnt++] = new Drone(sx, -40, waveSpeedScale, waveFireScale);
        waveSpawned++;
    }
    void updateSurvival(float dt)
    {
        if (waveClearFlag)
        {
            waveClearTimer += dt;
            if (waveClearTimer >= 2.0f)
            {
                wave++; waveTotal = 5 + (wave - 1) * 2; waveSpawned = 0; waveSpeedScale = 1.0f + (wave - 1) * 0.05f; waveFireScale = 1.0f + (wave - 1) * 0.10f;
                if (waveSpeedScale > 2.5f)
                    waveSpeedScale = 2.5f; 
                if (waveFireScale > 3.5f)
                    waveFireScale = 3.5f; waveClearFlag = false; waveClearTimer = 0; spawnClock.restart();
            }
        }
        else { spawnSurvivalEnemy(); if (waveSpawned >= waveTotal && enemyCnt == 0) { waveClearFlag = true; waveClearTimer = 0; player->addScore(500 * wave); } }
    }
    void updateGameplay(float dt) {
        player->update(dt); explosions.update(dt);
        for (int i = 0; i < bulletCnt; i++) 
            if (bullets[i]) 
                bullets[i]->update(dt);
        for (int i = 0; i < enemyCnt; i++)
            if (enemies[i])
            { enemies[i]->update(dt); 
        enemies[i]->shoot(bullets, bulletCnt, MAX_BULLETS);
            }
        for (int i = 0; i < asteroidCnt; i++)
            if (asteroids[i]) 
                asteroids[i]->update(dt);
        for (int i = 0; i < powerUpCnt; i++) 
            if (powerups[i])
                powerups[i]->update(dt);
        colMgr.checkAll(player, bullets, bulletCnt, enemies, enemyCnt, asteroids, asteroidCnt, powerups, powerUpCnt, boss);
        if (player->popShieldBroke())
            explosions.spawn(player->getPosX() + player->getWidth() / 2, player->getPosY() + player->getHeight() / 2, Color(0, 200, 255));
        if (player->popTookDamage() && !empShake) 
        {
            shakeAmt = 9.0f; shakeClock.restart(); }
        colMgr.removeDeadObjects(player, enemies, enemyCnt, bullets, bulletCnt, asteroids, asteroidCnt, powerups, powerUpCnt, explosions);
        ensureAsteroids();
        if (state == STATE_ARCADE)  
            updateArcade(dt);
        if (state == STATE_SURVIVAL)
            updateSurvival(dt);
        if (player && !player->isAlive())
        { state = STATE_GAMEOVER; 
        if (gAudio) gAudio->stopAll();
        }
    }

    //  P-10: Clean dedicated boss info panel — no text overlap

    void drawBossInfo() {
        if (!boss)
            return;
        string bname; Color bnameCol;
        Mothership* ms = dynamic_cast<Mothership*>(boss);
        TwinCannons* tc = dynamic_cast<TwinCannons*>(boss);
        Cruiser* cr = dynamic_cast<Cruiser*>(boss);
        if (ms) 
        {
            bname = "MOTHERSHIP";   bnameCol = Color(200, 0, 255);
        }
        else if (tc)
        {
            bname = "TWIN CANNONS"; bnameCol = Color(0, 200, 220);
        }
        else if (cr) 
        {
            bname = "THE CRUISER";  bnameCol = Color(200, 80, 255);
        }
        else 
        {
            bname = "BOSS";        bnameCol = Color(255, 255, 255);
        }

        float panW = ms ? 660.0f : 540.0f, panX = WINDOW_W / 2 - panW / 2;
        RectangleShape panel(Vector2f(panW, 52));
        panel.setFillColor(Color(6, 3, 18, 200));
        panel.setOutlineColor(Color(55, 35, 90, 160)); panel.setOutlineThickness(1.5f);
        panel.setPosition(panX, 4); window.draw(panel);

        boss->drawHPBar(window); // HP bar at y=8

        if (fontOk) {
            Text nt(bname, font, 15); nt.setFillColor(bnameCol);
            nt.setPosition(WINDOW_W / 2 - nt.getLocalBounds().width / 2, 33); window.draw(nt);
        }
        // P-08: laser warning text — no backing rectangle, clean text only
        if (ms && ms->isWarning() && fontOk) {
            Text wt("!! LASER INCOMING  --  STAY ON EDGES !!", font, 23);
            wt.setFillColor(Color(255, 60, 60));
            wt.setPosition(WINDOW_W / 2 - wt.getLocalBounds().width / 2, WINDOW_H / 2 - 18);
            window.draw(wt);
        }
    }

    void drawBossIncomingBanner()
    {
        if (!bossWaiting || bossSpawned || !fontOk)
            return;
        Text bt("ELIMINATE ALL ENEMIES  --  BOSS INCOMING!", font, 20);
        bt.setFillColor(Color(255, 200, 0));
        bt.setPosition(WINDOW_W / 2 - bt.getLocalBounds().width / 2, WINDOW_H / 2 - 16); window.draw(bt);
    }

    void drawGameplayFrame()
    {
        window.clear(Color(4, 4, 16));
        if (shakeAmt > 0) {
            float t = shakeClock.getElapsedTime().asSeconds(), dur = empShake ? 0.50f : 0.30f;
            if (t < dur) 
            {
                float decay = 1.0f - t / dur; float ox = ((float)(rand() % 101 - 50) / 50.0f) * shakeAmt * decay, oy = ((float)(rand() % 101 - 50) / 50.0f) * shakeAmt * decay; View v = window.getDefaultView(); v.move(ox, oy); window.setView(v);
            }
            else 
            {
                shakeAmt = 0; empShake = false; window.setView(window.getDefaultView());
            }
        }
        stars.draw(window);
        for (int i = 0; i < asteroidCnt; i++)
            if (asteroids[i]) 
                asteroids[i]->draw(window);
        for (int i = 0; i < powerUpCnt; i++) 
            if (powerups[i]) 
                powerups[i]->draw(window);
        for (int i = 0; i < enemyCnt; i++)    
            if (enemies[i])   
                enemies[i]->draw(window);
        if (boss) boss->draw(window);
        for (int i = 0; i < bulletCnt; i++)  
            if (bullets[i])  
                bullets[i]->draw(window);
        player->draw(window);
        explosions.draw(window);
        if (empFlashOn) 
        {
            float t = empFlashClock.getElapsedTime().asSeconds();
            if (t < 0.45f)
            {
                RectangleShape fl(Vector2f(WINDOW_W, WINDOW_H)); fl.setFillColor(Color(80, 200, 255, (Uint8)((1 - t / 0.45f) * 210))); window.draw(fl);
            }
            else
                empFlashOn = false; 
        }
        window.setView(window.getDefaultView());
        drawHUD(); drawBossInfo(); drawBossIncomingBanner();
        window.display();
    }

    // P-09: Lives = red circles; no Unicode
    void drawHUD() {
        if (!fontOk) 
            return;
        float y = 8;
        auto txt = [&](const string& s, unsigned sz, Color c, float x, float yp) 
            {
                Text t(s, font, sz); t.setFillColor(c); t.setPosition(x, yp); window.draw(t);
            };
        txt("SCORE  " + to_string(player->getScore()), 20, Color(220, 220, 220), 10, y); y += 26;
        int mul = player->getMultiplier();
        Color mc = (mul >= 4) ? Color(255, 60, 60) : (mul >= 2) ? Color(255, 220, 0) : Color(100, 100, 100);
        txt("MULT   x" + to_string(mul), 20, mc, 10, y); y += 26;
        // P-09: red circles for lives
        txt("LIVES  ", 20, Color(255, 80, 80), 10, y);
        for (int i = 0; i < player->getLives(); i++) 
        {
            CircleShape dot(7.0f); dot.setFillColor(Color(255, 50, 50)); dot.setOutlineColor(Color(180, 0, 0)); dot.setOutlineThickness(1.5f); dot.setPosition(90.0f + i * 20.0f, y + 4.0f); 
            window.draw(dot);
        }
        y += 26;
        txt("WPN    " + player->getWeaponName(), 20, Color(60, 255, 140), 10, y); y += 26;
        int sh = player->getShieldHits();
        txt(sh > 0 ? ("SHIELD [" + to_string(sh) + "]") : "SHIELD ---", 20, sh > 0 ? Color(0, 220, 255) : Color(80, 80, 80), 10, y); y += 26;
        string es = "EMP    ";
        for (int i = 0; i < player->getEMPCount(); i++) 
            es += "* "; 
        for (int i = player->getEMPCount(); i < 3; i++)
            es += "o ";
        txt(es, 20, Color(255, 220, 0), 10, y); y += 26;
        txt("DASH   ", 20, Color(180, 180, 180), 10, y);
        RectangleShape dbg(Vector2f(90, 12)); dbg.setFillColor(Color(30, 30, 30)); dbg.setPosition(84, y + 5); window.draw(dbg);
        float dr = player->getDashCooldown(), dfw = (dr <= 0) ? 90 : (1.0f - dr / 3.0f) * 90;
        RectangleShape dfg(Vector2f(dfw, 12)); dfg.setFillColor(dr <= 0 ? Color(0, 200, 80) : Color(200, 120, 0)); dfg.setPosition(84, y + 5); window.draw(dfg); y += 26;
        if (state == STATE_SURVIVAL) 
        {
            Text wt("WAVE  " + to_string(wave), font, 24); wt.setFillColor(Color(100, 210, 255)); wt.setPosition(WINDOW_W / 2 - wt.getLocalBounds().width / 2, 8); window.draw(wt);
            if (waveClearFlag) { Text ct("WAVE CLEAR!", font, 38); ct.setFillColor(Color::Yellow); ct.setPosition(WINDOW_W / 2 - ct.getLocalBounds().width / 2, WINDOW_H / 2 - 22); window.draw(ct); }
        }
        if (state == STATE_ARCADE && !bossSpawned)
        {
            Text lt("LEVEL " + to_string(arcadeLevel), font, 22); lt.setFillColor(Color(255, 180, 80)); lt.setPosition(WINDOW_W / 2 - lt.getLocalBounds().width / 2, 8); window.draw(lt);
            if (!bossWaiting) { int need = bossScoreThreshold(), cur = player->getScore(); int pct = (int)(min(1.0f, (float)cur / (float)need) * 100); Text pt("Boss at " + to_string(need) + " pts [" + to_string(pct) + "%]", font, 15); pt.setFillColor(Color(120, 120, 120)); pt.setPosition(WINDOW_W / 2 - pt.getLocalBounds().width / 2, 34); window.draw(pt); }
        }
    }

    //  MAIN MENU — 5 options (P-02)


    void drawMenu() {
        window.clear(Color(4, 4, 20)); stars.draw(window);
        if (!fontOk) { window.display(); return; }
        auto ctr = [&](const string& s, unsigned sz, Color c, float y) {Text t(s, font, sz); t.setFillColor(c); t.setPosition(WINDOW_W / 2 - t.getLocalBounds().width / 2, y); window.draw(t); };
        ctr("SPACE INVADERS", 60, Color(80, 200, 255), 80);
        ctr("Galactic Defense", 26, Color(100, 180, 255), 152);
        RectangleShape div(Vector2f(440, 2)); div.setFillColor(Color(40, 80, 160)); div.setPosition(WINDOW_W / 2 - 220, 192); window.draw(div);
        // P-02: 5 menu options
        const char* labels[5] = { "Arcade Mode","Survival Mode","Instructions","Controls","Exit" };
        Color selC(255, 220, 0), normC(200, 200, 200);
        for (int i = 0; i < 5; i++) {
            float by = 214.0f + i * 60.0f; bool sel = (menuSel == i);
            if (sel) { RectangleShape btn(Vector2f(340, 46)); btn.setFillColor(Color(20, 50, 120, 180)); btn.setOutlineColor(Color(80, 160, 255)); btn.setOutlineThickness(2); btn.setPosition(WINDOW_W / 2 - 170, by - 5); window.draw(btn); }
            ctr(string(sel ? "> " : "  ") + labels[i], 28, sel ? selC : normC, by);
        }
        RectangleShape div2(Vector2f(440, 2)); div2.setFillColor(Color(40, 80, 160)); div2.setPosition(WINDOW_W / 2 - 220, 520); window.draw(div2);
        ctr("Arrow Keys: Navigate   Enter: Select", 15, Color(80, 80, 100), 530);
        window.display();
    }

    void inputMenu() {
        static bool uW = false, dW = false, eW = false;
        bool uN = Keyboard::isKeyPressed(Keyboard::Up), dN = Keyboard::isKeyPressed(Keyboard::Down), eN = Keyboard::isKeyPressed(Keyboard::Return);
        if (uN && !uW) menuSel = (menuSel - 1 + 5) % 5; // P-02: 5 options
        if (dN && !dW) menuSel = (menuSel + 1) % 5;
        if (eN && !eW) {
            if (menuSel == 0) startArcade();
            else if (menuSel == 1) startSurvival();
            else if (menuSel == 2) state = STATE_INSTRUCTIONS; // P-01
            else if (menuSel == 3) state = STATE_CONTROLS;
            else                window.close();
        }
        if (Keyboard::isKeyPressed(Keyboard::Num1)) { menuSel = 0; startArcade(); }
        if (Keyboard::isKeyPressed(Keyboard::Num2)) { menuSel = 1; startSurvival(); }
        uW = uN; dW = dN; eW = eN;
    }

    //  INSTRUCTIONS SCREEN — P-01

    void drawInstructions() {
        window.clear(Color(4, 4, 20)); stars.draw(window);
        if (!fontOk) { window.display(); return; }

        // Title
        Text title("HOW TO PLAY", font, 38); title.setFillColor(Color(80, 200, 255));
        title.setPosition(WINDOW_W / 2 - title.getLocalBounds().width / 2, 30); window.draw(title);

        // Top divider
        RectangleShape topDiv(Vector2f(920, 2)); topDiv.setFillColor(Color(40, 80, 160)); topDiv.setPosition(40, 82); window.draw(topDiv);

        // Column divider
        RectangleShape colDiv(Vector2f(2, 360)); colDiv.setFillColor(Color(40, 80, 100, 120)); colDiv.setPosition(498, 90); window.draw(colDiv);

        // Helper lambdas
        float lx = 42, rx = 515;
        auto hdr = [&](const string& s, float x, float y, Color c = Color(255, 200, 60)) {
            Text t(s, font, 17); t.setFillColor(c); t.setPosition(x, y); window.draw(t);
            };
        auto row = [&](const string& label, const string& desc, float x, float y, Color lc, Color dc = Color(160, 160, 160)) {
            Text tl(label, font, 15); tl.setFillColor(lc); tl.setPosition(x, y); window.draw(tl);
            Text td(" -- " + desc, font, 14); td.setFillColor(dc); td.setPosition(x + tl.getLocalBounds().width + 2, y + 1); window.draw(td);
            };
        auto line = [&](const string& s, float x, float y, Color c = Color(155, 155, 155)) {
            Text t(s, font, 14); t.setFillColor(c); t.setPosition(x, y); window.draw(t);
            };

        // ── LEFT COLUMN ──────────────────────────────────────────────
        float ly = 92;
        hdr("GAME MODES", lx, ly); ly += 28;
        Text amTitle("Arcade Mode", font, 15); amTitle.setFillColor(Color(255, 180, 80)); amTitle.setPosition(lx, ly); window.draw(amTitle); ly += 22;
        line("  Fight through 3 levels, each ending with a boss", lx, ly); ly += 20;
        line("  Score threshold unlocks the boss fight", lx, ly); ly += 20;
        line("  Beat the Mothership to win", lx, ly); ly += 30;
        Text smTitle("Survival Mode", font, 15); smTitle.setFillColor(Color(100, 210, 255)); smTitle.setPosition(lx, ly); window.draw(smTitle); ly += 22;
        line("  Endless escalating waves, no bosses", lx, ly); ly += 20;
        line("  Enemy speed +5% and fire rate +10% per wave", lx, ly); ly += 20;
        line("  Vipers appear wave 5+, Seekers wave 8+", lx, ly); ly += 38;

        hdr("ENEMIES", lx, ly); ly += 26;
        row("DRONE", "Straight down, fires at random intervals", lx, ly, Color(220, 80, 80));  ly += 24;
        row("VIPER", "Sine-wave movement, fires regularly", lx, ly, Color(180, 80, 255)); ly += 24;
        row("SEEKER", "Locks your X on spawn, kamikaze no fire", lx, ly, Color(255, 120, 0));  ly += 24;

        // ── RIGHT COLUMN ─────────────────────────────────────────────
        float ry = 92;
        hdr("POWERUPS", rx, ry, Color(255, 200, 60)); ry += 28;
        row("SPREAD", "Three-way bullet fan", rx, ry, Color(255, 150, 0));  ry += 24;
        row("PIERCING", "Heavy beam, pierces 2 enemies", rx, ry, Color(180, 80, 255)); ry += 24;
        row("SHIELD", "Absorbs 2 hits before breaking", rx, ry, Color(0, 200, 255));  ry += 24;
        row("EMP", "Destroys all enemies on screen [N]", rx, ry, Color(255, 230, 0));  ry += 38;

        hdr("BOSSES", rx, ry, Color(255, 200, 60)); ry += 26;
        Text cr("CRUISER  (Level 1)", font, 15); cr.setFillColor(Color(200, 80, 255)); cr.setPosition(rx, ry); window.draw(cr); ry += 20;
        line("  Bullet wall -- find the single safe gap", rx, ry); ry += 24;
        Text tc("TWIN CANNONS  (Level 2)", font, 15); tc.setFillColor(Color(0, 200, 220)); tc.setPosition(rx, ry); window.draw(tc); ry += 20;
        line("  Destroy BOTH turrets to expose the core", rx, ry); ry += 24;
        Text ms("MOTHERSHIP  (Level 3)", font, 15); ms.setFillColor(Color(200, 0, 255)); ms.setPosition(rx, ry); window.draw(ms); ry += 20;
        line("  Dodge the wide screen laser -- dash to edges", rx, ry); ry += 24;

        // ── CONTROLS BAR ─────────────────────────────────────────────
        float cy2 = 462;
        RectangleShape midDiv(Vector2f(920, 2)); midDiv.setFillColor(Color(40, 80, 160)); midDiv.setPosition(40, cy2); window.draw(midDiv); cy2 += 10;
        hdr("CONTROLS", lx, cy2, Color(255, 200, 60)); cy2 += 26;

        struct Ctrl { const char* k; const char* a; };
        Ctrl ctrls[] = { {"Arrows","Move"},{"Space","Fire"},{"E + Arrow","Evasive Dash (0.5s invincible)"},{"N","EMP Screen Nuke"},{"ESC","Pause"} };
        float cx2 = lx; int nc = 5;
        for (int i = 0; i < nc; i++)
        {
            Text tk(ctrls[i].k, font, 14); tk.setFillColor(Color(255, 220, 0)); tk.setPosition(cx2, cy2); window.draw(tk);
            Text ta(string("  ") + ctrls[i].a, font, 14); ta.setFillColor(Color(160, 160, 160)); ta.setPosition(cx2 + tk.getLocalBounds().width, cy2 + 1); window.draw(ta);
            cx2 += 180; 
            if (i == 2)
            {
                cx2 = lx; cy2 += 22;
            }
        }

        // Bottom
        RectangleShape botDiv(Vector2f(920, 2)); botDiv.setFillColor(Color(40, 80, 160)); botDiv.setPosition(40, 745); window.draw(botDiv);
        Text back("[ESC or Enter]  Back to Menu", font, 16); back.setFillColor(Color(100, 100, 120));
        back.setPosition(WINDOW_W / 2 - back.getLocalBounds().width / 2, 754); window.draw(back);

        window.display();
        static bool escW = false, entW = false;
        bool escN = Keyboard::isKeyPressed(Keyboard::Escape), entN = Keyboard::isKeyPressed(Keyboard::Return);
        if ((escN && !escW) || (entN && !entW)) state = STATE_MENU;
        escW = escN; entW = entN;
    }


    //  CONTROLS SCREEN


    void drawControls() {
        window.clear(Color(4, 4, 20)); stars.draw(window);
        if (!fontOk) { window.display(); return; }
        auto ctr = [&](const string& s, unsigned sz, Color c, float y) {Text t(s, font, sz); t.setFillColor(c); t.setPosition(WINDOW_W / 2 - t.getLocalBounds().width / 2, y); window.draw(t); };
        ctr("CONTROLS", 44, Color(80, 200, 255), 80);
        RectangleShape div(Vector2f(440, 2)); div.setFillColor(Color(40, 80, 160)); div.setPosition(WINDOW_W / 2 - 220, 140); window.draw(div);
        struct Row { const char* a; const char* k; };
        Row rows[] = { "Move","Arrow Keys","Fire","Spacebar","Evasive Dash","E + Arrow Key","EMP / Nuke","N","Pause","ESC","Resume","R  (paused)","Quit to Menu","Q  (paused)" };
        int n = 7;
        for (int i = 0; i < n; i++) {
            Text ta(rows[i].a, font, 22); ta.setFillColor(Color(200, 200, 200)); ta.setPosition(WINDOW_W / 2 - 220, 160 + i * 54); window.draw(ta);
            Text tk(rows[i].k, font, 22); tk.setFillColor(Color(255, 220, 0)); tk.setPosition(WINDOW_W / 2 + 20, 160 + i * 54); window.draw(tk);
        }
        ctr("[ESC or Enter]  Back to Menu", 17, Color(80, 80, 100), 570);
        window.display();
        static bool escW = false, entW = false;
        bool escN = Keyboard::isKeyPressed(Keyboard::Escape), entN = Keyboard::isKeyPressed(Keyboard::Return);
        if ((escN && !escW) || (entN && !entW)) state = STATE_MENU;
        escW = escN; entW = entN;
    }

    // ====================================================================
    //  PAUSE
    // ====================================================================
    void drawPause() {
        RectangleShape dim(Vector2f(WINDOW_W, WINDOW_H)); dim.setFillColor(Color(0, 0, 0, 160)); window.draw(dim);
        if (!fontOk) { window.display(); return; }
        auto ctr = [&](const string& s, unsigned sz, Color c, float y) {Text t(s, font, sz); t.setFillColor(c); t.setPosition(WINDOW_W / 2 - t.getLocalBounds().width / 2, y); window.draw(t); };
        RectangleShape panel(Vector2f(340, 220)); panel.setFillColor(Color(10, 10, 30, 220)); panel.setOutlineColor(Color(80, 160, 255)); panel.setOutlineThickness(2); panel.setPosition(WINDOW_W / 2 - 170, 260); window.draw(panel);
        ctr("PAUSED", 48, Color::White, 278); ctr("[R]  Resume", 28, Color(120, 255, 120), 358); ctr("[Q]  Quit to Menu", 28, Color(255, 100, 100), 408);
        window.display();
    }
    void inputPause() {
        static bool rW = false, qW = false; bool rN = Keyboard::isKeyPressed(Keyboard::R), qN = Keyboard::isKeyPressed(Keyboard::Q);
        if (rN && !rW) { state = prevPlayState; if (gAudio) { if (boss) gAudio->playBossMusic(); else gAudio->playGameplayMusic(); } }
        if (qN && !qW) { clearAll(); delete player; player = nullptr; state = STATE_MENU; if (gAudio) gAudio->stopAll(); }
        rW = rN; qW = qN;
    }

    // ====================================================================
    //  GAME OVER / WIN
    // ====================================================================
    void drawGameOver() {
        window.clear(Color(10, 0, 0)); stars.draw(window);
        if (fontOk) {
            auto ctr = [&](const string& s, unsigned sz, Color c, float y) {Text t(s, font, sz); t.setFillColor(c); t.setPosition(WINDOW_W / 2 - t.getLocalBounds().width / 2, y); window.draw(t); };
            RectangleShape panel(Vector2f(440, 200)); panel.setFillColor(Color(30, 0, 0, 200)); panel.setOutlineColor(Color(180, 0, 0)); panel.setOutlineThickness(2); panel.setPosition(WINDOW_W / 2 - 220, 210); window.draw(panel);
            ctr("GAME OVER", 58, Color(255, 60, 60), 220);
            ctr("Final Score: " + to_string(player ? player->getScore() : 0), 30, Color::White, 308);
            ctr("[Enter]  Return to Menu", 22, Color(140, 140, 140), 375);
        }
        window.display();
        static bool eW = false; bool eN = Keyboard::isKeyPressed(Keyboard::Return);
        if (eN && !eW) { clearAll(); delete player; player = nullptr; state = STATE_MENU; }
        eW = eN;
    }
    void drawWin() {
        window.clear(Color(0, 10, 20)); stars.draw(window);
        if (fontOk) {
            auto ctr = [&](const string& s, unsigned sz, Color c, float y) {Text t(s, font, sz); t.setFillColor(c); t.setPosition(WINDOW_W / 2 - t.getLocalBounds().width / 2, y); window.draw(t); };
            RectangleShape panel(Vector2f(500, 230)); panel.setFillColor(Color(0, 20, 10, 200)); panel.setOutlineColor(Color(0, 200, 100)); panel.setOutlineThickness(2); panel.setPosition(WINDOW_W / 2 - 250, 175); window.draw(panel);
            ctr("VICTORY!", 62, Color(100, 255, 120), 185);
            ctr("Mothership Destroyed!", 28, Color(180, 255, 180), 268);
            ctr("Score: " + to_string(player ? player->getScore() : 0), 32, Color::Yellow, 314);
            ctr("[Enter]  Return to Menu", 22, Color(120, 120, 120), 375);
        }
        window.display();
        static bool eW = false; bool eN = Keyboard::isKeyPressed(Keyboard::Return);
        if (eN && !eW) { clearAll(); delete player; player = nullptr; state = STATE_MENU; }
        eW = eN;
    }

public:
    Game()
        : window(VideoMode((int)WINDOW_W, (int)WINDOW_H), "Space Invaders: Galactic Defense", Style::Close),
        fontOk(false), state(STATE_MENU), prevPlayState(STATE_ARCADE), player(nullptr),
        bulletCnt(0), enemyCnt(0), asteroidCnt(0), powerUpCnt(0), boss(nullptr),
        arcadeLevel(1), bossWaiting(false), bossSpawned(false),
        wave(1), waveTotal(5), waveSpawned(0), waveSpeedScale(1.0f), waveFireScale(1.0f),
        waveClearFlag(false), waveClearTimer(0),
        empFlashOn(false), empShake(false), shakeAmt(0.0f), menuSel(0) {
        window.setFramerateLimit(60);
        initArrays();
        fontOk = font.loadFromFile("arial.ttf");
        if (!fontOk) cout << "Warning: arial.ttf not found\n";
    }
    ~Game() { clearAll(); delete player; }

    void run() {
        // P-03: start menu music immediately on launch
        if (gAudio) gAudio->playGameplayMusic();

        while (window.isOpen()) {
            Event ev;
            while (window.pollEvent(ev)) {
                if (ev.type == Event::Closed) window.close();
                if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape)
                    if (state == STATE_ARCADE || state == STATE_SURVIVAL) { prevPlayState = state; state = STATE_PAUSED; if (gAudio) gAudio->stopAll(); }
            }
            float dt = deltaClock.restart().asSeconds(); if (dt > 0.033f) dt = 0.033f;
            stars.update(dt);

            switch (state) {
            case STATE_MENU:         drawMenu();         inputMenu();   break;
            case STATE_INSTRUCTIONS: drawInstructions();               break; // P-01
            case STATE_CONTROLS:     drawControls();                    break;
            case STATE_ARCADE:
            case STATE_SURVIVAL:     inputGameplay(dt); updateGameplay(dt); drawGameplayFrame(); break;
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
    AudioManager audio; gAudio = &audio;
    Game game; game.run();
    gAudio = nullptr; return 0;
}