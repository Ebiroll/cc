#include "raylib.h"
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define MAX_BULLETS 32
#define MAX_BIRDS 15
#define MAX_LIVES 3
#define WAVE_PATTERN_LENGTH 9  // Length of the binary pattern {001101011}

// --- Texture for the player ship ---
Texture2D playerTexture;
// -----------------------------------

// New Bird structure fields for formation and attack behavior.
typedef struct Bird {
    Vector3 position;
    Vector3 velocity;       // Used only when bird is attacking
    bool active;
    bool attacking;         // Whether the bird is currently in attack mode
    Color color;
    float size;
    Vector3 formationOffset;  // Position offset relative to the formation center
    float wobbleFactor;       // Individual wobble amount for unpredictable movement
    float wobbleSpeed;        // Individual speed of wobble
} Bird;

typedef struct Bullet {
    Vector3 position;
    bool active;
    float radius;
} Bullet;

Bullet bullets[MAX_BULLETS] = {0};
Bird birds[MAX_BIRDS] = {0};

Camera camera = { 0 };
Vector3 playerPos = { 0.0f, 0.0f, 8.0f };  // Player position
float playerSpeed = 0.4f;
float bulletSpeed = 0.8f;
float playAreaWidth = 30.0f;
float playAreaHeight = 20.0f;
int score = 0;
int lives = MAX_LIVES;
bool gameOver = false;
int invincibilityFrames = 0;  // Invincibility after being hit

// Formation globals: the entire bird formation will be anchored at (formationX, formationBaseZ)
float formationX = 0.0f;
float formationBaseZ = -8.0f;
float formationSpeed = 0.2f;
int formationDirection = 1;  // 1 = moving right, -1 = moving left

// Wave respawn pattern (binary representation: 001101011)
int wavePattern[WAVE_PATTERN_LENGTH] = {0, 0, 1, 1, 0, 1, 0, 1, 1};

// Reduced wave layout for testing (adjust MAX_BIRDS if needed)
int waveLayout[2][5] = { // Assuming MAX_BIRDS is at least 10
    {1, 0, 1, 0, 1},
    {1, 1, 1, 1, 1}
};


int currentWave = 0;
bool allBirdsDestroyed = false;
int respawnDelay = 0;
const int RESPAWN_DELAY_FRAMES = 180;  // 3 seconds at 60 FPS

// --- Sound variables ---
Sound shootSound;
Sound birdHitSound;
Sound playerHitSound;
Sound birdAttackSound; // Renamed from waveSpawnSound for clarity
Sound gameOverSound;
Sound waveStartSound; // Changed from Music gameMusic
// ---------------------

static void UpdateDrawFrame(void);
static void InitBirds(void);
static void CheckCollisions(void);
static void CheckPlayerBirdCollisions(void);
static void ResetGame(void);
static void CheckAndRespawnBirds(void);
static void LoadGameSounds(void);
static void UnloadGameSounds(void);

// Custom distance function
float CalculateDistance(Vector3 v1, Vector3 v2)
{
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;

    return sqrtf(dx*dx + dy*dy + dz*dz);
}

bool IsGestureTap(void) {
    return IsGestureDetected(GESTURE_TAP);
}

// Spawn a bullet from the player's current position
static void SpawnBullet(void)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].active = true;
            bullets[i].radius = 0.5f;
            bullets[i].position = (Vector3){ playerPos.x, 0.0f, playerPos.z - 1.5f };
            if (IsSoundValid(shootSound)) PlaySound(shootSound);
            break;
        }
    }
}

