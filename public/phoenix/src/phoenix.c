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

// Sound variables
Sound shootSound;
Sound birdHitSound;
Sound playerHitSound;
Sound waveSpawnSound;
Sound gameOverSound;
Music gameMusic;

// --- Music Play Once Flag ---
bool musicPlayedOnce = false;
// -----------------------------

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
            // Spawn bullet slightly ahead of the player sprite's visual center
            bullets[i].position = (Vector3){ playerPos.x, 0.0f, playerPos.z - 1.5f }; // Adjusted Z offset
            if (IsSoundValid(shootSound)) PlaySound(shootSound); // Check before playing
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
    int rows = sizeof(waveLayout) / sizeof(waveLayout[0]); // Dynamically get rows from layout
    float spacingX = 4.0f;
    float spacingZ = 2.0f;

    // Determine wave type based on pattern (0 = normal, 1 = aggressive)
    bool aggressiveWave = wavePattern[currentWave % WAVE_PATTERN_LENGTH];

    int birdIndex = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < columns; c++) {
             if (birdIndex >= MAX_BIRDS) break; // Stop if we exceed MAX_BIRDS

             if (waveLayout[r][c] == 1) { // Only initialize active birds based on layout
                 birds[birdIndex].formationOffset = (Vector3){ (c - (columns/2)) * spacingX, 0.0f, r * spacingZ };
                 birds[birdIndex].active = true;
                 birds[birdIndex].attacking = false;

                 // Size varies by wave type
                 if (aggressiveWave)
                     birds[birdIndex].size = GetRandomValue(15, 20) / 10.0f;  // Larger birds for aggressive waves
                 else
                     birds[birdIndex].size = GetRandomValue(10, 15) / 10.0f;  // Normal sized birds

                 // Set initial position in formation (start further back)
                 birds[birdIndex].position.x = formationX + birds[birdIndex].formationOffset.x;
                 birds[birdIndex].position.y = 0.0f;
                 birds[birdIndex].position.z = formationBaseZ + birds[birdIndex].formationOffset.z - 40; // Start further back
                 birds[birdIndex].velocity = (Vector3){0.0f, 0.0f, 0.0f};

                 // Add unpredictable movement factors
                 birds[birdIndex].wobbleFactor = GetRandomValue(50, 150) / 100.0f;  // Random wobble amount between 0.5 and 1.5
                 birds[birdIndex].wobbleSpeed = GetRandomValue(100, 300) / 100.0f;  // Random wobble speed between 1.0 and 3.0

                 // Bird color based on wave type
                 if (aggressiveWave) {
                     int colorChoice = GetRandomValue(0, 2);
                     switch (colorChoice)
                     {
                         case 0: birds[birdIndex].color = RED; break;
                         case 1: birds[birdIndex].color = ORANGE; break;
                         case 2: birds[birdIndex].color = MAROON; break;
                     }
                 } else {
                     int colorChoice = GetRandomValue(0, 4);
                     switch (colorChoice)
                     {
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

     // Deactivate any remaining birds if waveLayout uses fewer than MAX_BIRDS
    for (int i = birdIndex; i < MAX_BIRDS; i++) {
        birds[i].active = false;
    }


    // Update wave counter for next wave
    currentWave++;

    // Reset respawn flags
    allBirdsDestroyed = false;
    respawnDelay = 0;
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
                    // Adjusted collision check: Use bird size, bullet radius might be less relevant
                    if (CalculateDistance(bullets[i].position, birds[j].position) < (birds[j].size * 0.8f)) // Hitbox based mainly on bird size
                    {
                        bullets[i].active = false;
                        birds[j].active = false;
                        score++;
                        if (IsSoundValid(birdHitSound)) PlaySound(birdHitSound); // Check before playing
                        break; // Bullet can only hit one bird
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
            // Adjusted player collision size to roughly match the sprite's visual extent
            float playerCollisionRadius = 1.5f; // Tunable parameter
            if (CalculateDistance(playerPos, birds[i].position) < (playerCollisionRadius + birds[i].size * 0.7f)) // Combine radii
            {
                lives--;
                invincibilityFrames = 120;  // 2 seconds at 60 FPS
                if (IsSoundValid(playerHitSound)) PlaySound(playerHitSound); // Check before playing

                // Make the bird inactive instead of moving it, consistent with bullet hits
                birds[i].active = false;

                if (lives <= 0) {
                    gameOver = true;
                    if (IsSoundValid(gameOverSound)) PlaySound(gameOverSound); // Check before playing
                    if (IsMusicValid(gameMusic)) StopMusicStream(gameMusic); // Explicitly stop music on game over
                    musicPlayedOnce = true; // Prevent music restart attempts if game over happens before music ends
                }
                 // Break after one hit per frame to avoid instant death
                 break;
            }
        }
    }
}

// Check if all birds are destroyed and handle respawning
static void CheckAndRespawnBirds(void)
{
    // If already waiting to respawn, update counter
    if (allBirdsDestroyed)
    {
        respawnDelay++;
        if (respawnDelay >= RESPAWN_DELAY_FRAMES)
        {
            InitBirds();  // Respawn birds after delay
        }
        return;
    }

    // Check if all *initial* birds are destroyed
    bool anyBirdActive = false;
    for (int i = 0; i < MAX_BIRDS; i++)
    {
        // Only consider birds that were initially active in the waveLayout
        // This prevents checking placeholder inactive birds
        int r = i / 5; // Assuming 5 columns
        int c = i % 5;
        if (r < (sizeof(waveLayout) / sizeof(waveLayout[0])) && waveLayout[r][c] == 1) {
            if (birds[i].active) {
                anyBirdActive = true;
                break;
            }
        }
    }

    // If no birds *that started active* are left, set flag to start respawn delay
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
    currentWave = 0; // Reset wave counter fully
    allBirdsDestroyed = false;
    respawnDelay = 0;
    invincibilityFrames = 0;

    for (int i = 0; i < MAX_BULLETS; i++) { bullets[i].active = false; }
    InitBirds(); // This will now correctly initialize wave 1 (currentWave=0 before increment)

    // Reset and play music if it was loaded, ensuring it plays from the start
    //if (IsMusicValid(gameMusic)) {
    //    StopMusicStream(gameMusic); // Stop just in case it was playing
    //    PlayMusicStream(gameMusic);
    //    musicPlayedOnce = false; // <<< RESET MUSIC FLAG HERE
    //} else {
    //    musicPlayedOnce = true; // If music isn't valid, act as if it already played
    //}
}

// Load game sounds
static void LoadGameSounds(void)
{
    // Ensure files exist or handle errors
    if (FileExists("resources/shoot.mp3")) shootSound = LoadSound("resources/shoot.mp3");
    if (FileExists("resources/birdhit.mp3")) birdHitSound = LoadSound("resources/birdhit.mp3");
    if (FileExists("resources/playerhit.mp3")) playerHitSound = LoadSound("resources/playerhit.mp3");
    if (FileExists("resources/wave.mp3")) waveSpawnSound = LoadSound("resources/wave.mp3");
    if (FileExists("resources/gameover.mp3")) gameOverSound = LoadSound("resources/gameover.mp3");
    //if (FileExists("resources/gamemusic.mp3")) gameMusic = LoadMusicStream("resources/gamemusic.mp3");

    // Set volumes only if sounds/music were loaded successfully
    if (IsSoundValid(shootSound)) SetSoundVolume(shootSound, 0.7f);
    if (IsSoundValid(birdHitSound)) SetSoundVolume(birdHitSound, 0.8f);
    if (IsSoundValid(playerHitSound)) SetSoundVolume(playerHitSound, 0.9f);
    if (IsSoundValid(waveSpawnSound)) SetSoundVolume(waveSpawnSound, 0.8f);
    if (IsSoundValid(gameOverSound)) SetSoundVolume(gameOverSound, 1.0f);
    //if (IsMusicValid(gameMusic)) {
    //    SetMusicVolume(gameMusic, 0.5f);
        // Don't PlayMusicStream here, do it in ResetGame to ensure it starts fresh
   // }
}

// Unload game sounds
static void UnloadGameSounds(void)
{
    // Unload only if the handle is valid
    if (IsSoundValid(shootSound)) UnloadSound(shootSound);
    if (IsSoundValid(birdHitSound)) UnloadSound(birdHitSound);
    if (IsSoundValid(playerHitSound)) UnloadSound(playerHitSound);
    if (IsSoundValid(waveSpawnSound)) UnloadSound(waveSpawnSound);
    if (IsSoundValid(gameOverSound)) UnloadSound(gameOverSound);
    //if (IsMusicValid(gameMusic)) UnloadMusicStream(gameMusic);
}

int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "raylib - phoenix bird shooter");
    InitAudioDevice();

    // --- Load Player Texture ---
    if (FileExists("resources/spaceship.png")) {
         playerTexture = LoadTexture("resources/spaceship.png");
    } else {
         TraceLog(LOG_WARNING, "Failed to load spaceship.png");
         Image fallbackImg = GenImageColor(32, 32, RED);
         playerTexture = LoadTextureFromImage(fallbackImg);
         UnloadImage(fallbackImg);
    }
    if (!IsTextureValid(playerTexture)) {
        TraceLog(LOG_ERROR, "Player texture is not ready after loading attempt!");
    }
    // -------------------------

    LoadGameSounds();

    // --- Setup Orthographic Camera for constant size ---
    camera.position = (Vector3){ 0.0f, 25.0f, 15.0f }; // Y is height, Z is less critical now but keep somewhat back
    camera.target   = (Vector3){ 0.0f, 0.0f, 0.0f };  // Look at the center of the play area origin
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };  // Y is up
    camera.fovy     = 40.0f;                          // Controls vertical view size (zoom). Adjust this value!
                                                      // Smaller fovy = zoomed in, larger fovy = zoomed out.
    camera.projection = CAMERA_ORTHOGRAPHIC;          // Use Orthographic projection
    // ---------------------------------------------------

    // Start the game state (this will play music for the first time)
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

    // --- Unload Player Texture ---
    UnloadTexture(playerTexture);
    // ---------------------------

    UnloadGameSounds();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

