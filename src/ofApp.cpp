// made by Yassine Amraoui & Rayane Tsouli

#include "ofApp.h"

static ofVec3f forwardFromQuat(const ofQuaternion& q) {
    return q * ofVec3f(0, 0, -1);
}
static ofVec3f rightFromQuat(const ofQuaternion& q) {
    return q * ofVec3f(1, 0, 0);
}
static ofVec3f upFromQuat(const ofQuaternion& q) {
    return q * ofVec3f(0, 1, 0);
}

void ofApp::setup() {
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofBackground(5);
    ofEnableDepthTest();
    ofDisableArbTex();

    // --- load GLTF model ---
    shipLoaded = shipModel.loadModel("scene.gltf", true);
    if (shipLoaded) {
        ofLogNotice() << "Ship model loaded successfully";
    } else {
        ofLogWarning() << "Ship model failed to load, using fallback box";
    }

    // No asteroid model - we'll use procedural spheres
    asteroidLoaded = false;

    // Load textures from texture folder
    ofImage tmp;
    ofDirectory dir("textures");
    dir.allowExt("jpg");
    dir.allowExt("png");
    dir.listDir();

    if (dir.size() > 0) {
        // Try to load textures - look for ship and asteroid textures
        for (int i = 0; i < dir.size(); i++) {
            string filename = dir.getName(i);
            ofToLower(filename); // Make lowercase for comparison

            if (filename.find("ship") != string::npos || filename.find("spaceship") != string::npos) {
                if (tmp.load(dir.getPath(i))) {
                    shipTex = tmp.getTexture();
                    ofLogNotice() << "Loaded ship texture: " << dir.getName(i);
                }
            }
            else if (filename.find("asteroid") != string::npos || filename.find("rock") != string::npos) {
                if (tmp.load(dir.getPath(i))) {
                    asteroidTex = tmp.getTexture();
                    ofLogNotice() << "Loaded asteroid texture: " << dir.getName(i);
                }
            }
        }

        // If no asteroid texture found, use first available texture
        if (!asteroidTex.isAllocated() && dir.size() > 0) {
            if (tmp.load(dir.getPath(0))) {
                asteroidTex = tmp.getTexture();
                ofLogNotice() << "Using fallback asteroid texture: " << dir.getName(0);
            }
        }
    }

    // --- lighting ---
    sunLight.setup();
    sunLight.setPointLight();
    sunLight.setPosition(500, 400, 300);
    sunLight.setDiffuseColor(ofFloatColor(1.0, 1.0, 1.0));
    sunLight.setSpecularColor(ofFloatColor(1.0, 1.0, 1.0));

    shipSpot.setup();
    shipSpot.setSpotlight();
    shipSpot.setSpotlightCutOff(30);
    shipSpot.setSpotConcentration(40);
    shipSpot.setDiffuseColor(ofFloatColor(0.9, 0.95, 1.0));
    shipSpot.setSpecularColor(ofFloatColor(1.0, 1.0, 1.0));

    matShip.setShininess(64);
    matShip.setSpecularColor(ofFloatColor(1,1,1));
    matRock.setShininess(12);
    matRock.setSpecularColor(ofFloatColor(0.6,0.6,0.6));

    // --- sound (optional - won't crash if missing) ---
    bool engineLoaded = engineLoop.load("sounds/engine_loop.wav");
    if (engineLoaded) {
        engineLoop.setLoop(true);
        engineLoop.setMultiPlay(false);
        engineLoop.setVolume(0.35f);
        engineLoop.play();
        ofLogNotice() << "Engine sound loaded";
    } else {
        ofLogWarning() << "Engine sound not found - continuing without audio";
    }

    bool laserLoaded = laserSfx.load("sounds/laser.wav");
    if (laserLoaded) {
        laserSfx.setMultiPlay(true);
        laserSfx.setVolume(0.7f);
    }

    bool boomLoaded = boomSfx.load("sounds/explosion.wav");
    if (boomLoaded) {
        boomSfx.setMultiPlay(true);
        boomSfx.setVolume(0.8f);
    }

    resetWorld();
}