// Initialize birds into a formation based on waveLayout
static void InitBirds(void)
{
    formationX = 0.0f;
    formationBaseZ = -5.0f;
    int columns = 5;
    int rows = sizeof(waveLayout) / sizeof(waveLayout[0]);
    float spacingX = 4.0f;
    float spacingZ = 2.0f;

    // Update wave counter *before* checking pattern for the *new* wave
    currentWave++;
    bool aggressiveWave = wavePattern[(currentWave - 1) % WAVE_PATTERN_LENGTH]; // Use currentWave-1 for pattern index

    int birdIndex = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < columns; c++) {
             if (birdIndex >= MAX_BIRDS) break;

             if (waveLayout[r][c] == 1) {
                 birds[birdIndex].formationOffset = (Vector3){ (c - (columns/2)) * spacingX, 0.0f, r * spacingZ };
                 birds[birdIndex].active = true;
                 birds[birdIndex].attacking = false;

                 if (aggressiveWave) birds[birdIndex].size = GetRandomValue(15, 20) / 10.0f;
                 else birds[birdIndex].size = GetRandomValue(10, 15) / 10.0f;

                 birds[birdIndex].position.x = formationX + birds[birdIndex].formationOffset.x;
                 birds[birdIndex].position.y = 0.0f;
                 birds[birdIndex].position.z = formationBaseZ + birds[birdIndex].formationOffset.z - 40;
                 birds[birdIndex].velocity = (Vector3){0.0f, 0.0f, 0.0f};

                 birds[birdIndex].wobbleFactor = GetRandomValue(50, 150) / 100.0f;
                 birds[birdIndex].wobbleSpeed = GetRandomValue(100, 300) / 100.0f;

                 if (aggressiveWave) {
                     int colorChoice = GetRandomValue(0, 2);
                     switch (colorChoice) {
                         case 0: birds[birdIndex].color = RED; break;
                         case 1: birds[birdIndex].color = ORANGE; break;
                         case 2: birds[birdIndex].color = MAROON; break;
                     }
                 } else {
                     int colorChoice = GetRandomValue(0, 4);
                     switch (colorChoice) {
                         case 0: birds[birdIndex].color = GREEN; break;
                         case 1: birds[birdIndex].color = BLUE; break;
                         case 2: birds[birdIndex].color = PURPLE; break;
                         case 3: birds[birdIndex].color = SKYBLUE; break;
                         case 4: birds[birdIndex].color = LIME; break;
                     }
                 }
                 birdIndex++;
             }
        }
         if (birdIndex >= MAX_BIRDS) break;
    }

    for (int i = birdIndex; i < MAX_BIRDS; i++) {
        birds[i].active = false;
    }

    // Reset respawn flags
    allBirdsDestroyed = false;
    respawnDelay = 0;

    // --- Play Wave Start Sound ---
    if (IsSoundValid(waveStartSound)) {
        PlaySound(waveStartSound);
    }
    // ---------------------------
}


// Check for collisions between bullets and birds.
static void CheckCollisions(void)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            for (int j = 0; j < MAX_BIRDS; j++)
            {
                if (birds[j].active)
                {
                    if (CalculateDistance(bullets[i].position, birds[j].position) < (birds[j].size * 0.8f))
                    {
                        bullets[i].active = false;
                        birds[j].active = false;
                        score++;
                        if (IsSoundValid(birdHitSound)) PlaySound(birdHitSound);
                        break;
                    }
                }
            }
        }
    }
}

// Check for collisions between player and birds.
static void CheckPlayerBirdCollisions(void)
{
    if (invincibilityFrames > 0) return;

    for (int i = 0; i < MAX_BIRDS; i++)
    {
        if (birds[i].active)
        {
            float playerCollisionRadius = 1.5f;
            if (CalculateDistance(playerPos, birds[i].position) < (playerCollisionRadius + birds[i].size * 0.7f))
            {
                lives--;
                invincibilityFrames = 120;
                if (IsSoundValid(playerHitSound)) PlaySound(playerHitSound);

                birds[i].active = false;

                if (lives <= 0) {
                    gameOver = true;
                    if (IsSoundValid(gameOverSound)) PlaySound(gameOverSound);
                    // No need to stop music stream anymore
                }
                 break;
            }
        }
    }
}

// Check if all birds are destroyed and handle respawning
static void CheckAndRespawnBirds(void)
{
    if (allBirdsDestroyed)
    {
        respawnDelay++;
        if (respawnDelay >= RESPAWN_DELAY_FRAMES)
        {
            InitBirds(); // This will trigger the next wave sound
        }
        return;
    }

    bool anyBirdActive = false;
    for (int i = 0; i < MAX_BIRDS; i++)
    {
        int r = i / 5;
        int c = i % 5;
        if (r < (sizeof(waveLayout) / sizeof(waveLayout[0])) && waveLayout[r][c] == 1) {
            if (birds[i].active) {
                anyBirdActive = true;
                break;
            }
        }
    }

    if (!anyBirdActive)
    {
        allBirdsDestroyed = true;
        respawnDelay = 0;
    }
}


// Reset the game state
static void ResetGame(void)
{
    score = 0;
    lives = MAX_LIVES;
    gameOver = false;
    playerPos = (Vector3){ 0.0f, 0.0f, 8.0f };
    currentWave = 0; // Reset wave counter, InitBirds will increment to 1
    allBirdsDestroyed = false;
    respawnDelay = 0;
    invincibilityFrames = 0;

    for (int i = 0; i < MAX_BULLETS; i++) { bullets[i].active = false; }

    // InitBirds will be called right after this or at start, playing the first wave sound
    InitBirds();

    // No music flag reset needed anymore
}