static void UpdateDrawFrame(void)
{
    // --- Update Music Stream (Only Once) ---
    //if (IsMusicValid(gameMusic) && !musicPlayedOnce) { // Check flag
    //    UpdateMusicStream(gameMusic);
    //    // Check if music finished playing
    //    if (GetMusicTimePlayed(gameMusic) >= GetMusicTimeLength(gameMusic)) {
    //        musicPlayedOnce = true; // Set flag when done
    //         // Optional: Stop stream explicitly to release resources? Usually Update handles this.
    //         StopMusicStream(gameMusic);
    //    }
    //}
    // ---------------------------------------

    // --- Input Handling for Player ---
    if (!gameOver)
    {
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  playerPos.x -= playerSpeed;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) playerPos.x += playerSpeed;

        // Constrain player
        float playerHalfWidth = 1.5f; // Approx half-width based on billboard size used below
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
    else  // Game Over; wait for restart
    {
        if (IsKeyPressed(KEY_R) || IsGestureTap()) ResetGame();
    }

    // --- Formation & Attack Update for Birds (only if game is not over) ---
    if (!gameOver)
    {
        if (!allBirdsDestroyed) CheckAndRespawnBirds();
        else {
             respawnDelay++;
             if (respawnDelay >= RESPAWN_DELAY_FRAMES) {
                 InitBirds();
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

                        if (IsSoundValid(waveSpawnSound)) PlaySound(waveSpawnSound);

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
            // Draw the player ship using the texture as a billboard
            // With orthographic camera, the size parameter (3.0f) will result in a consistent visual size.
            if (!gameOver && IsTextureValid(playerTexture))
            {
                Color playerTint = WHITE;
                if (invincibilityFrames > 0 && (invincibilityFrames/10) % 2 == 0) {
                    playerTint = GRAY;
                }
                // DrawBillboard size is now consistent visually due to Orthographic projection
                DrawBillboard(camera, playerTexture, playerPos, 3.0f, playerTint);
            }

            // Draw bullets
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active)
                {
                    DrawSphere(bullets[i].position, bullets[i].radius, YELLOW);
                }
            }

            // Draw birds
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
             // Optional subtle grid
             // DrawGrid(40, 2.0f);

        EndMode3D();

        // --- UI Overlay ---
        DrawText(TextFormat("SCORE: %04i", score), 10, 10, 20, RAYWHITE);
        DrawText(TextFormat("LIVES: %i", lives), GetScreenWidth() - 100, 10, 20, RAYWHITE);

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
            DrawText("GAME OVER", GetScreenWidth()/2 - MeasureText("GAME OVER", 50)/2,
                     GetScreenHeight()/2 - 60, 50, RED);
            DrawText(TextFormat("FINAL SCORE: %i", score),
                     GetScreenWidth()/2 - MeasureText(TextFormat("FINAL SCORE: %i", score), 30)/2,
                     GetScreenHeight()/2 + 0, 30, RAYWHITE);
            DrawText("PRESS R or TAP TO RESTART", GetScreenWidth()/2 - MeasureText("PRESS R or TAP TO RESTART", 20)/2,
                     GetScreenHeight()/2 + 40, 20, LIGHTGRAY);
        }

        DrawFPS(GetScreenWidth() - 90, GetScreenHeight() - 30);
    EndDrawing();
}
