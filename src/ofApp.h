#pragma once
#include "ofMain.h"
#include "ofxAssimpModelLoader.h"

struct Projectile {
    ofVec3f pos;
    ofVec3f vel;
    float age = 0.0f;
    float life = 2.5f;      // seconds
    float radius = 2.0f;
    bool alive = true;
};

struct Asteroid {
    ofVec3f pos;
    ofQuaternion rot;
    ofVec3f axis;
    float spinDegPerSec = 25.0f;
    float radius = 25.0f;
    bool alive = true;
};

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;

    void keyPressed(int key) override;
    void keyReleased(int key) override;
    void mouseDragged(int x, int y, int button) override;

private:
    // --- world ---
    void resetWorld();
    void spawnAsteroids(int n);
    void fireProjectile();
    void updateShip(float dt);
    void updateProjectiles(float dt);
    void updateAsteroids(float dt);
    void handleCollisions();
    void drawScene3D();
    void drawHud2D();

    // --- viewports ---
    void beginViewport(const ofRectangle& r);
    void endViewport();

    // --- ship state ---
    ofVec3f shipPos{0, 0, 200};
    ofQuaternion shipRot; // orientation
    float shipSpeed = 0.0f;
    float shipMaxSpeed = 240.0f;
    float shipAccel = 320.0f;
    float shipTurnSpeed = 0.02f; // mouse drag sensitivity

    bool kW=false, kA=false, kS=false, kD=false, kQ=false, kE=false;

    // --- cameras ---
    ofCamera camThird;
    ofCamera camTop;
    ofCamera camCockpit;

    // --- lighting ---
    ofLight sunLight;
    ofLight shipSpot;
    ofMaterial matShip;
    ofMaterial matRock;

    // --- models/textures ---
    ofxAssimpModelLoader shipModel;
    ofxAssimpModelLoader asteroidModel;
    ofTexture shipTex;
    ofTexture asteroidTex;
    bool shipLoaded=false, asteroidLoaded=false;

    // --- dynamic objects ---
    std::vector<Asteroid> asteroids;
    std::vector<Projectile> projectiles;

    // --- sound ---
    ofSoundPlayer engineLoop;
    ofSoundPlayer laserSfx;
    ofSoundPlayer boomSfx;

    // --- timing / UI ---
    float timeSec = 0.0f;
    float dtClamp = 1.0f / 30.0f;
    std::string status;
    bool showGrid = true; // Toggle grid display
};