// Load game sounds
static void LoadGameSounds(void)
{
    // --- Use resources/ path ---
    if (FileExists("resources/shoot.mp3")) shootSound = LoadSound("resources/shoot.mp3");
    if (FileExists("resources/birdhit.mp3")) birdHitSound = LoadSound("resources/birdhit.mp3");
    if (FileExists("resources/playerhit.mp3")) playerHitSound = LoadSound("resources/playerhit.mp3");
    if (FileExists("resources/wave.mp3")) birdAttackSound = LoadSound("resources/wave.mp3"); // Keep this sound for attacks maybe?
    if (FileExists("resources/gameover.mp3")) gameOverSound = LoadSound("resources/gameover.mp3");
    // Load former music file as a Sound
    if (FileExists("resources/gamemusic.mp3")) waveStartSound = LoadSound("resources/gamemusic.mp3");
    // ---------------------------

    // Set volumes
    if (IsSoundValid(shootSound)) SetSoundVolume(shootSound, 0.7f);
    if (IsSoundValid(birdHitSound)) SetSoundVolume(birdHitSound, 0.8f);
    if (IsSoundValid(playerHitSound)) SetSoundVolume(playerHitSound, 0.9f);
    if (IsSoundValid(birdAttackSound)) SetSoundVolume(birdAttackSound, 0.8f); // Keep volume setting for this
    if (IsSoundValid(gameOverSound)) SetSoundVolume(gameOverSound, 1.0f);
    // Set volume for the new wave start sound
    if (IsSoundValid(waveStartSound)) SetSoundVolume(waveStartSound, 0.6f); // Adjust volume as needed

    // No PlayMusicStream needed here
}

// Unload game sounds
static void UnloadGameSounds(void)
{
    if (IsSoundValid(shootSound)) UnloadSound(shootSound);
    if (IsSoundValid(birdHitSound)) UnloadSound(birdHitSound);
    if (IsSoundValid(playerHitSound)) UnloadSound(playerHitSound);
    if (IsSoundValid(birdAttackSound)) UnloadSound(birdAttackSound);
    if (IsSoundValid(gameOverSound)) UnloadSound(gameOverSound);
    // Unload the wave start sound
    if (IsSoundValid(waveStartSound)) UnloadSound(waveStartSound);
    // No UnloadMusicStream needed
}

int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "raylib - phoenix bird shooter");
    // NOTE: InitAudioDevice might trigger the web console warning mentioned by the user.
    // This is expected browser behavior. Audio should resume after first user interaction.
    InitAudioDevice();

    // --- Load Player Texture from resources/ ---
    const char *playerTexturePath = "resources/spaceship.png";
    if (FileExists(playerTexturePath)) {
         playerTexture = LoadTexture(playerTexturePath);
    } else {
         TraceLog(LOG_WARNING, "Failed to load %s", playerTexturePath);
         Image fallbackImg = GenImageColor(32, 32, RED);
         playerTexture = LoadTextureFromImage(fallbackImg);
         UnloadImage(fallbackImg);
    }
    if (!IsTextureValid(playerTexture)) {
        TraceLog(LOG_ERROR, "Player texture is not ready after loading attempt!");
    }
    // ----------------------------------------

    LoadGameSounds();

    // Setup Orthographic Camera for constant size
    camera.position = (Vector3){ 0.0f, 25.0f, 15.0f };
    camera.target   = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy     = 40.0f; // Adjust for desired zoom level
    camera.projection = CAMERA_ORTHOGRAPHIC;

    // Start the game state (this will call InitBirds and play the first wave sound)
    ResetGame();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
#endif

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    UnloadTexture(playerTexture);
    UnloadGameSounds();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