void ofApp::resetWorld() {
    shipPos.set(0, 0, 220);
    shipRot.makeRotate(0, 0, 1, 0);
    shipSpeed = 0.0f;

    asteroids.clear();
    projectiles.clear();
    spawnAsteroids(30);

    status = "WASD move, Q/E up/down, mouse drag turn, SPACE fire, R reset, G toggle grid";
}

void ofApp::spawnAsteroids(int n) {
    asteroids.reserve(asteroids.size() + n);
    for (int i = 0; i < n; i++) {
        Asteroid a;
        float r = ofRandom(250, 900);
        float ang = ofRandom(TWO_PI);
        a.pos = ofVec3f(cosf(ang)*r, ofRandom(-120, 120), sinf(ang)*r);

        a.axis = ofVec3f(ofRandomf(), ofRandomf(), ofRandomf()).getNormalized();
        a.spinDegPerSec = ofRandom(8, 40);
        a.radius = ofRandom(18, 55);
        a.rot.makeRotate(ofRandom(0, 360), a.axis);

        asteroids.push_back(a);
    }
}

void ofApp::update() {
    float dt = ofClamp(ofGetLastFrameTime(), 0.0f, dtClamp);
    timeSec += dt;

    updateShip(dt);
    updateProjectiles(dt);
    updateAsteroids(dt);
    handleCollisions();

    // engine pitch based on speed (only if sound loaded)
    if (engineLoop.isLoaded()) {
        float norm = ofClamp(fabs(shipSpeed) / shipMaxSpeed, 0.0f, 1.0f);
        engineLoop.setSpeed(0.9f + 0.5f * norm);
        engineLoop.setVolume(0.25f + 0.25f * norm);
    }
}

void ofApp::updateShip(float dt) {
    // throttle
    float target = 0.0f;
    if (kW) target += 1.0f;
    if (kS) target -= 1.0f;

    float accel = shipAccel * target;
    shipSpeed += accel * dt;
    shipSpeed = ofClamp(shipSpeed, -shipMaxSpeed * 0.5f, shipMaxSpeed);

    // strafe + vertical
    ofVec3f lateral(0, 0, 0);
    if (kA) lateral -= rightFromQuat(shipRot);
    if (kD) lateral += rightFromQuat(shipRot);
    if (kQ) lateral += upFromQuat(shipRot);
    if (kE) lateral -= upFromQuat(shipRot);
    if (lateral.lengthSquared() > 0) lateral.normalize();

    shipPos += forwardFromQuat(shipRot) * shipSpeed * dt;
    shipPos += lateral * (shipMaxSpeed * 0.55f) * dt;

    // update camera rigs
    ofVec3f fwd = forwardFromQuat(shipRot);
    ofVec3f up = upFromQuat(shipRot);

    camThird.setPosition(shipPos - fwd * 240.0f + up * 90.0f);
    camThird.lookAt(shipPos + fwd * 40.0f, up);

    camTop.setPosition(shipPos + ofVec3f(0, 600, 0));
    camTop.lookAt(shipPos, ofVec3f(0, 0, -1));

    camCockpit.setPosition(shipPos + up * 10.0f);
    camCockpit.lookAt(shipPos + fwd * 200.0f, up);

    shipSpot.setPosition(shipPos);
    shipSpot.lookAt(shipPos + fwd * 300.0f, up);
}

void ofApp::updateProjectiles(float dt) {
    for (auto &p : projectiles) {
        if (!p.alive) continue;
        p.age += dt;
        p.pos += p.vel * dt;
        if (p.age >= p.life) p.alive = false;
    }
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
                       [](const Projectile& p){ return !p.alive; }),
        projectiles.end()
    );
}