static void UpdateDrawFrame(void)
{
    // --- Music Update Removed ---
    // No UpdateMusicStream needed anymore

    // --- Input Handling ---
    if (!gameOver)
    {
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  playerPos.x -= playerSpeed;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) playerPos.x += playerSpeed;

        float playerHalfWidth = 1.5f;
        if (playerPos.x < -playAreaWidth/2 + playerHalfWidth) playerPos.x = -playAreaWidth/2 + playerHalfWidth;
        if (playerPos.x > playAreaWidth/2 - playerHalfWidth) playerPos.x = playAreaWidth/2 - playerHalfWidth;

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            Vector2 touchPos = GetMousePosition();
            if (touchPos.x < GetScreenWidth() / 2) playerPos.x -= playerSpeed;
            else playerPos.x += playerSpeed;
        }

        if (IsKeyPressed(KEY_SPACE) || IsGestureTap())
        {
            SpawnBullet();
        }

        if (invincibilityFrames > 0) invincibilityFrames--;
    }
    else
    {
        if (IsKeyPressed(KEY_R) || IsGestureTap()) ResetGame();
    }

    // --- Bird Updates ---
    if (!gameOver)
    {
        if (!allBirdsDestroyed) CheckAndRespawnBirds();
        else {
             respawnDelay++;
             if (respawnDelay >= RESPAWN_DELAY_FRAMES) {
                 // InitBirds() called within CheckAndRespawnBirds will play the next wave sound
                 CheckAndRespawnBirds();
             }
        }

        formationX += formationSpeed * formationDirection;
        float formationEdgeOffset = 4.0f * (5/2);
        if (formationX - formationEdgeOffset < -playAreaWidth/2) {
            formationX = -playAreaWidth/2 + formationEdgeOffset;
            formationDirection = 1;
        }
        if (formationX + formationEdgeOffset > playAreaWidth/2) {
            formationX = playAreaWidth/2 - formationEdgeOffset;
            formationDirection = -1;
        }

        // Use currentWave-1 because currentWave was incremented at the start of InitBirds
        bool aggressiveWave = (currentWave > 0) && wavePattern[(currentWave - 1) % WAVE_PATTERN_LENGTH];


        for (int i = 0; i < MAX_BIRDS; i++)
        {
            if (birds[i].active && !birds[i].attacking)
            {
                 float targetZ = formationBaseZ + birds[i].formationOffset.z;
                 if (birds[i].position.z < targetZ) {
                     birds[i].position.z += 0.2f;
                     if (birds[i].position.z > targetZ) birds[i].position.z = targetZ;
                 }

                if (fabsf(birds[i].position.z - targetZ) < 1.0f) {
                    float wobbleX = sinf(GetTime() * birds[i].wobbleSpeed) * birds[i].wobbleFactor;
                    float wobbleZ = cosf(GetTime() * birds[i].wobbleSpeed * 0.7f) * birds[i].wobbleFactor * 0.5f;
                    birds[i].position.x = formationX + birds[i].formationOffset.x + wobbleX;
                    birds[i].position.z = targetZ + wobbleZ;
                } else {
                     birds[i].position.x = formationX + birds[i].formationOffset.x;
                }
            }
        }

        int attackingCount = 0;
        for (int i = 0; i < MAX_BIRDS; i++)
        {
            if (birds[i].active && birds[i].attacking) attackingCount++;
        }

        int maxAttacking = aggressiveWave ? 4 : 2;

         bool canAttack = false;
         for(int i = 0; i < MAX_BIRDS; ++i) {
             if (birds[i].active && !birds[i].attacking && fabsf(birds[i].position.z - (formationBaseZ + birds[i].formationOffset.z)) < 1.0f) {
                 canAttack = true;
                 break;
             }
         }

        if (canAttack && attackingCount < maxAttacking)
        {
            for (int i = 0; i < MAX_BIRDS; i++)
            {
                 if (birds[i].active && !birds[i].attacking && fabsf(birds[i].position.z - (formationBaseZ + birds[i].formationOffset.z)) < 1.0f)
                {
                    int attackChance = aggressiveWave ? 15 : 5;
                    if (GetRandomValue(0, 5000) < attackChance)
                    {
                        birds[i].attacking = true;

                        // Play bird attack sound
                        if (IsSoundValid(birdAttackSound)) PlaySound(birdAttackSound);

                        float targetX = playerPos.x + GetRandomValue(-20, 20) / 10.0f;
                        float deltaX = targetX - birds[i].position.x;
                        float dist = CalculateDistance(birds[i].position, (Vector3){targetX, 0.0f, playerPos.z});
                        float attackSpeedZ = aggressiveWave ? 0.4f : 0.3f;
                        float attackSpeedX = (dist > 0.1f) ? (deltaX / dist) * attackSpeedZ * 0.8f : 0.0f;
                        birds[i].velocity.x = attackSpeedX;
                        birds[i].velocity.z = attackSpeedZ;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_BIRDS; i++)
        {
            if (birds[i].active && birds[i].attacking)
            {
                birds[i].position.x += birds[i].velocity.x;
                birds[i].position.z += birds[i].velocity.z;
                birds[i].position.x += sinf(GetTime() * 8.0f + i) * 0.05f;

                if (birds[i].position.z > playerPos.z + 5.0f)
                {
                     birds[i].active = false;
                }
            }
        }
    }

    // --- Bullet Updates ---
    if (!gameOver)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (bullets[i].active)
            {
                bullets[i].position.z -= bulletSpeed;
                if (bullets[i].position.z < -playAreaHeight - 5.0f)
                    bullets[i].active = false;
            }
        }
    }

    // --- Check for Collisions ---
    if (!gameOver)
    {
        CheckCollisions();
        CheckPlayerBirdCollisions();
    }

    // --- Drawing ---
    BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
            if (!gameOver && IsTextureValid(playerTexture))
            {
                Color playerTint = WHITE;
                if (invincibilityFrames > 0 && (invincibilityFrames/10) % 2 == 0) {
                    playerTint = GRAY;
                }
                DrawBillboard(camera, playerTexture, playerPos, 3.0f, playerTint);
            }

            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active) DrawSphere(bullets[i].position, bullets[i].radius, YELLOW);
            }

            for (int i = 0; i < MAX_BIRDS; i++)
            {
                if (birds[i].active)
                {
                    Vector3 birdPos = birds[i].position;
                    float size = birds[i].size;
                    Color birdColor = birds[i].color;
                    DrawCube(birdPos, size, size * 0.3f, size, birdColor);
                    float wingOffset = sinf(GetTime() * 15.0f + i * 0.5f) * 0.4f + 0.6f;
                    DrawCube((Vector3){birdPos.x - size*0.6f, birdPos.y, birdPos.z + size * 0.2f},
                             size * 0.7f, size * 0.2f, size * 0.5f * wingOffset, birdColor);
                    DrawCube((Vector3){birdPos.x + size*0.6f, birdPos.y, birdPos.z + size * 0.2f},
                             size * 0.7f, size * 0.2f, size * 0.5f * wingOffset, birdColor);
                    DrawCubeWires(birdPos, size, size * 0.3f, size, ColorBrightness(birdColor, -0.5f));
                }
            }
            // DrawGrid(40, 2.0f);
        EndMode3D();

        // --- UI Overlay ---
        DrawText(TextFormat("SCORE: %04i", score), 10, 10, 20, RAYWHITE);
        DrawText(TextFormat("LIVES: %i", lives), GetScreenWidth() - 100, 10, 20, RAYWHITE);
        // Use currentWave-1 for display pattern check, consistent with InitBirds logic
        bool isAggressive = (currentWave > 0) && wavePattern[(currentWave - 1) % WAVE_PATTERN_LENGTH];
        DrawText(TextFormat("WAVE: %i %s", currentWave, isAggressive ? "[AGGRESSIVE]" : ""), 10, 35, 20, isAggressive ? RED : LIGHTGRAY);

        if (score == 0 && lives == MAX_LIVES && currentWave <= 1) {
             DrawText("Arrows/AD/Touch: Move | Space/Tap: Shoot", 10, GetScreenHeight() - 30, 20, GRAY);
        }

        if (allBirdsDestroyed && !gameOver)
        {
            int countdownSeconds = (RESPAWN_DELAY_FRAMES - respawnDelay) / 60 + 1;
             DrawText(TextFormat("WAVE CLEARED! NEXT WAVE IN: %i", countdownSeconds),
                      GetScreenWidth()/2 - MeasureText(TextFormat("WAVE CLEARED! NEXT WAVE IN: %i", countdownSeconds), 20)/2,
                      GetScreenHeight()/2 - 10, 20, GREEN);
        }

        if (gameOver)
        {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f));
            DrawText("GAME OVER", GetScreenWidth()/2 - MeasureText("GAME OVER", 50)/2, GetScreenHeight()/2 - 60, 50, RED);
            DrawText(TextFormat("FINAL SCORE: %i", score), GetScreenWidth()/2 - MeasureText(TextFormat("FINAL SCORE: %i", score), 30)/2, GetScreenHeight()/2 + 0, 30, RAYWHITE);
            DrawText("PRESS R or TAP TO RESTART", GetScreenWidth()/2 - MeasureText("PRESS R or TAP TO RESTART", 20)/2, GetScreenHeight()/2 + 40, 20, LIGHTGRAY);
        }

        DrawFPS(GetScreenWidth() - 90, GetScreenHeight() - 30);
    EndDrawing();
}