void ofApp::updateAsteroids(float dt) {
    for (auto &a : asteroids) {
        if (!a.alive) continue;
        ofQuaternion dq;
        dq.makeRotate(a.spinDegPerSec * dt, a.axis);
        a.rot = dq * a.rot;
    }
    asteroids.erase(
        std::remove_if(asteroids.begin(), asteroids.end(),
                       [](const Asteroid& a){ return !a.alive; }),
        asteroids.end()
    );

    if ((int)asteroids.size() < 20) spawnAsteroids(10);
}

void ofApp::handleCollisions() {
    for (auto &p : projectiles) {
        if (!p.alive) continue;
        for (auto &a : asteroids) {
            if (!a.alive) continue;
            float d2 = (p.pos - a.pos).lengthSquared();
            float rr = (p.radius + a.radius);
            if (d2 < rr * rr) {
                p.alive = false;
                a.alive = false;
                if (boomSfx.isLoaded()) boomSfx.play();
                break;
            }
        }
    }
}

void ofApp::draw() {
    ofEnableDepthTest();
    ofEnableLighting();

    const int w = ofGetWidth();
    const int h = ofGetHeight();

    // Draw main viewport first (full screen)
    camThird.begin();
    drawScene3D();
    camThird.end();

    // Now draw the mini viewports on top
    ofDisableLighting();
    ofDisableDepthTest();

    // TOP DOWN viewport
    ofRectangle topVp(w - 320, 20, 300, 180);
    beginViewport(topVp);
    ofEnableDepthTest();
    ofEnableLighting();
    camTop.begin();
    drawScene3D();
    camTop.end();
    ofDisableLighting();
    ofDisableDepthTest();
    endViewport();

    // COCKPIT viewport
    ofRectangle cockpitVp(w - 320, 220, 300, 180);
    beginViewport(cockpitVp);
    ofEnableDepthTest();
    ofEnableLighting();
    camCockpit.begin();
    drawScene3D();
    camCockpit.end();
    ofDisableLighting();
    ofDisableDepthTest();
    endViewport();

    // Draw HUD and borders on top
    drawHud2D();
}

void ofApp::drawScene3D() {
    sunLight.enable();
    shipSpot.enable();

    // Draw grid only if enabled
    if (showGrid) {
        ofPushStyle();
        ofSetColor(40);
        ofDrawGridPlane(200.0f, 8, true);
        ofPopStyle();
    }

    // Enable face drawing
    ofFill();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // ship - GLTF model
    ofPushMatrix();
    ofTranslate(shipPos);

    ofMatrix4x4 m;
    shipRot.get(m);
    ofMultMatrix(m);

	// The ship was initially spawning upside down
    // Flip upside down ship and rotate to face forward
    ofRotateXDeg(180);
    ofRotateYDeg(180);  // Turn it around to face away from camera

    // Scale down the spaceship
    ofScale(0.1, 0.1, 0.1);

    matShip.begin();
    if (shipTex.isAllocated()) shipTex.bind();

    if (shipLoaded) {
        ofSetColor(255);
        shipModel.draw(OF_MESH_FILL); // Force filled mesh drawing
    } else {
        ofSetColor(220);
        ofDrawBox(0, 0, 0, 30, 12, 60);
    }

    if (shipTex.isAllocated()) shipTex.unbind();
    matShip.end();

    ofPopMatrix();

    // asteroids - using procedural spheres with rocky look
    matRock.begin();
    if (asteroidTex.isAllocated()) asteroidTex.bind();

    for (auto &a : asteroids) {
        ofPushMatrix();
        ofTranslate(a.pos);

        ofMatrix4x4 rm;
        a.rot.get(rm);
        ofMultMatrix(rm);

        // Set color (gray/brown for rocky look)
        if (asteroidTex.isAllocated()) {
            ofSetColor(255);
        } else {
            // Create a rocky appearance with color variation
            float colorVar = ofMap(a.radius, 18, 55, 0.5f, 0.8f);
            ofSetColor(160 * colorVar, 150 * colorVar, 140 * colorVar);
        }

        // Draw icosphere for rocky look
        ofDrawIcoSphere(0, 0, 0, a.radius);

        ofPopMatrix();
    }

    if (asteroidTex.isAllocated()) asteroidTex.unbind();
    matRock.end();

    // projectiles
    ofPushStyle();
    ofSetColor(255, 80, 80);
    for (auto &p : projectiles) {
        ofDrawSphere(p.pos, p.radius);
    }
    ofPopStyle();

    shipSpot.disable();
    sunLight.disable();
}

void ofApp::drawHud2D() {
    ofSetColor(255);
    ofDrawBitmapStringHighlight(status, 15, 20);

    ofSetColor(200);
    ofDrawBitmapString("Asteroids: " + ofToString(asteroids.size()) +
                       "   Projectiles: " + ofToString(projectiles.size()) +
                       "   Speed: " + ofToString((int)shipSpeed) +
                       "   Grid: " + (showGrid ? "ON" : "OFF"),
                       15, 45);

    ofNoFill();
    ofSetColor(255, 255, 255, 90);
    ofDrawRectangle(ofGetWidth() - 320, 20, 300, 180);
    ofDrawRectangle(ofGetWidth() - 320, 220, 300, 180);
}

void ofApp::beginViewport(const ofRectangle& r) {
    // OpenGL's coordinate system has Y=0 at bottom, ofRectangle has Y=0 at top
    int glY = ofGetHeight() - (r.y + r.height);

    ofViewport(r.x, r.y, r.width, r.height);
    glScissor(r.x, glY, r.width, r.height);
    glEnable(GL_SCISSOR_TEST);

    // Clear this viewport
    ofPushStyle();
    ofClear(5, 5, 8, 255);
    ofPopStyle();
}

void ofApp::endViewport() {
    glDisable(GL_SCISSOR_TEST);
    ofViewport();
}

void ofApp::fireProjectile() {
    Projectile p;
    ofVec3f fwd = forwardFromQuat(shipRot);
    p.pos = shipPos + fwd * 40.0f;
    p.vel = fwd * 650.0f;
    p.life = 2.2f;
    p.radius = 2.2f;
    projectiles.push_back(p);

    if (laserSfx.isLoaded()) laserSfx.play();
}

void ofApp::keyPressed(int key) {
    if (key == 'w' || key == 'W') kW = true;
    if (key == 's' || key == 'S') kS = true;
    if (key == 'a' || key == 'A') kA = true;
    if (key == 'd' || key == 'D') kD = true;
    if (key == 'q' || key == 'Q') kQ = true;
    if (key == 'e' || key == 'E') kE = true;

    if (key == ' ') fireProjectile();
    if (key == 'r' || key == 'R') resetWorld();
    if (key == 'g' || key == 'G') showGrid = !showGrid; // Toggle grid
}

void ofApp::keyReleased(int key) {
    if (key == 'w' || key == 'W') kW = false;
    if (key == 's' || key == 'S') kS = false;
    if (key == 'a' || key == 'A') kA = false;
    if (key == 'd' || key == 'D') kD = false;
    if (key == 'q' || key == 'Q') kQ = false;
    if (key == 'e' || key == 'E') kE = false;
}

void ofApp::mouseDragged(int x, int y, int button) {
    static int lastX = x, lastY = y;
    int dx = x - lastX;
    int dy = y - lastY;
    lastX = x; lastY = y;

    float yaw = -dx * shipTurnSpeed;
    float pitch = -dy * shipTurnSpeed;

    ofQuaternion qYaw, qPitch;
    qYaw.makeRotate(ofRadToDeg(yaw), 0, 1, 0);
    qPitch.makeRotate(ofRadToDeg(pitch), 1, 0, 0);

    shipRot = qYaw * shipRot * qPitch;
}