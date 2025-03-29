#include "raylib.h"
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define MAX_BULLETS 32
#define MAX_BIRDS 15
#define MAX_LIVES 3
#define WAVE_PATTERN_LENGTH 9  // Length of the binary pattern {001101011}

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

int waveLayout[3][5] = {
    {1, 0, 0, 0, 1},
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
            // Spawn bullet ahead of the player
            bullets[i].position = (Vector3){ playerPos.x, 0.0f, playerPos.z - 1.0f };
            PlaySound(shootSound);
            break;
        }
    }
}

// Initialize birds into a neat formation (2 rows x 5 columns) with unpredictable movement factors
static void InitBirds(void)
{
    formationX = 0.0f;
    formationBaseZ = -5.0f;
    int columns = 5;
    int rows = 3;  // MAX_BIRDS assumed to be 15
    float spacingX = 4.0f;
    float spacingZ = 2.0f;
    
    // Determine wave type based on pattern (0 = normal, 1 = aggressive)
    bool aggressiveWave = wavePattern[currentWave % WAVE_PATTERN_LENGTH];
    
    for (int i = 0; i < MAX_BIRDS; i++)
    {
         int row = i / columns;
         int col = i % columns;
         birds[i].formationOffset = (Vector3){ (col - (columns/2)) * spacingX, 0.0f, row * spacingZ };
         birds[i].active = true;
         birds[i].attacking = false;
         
         // Size varies by wave type
         if (aggressiveWave)
             birds[i].size = GetRandomValue(15, 20) / 10.0f;  // Larger birds for aggressive waves
         else
             birds[i].size = GetRandomValue(10, 15) / 10.0f;  // Normal sized birds
             
         // Set initial position in formation:
         if (waveLayout[row][col] == 0) birds[i].active = false;
         
         birds[i].position.x = formationX + birds[i].formationOffset.x;
         birds[i].position.y = 0.0f;
         birds[i].position.z = formationBaseZ + birds[i].formationOffset.z  - 40;
         birds[i].velocity = (Vector3){0.0f, 0.0f, 0.0f};
         
         // Add unpredictable movement factors
         birds[i].wobbleFactor = GetRandomValue(50, 150) / 100.0f;  // Random wobble amount between 0.5 and 1.5
         birds[i].wobbleSpeed = GetRandomValue(100, 300) / 100.0f;  // Random wobble speed between 1.0 and 3.0

         // Bird color based on wave type
         if (aggressiveWave) {
             // Aggressive wave uses more red/orange hues
             int colorChoice = GetRandomValue(0, 2);
             switch (colorChoice)
             {
                 case 0: birds[i].color = RED; break;
                 case 1: birds[i].color = ORANGE; break;
                 case 2: birds[i].color = MAROON; break;
             }
         } else {
             // Normal wave uses varied colors
             int colorChoice = GetRandomValue(0, 4);
             switch (colorChoice)
             {
                 case 0: birds[i].color = GREEN; break;
                 case 1: birds[i].color = BLUE; break;
                 case 2: birds[i].color = PURPLE; break;
                 case 3: birds[i].color = SKYBLUE; break;
                 case 4: birds[i].color = LIME; break;
             }
         }
    }
    
    
    // Update wave counter for next wave
    currentWave++;
    
    // Reset respawn flags
    allBirdsDestroyed = false;
    respawnDelay = 0;
}

// Check for collisions between bullets and birds.
// When a collision is detected, mark both bullet and bird inactive (no respawn).
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
                    if (CalculateDistance(bullets[i].position, birds[j].position) < (bullets[i].radius + birds[j].size * 0.7f))
                    {
                        bullets[i].active = false;
                        birds[j].active = false;
                        score++;
                        PlaySound(birdHitSound);
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
            float playerSize = 1.0f;  // Approximate collision size
            if (CalculateDistance(playerPos, birds[i].position) < (playerSize + birds[i].size))
            {
                lives--;
                invincibilityFrames = 120;  // 2 seconds at 60 FPS
                PlaySound(playerHitSound);
                
                if (lives <= 0) {
                    gameOver = true;
                    PlaySound(gameOverSound);
                    StopMusicStream(gameMusic);
                }
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
    
    // Check if all birds are destroyed
    bool anyBirdActive = false;
    for (int i = 0; i < MAX_BIRDS; i++)
    {
        if (birds[i].active)
        {
            anyBirdActive = true;
            break;
        }
    }
    
    // If no birds are active, set flag to start respawn delay
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
    currentWave = 0;
    allBirdsDestroyed = false;
    respawnDelay = 0;
    
    for (int i = 0; i < MAX_BULLETS; i++) { bullets[i].active = false; }
    InitBirds();
    
    // Reset music
    //StopMusicStream(gameMusic);
    //PlayMusicStream(gameMusic);
}

// Load game sounds
static void LoadGameSounds(void)
{
    shootSound = LoadSound("resources/shoot.mp3");       // Replace with actual file path
    birdHitSound = LoadSound("resources/birdhit.mp3");   // Replace with actual file path
    playerHitSound = LoadSound("resources/playerhit.mp3"); // Replace with actual file path
    waveSpawnSound = LoadSound("resources/wave.mp3");     // Replace with actual file path
    gameOverSound = LoadSound("resources/gameover.mp3");  // Replace with actual file path
    //gameMusic = LoadMusicStream("resources/gamemusic.mp3"); // Replace with actual file path
    
    SetSoundVolume(shootSound, 0.7f);
    SetSoundVolume(birdHitSound, 0.8f);
    SetSoundVolume(playerHitSound, 0.9f);
    SetSoundVolume(waveSpawnSound, 0.8f);
    SetSoundVolume(gameOverSound, 1.0f);
    SetMusicVolume(gameMusic, 0.5f);
    
    //PlayMusicStream(gameMusic);
}

// Unload game sounds
static void UnloadGameSounds(void)
{
    UnloadSound(shootSound);
    UnloadSound(birdHitSound);
    UnloadSound(playerHitSound);
    UnloadSound(waveSpawnSound);
    UnloadSound(gameOverSound);
    UnloadMusicStream(gameMusic);
}

int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "raylib - phoenix bird shooter");
    InitAudioDevice();
    LoadGameSounds();

    // Adjusted camera so the formation and player are visible
    camera.position = (Vector3){ 0.0f, 30.0f, 0.0f };
    camera.target   = (Vector3){ 0.0f, -2.0f, 0.0f };
    camera.up       = (Vector3){ 0.0f, 0.0f, -1.0f };
    camera.fovy     = 30.0f;
    camera.projection = CAMERA_ORTHOGRAPHIC;

    // Initialize bird formation
    InitBirds();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
#endif

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    UnloadGameSounds();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

static void UpdateDrawFrame(void)
{
    // Update music stream
    UpdateMusicStream(gameMusic);
    
    // --- Input Handling for Player ---
    if (!gameOver)
    {
        if (IsKeyDown(KEY_LEFT))  playerPos.x -= playerSpeed;
        if (IsKeyDown(KEY_RIGHT)) playerPos.x += playerSpeed;
        if (playerPos.x < -playAreaWidth/2) playerPos.x = -playAreaWidth/2;
        if (playerPos.x > playAreaWidth/2) playerPos.x = playAreaWidth/2;
        
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            Vector2 pos = GetMousePosition();
            if (pos.x < GetScreenWidth()/2) playerPos.x -= playerSpeed;
            else                           playerPos.x += playerSpeed;
        }
        
        if (IsKeyPressed(KEY_SPACE) || IsGestureTap())
        {
            SpawnBullet();
        }
        
        if (invincibilityFrames > 0) invincibilityFrames--;
    }
    else  // Game Over; wait for restart
    {
        if (IsKeyPressed(KEY_R)) ResetGame();
    }
    
    // --- Formation & Attack Update for Birds (only if game is not over) ---
    if (!gameOver)
    {
        // Check and handle bird respawning
        CheckAndRespawnBirds();
        
        // Update formation horizontal movement.
        formationX += formationSpeed * formationDirection;
        // Boundaries for formation (assuming formation offsets span about ±8 units)
        if (formationX - 8 < -playAreaWidth/2) formationDirection = 1;
        if (formationX + 8 > playAreaWidth/2)  formationDirection = -1;
        
        // Determine if this is an aggressive wave based on pattern
        bool aggressiveWave = wavePattern[(currentWave - 1) % WAVE_PATTERN_LENGTH];
        
        // For birds still in formation, update their positions relative to the formation anchor.
        for (int i = 0; i < MAX_BIRDS; i++)
        {
            if (birds[i].active && !birds[i].attacking)
            {
                // Add wobble for unpredictable movement
                float wobbleX = sinf(GetTime() * birds[i].wobbleSpeed) * birds[i].wobbleFactor;
                float wobbleZ = cosf(GetTime() * birds[i].wobbleSpeed * 0.7f) * birds[i].wobbleFactor * 0.5f;
                
                birds[i].position.x = formationX + birds[i].formationOffset.x + wobbleX;
                birds[i].position.z = formationBaseZ + birds[i].formationOffset.z + wobbleZ;
            }
        }
        
        // Limit the number of birds attacking concurrently.
        int attackingCount = 0;
        for (int i = 0; i < MAX_BIRDS; i++)
        {
            if (birds[i].active && birds[i].attacking) attackingCount++;
        }
        
        // Adjust max attacking count based on wave type
        int maxAttacking = aggressiveWave ? 4 : 2;  // More birds attack in aggressive waves
        
        if (attackingCount < maxAttacking)
        {
            for (int i = 0; i < MAX_BIRDS; i++)
            {
                if (birds[i].active && !birds[i].attacking)
                {
                    // With a small chance, start an attack (higher chance in aggressive waves)
                    int attackChance = aggressiveWave ? 5 : 2;
                    if (GetRandomValue(0, 1000) < attackChance)
                    {
                        birds[i].attacking = true;

                        // Play wave spawn sound
                        PlaySound(waveSpawnSound);

                        
                        // Set an attack velocity: dive toward the player with a slight horizontal adjustment.
                        float deltaX = playerPos.x - birds[i].position.x;
                        birds[i].velocity.x = deltaX * (aggressiveWave ? 0.03f : 0.02f);  // Faster tracking in aggressive waves
                        birds[i].velocity.z = aggressiveWave ? 0.6f : 0.5f;  // Faster diving in aggressive waves
                    }
                }
            }
        }
        
        // Update positions of attacking birds.
        for (int i = 0; i < MAX_BIRDS; i++)
        {
            if (birds[i].active && birds[i].attacking)
            {
                birds[i].position.x += birds[i].velocity.x;
                birds[i].position.z += birds[i].velocity.z;
                
                // Add some unpredictable movement to attacking birds
                birds[i].position.x += sinf(GetTime() * 10 * birds[i].wobbleSpeed) * 0.05f;
                
                // Once an attacking bird reaches near the player (or passes a threshold),
                // return it to formation.
                if (birds[i].position.z > playerPos.z + 2)
                {
                    birds[i].attacking = false;
                    birds[i].velocity = (Vector3){0.0f, 0.0f, 0.0f};
                    birds[i].position.x = formationX + birds[i].formationOffset.x;
                    birds[i].position.z = formationBaseZ + birds[i].formationOffset.z;
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
                if (bullets[i].position.z < -playAreaHeight - 2.0f)
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
        ClearBackground(RAYWHITE);
        
        BeginMode3D(camera);
            // Draw player spaceship with extra "wings"
            Color playerColor = RED;
            if (invincibilityFrames > 0 && (invincibilityFrames/10) % 2 == 0)
                playerColor = LIGHTGRAY;
            if (!gameOver)
            {
                DrawCube(playerPos, 2.0f, 0.3f, 1.5f, playerColor);
                DrawCubeWires(playerPos, 2.0f, 0.3f, 1.5f, MAROON);
                DrawCube((Vector3){playerPos.x - 1.2f, playerPos.y, playerPos.z + 0.2f}, 
                         0.8f, 0.2f, 0.6f, DARKBLUE);
                DrawCube((Vector3){playerPos.x + 1.2f, playerPos.y, playerPos.z + 0.2f}, 
                         0.8f, 0.2f, 0.6f, DARKBLUE);
            }
            
            // Draw bullets
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active)
                {
                    DrawSphere(bullets[i].position, bullets[i].radius, BLUE);
                }
            }
            
            // Draw birds (only active ones)
            for (int i = 0; i < MAX_BIRDS; i++)
            {
                if (birds[i].active)
                {
                    Vector3 birdPos = birds[i].position;
                    float size = birds[i].size;
                    DrawCube(birdPos, size, size * 0.3f, size, birds[i].color);
                    
                    // Draw simple "wings" that flap using a sine function.
                    float wingOffset = sinf(GetTime() * 10) * 0.5f + 0.5f;
                    DrawCube((Vector3){birdPos.x - size, birdPos.y, birdPos.z}, 
                             size * 0.8f, size * 0.2f, size * 0.8f * wingOffset, birds[i].color);
                    DrawCube((Vector3){birdPos.x + size, birdPos.y, birdPos.z}, 
                             size * 0.8f, size * 0.2f, size * 0.8f * wingOffset, birds[i].color);
                }
            }
            
            // Draw play area boundaries
            DrawLine3D((Vector3){-playAreaWidth/2, 0, -playAreaHeight}, (Vector3){playAreaWidth/2, 0, -playAreaHeight}, RED);
            DrawLine3D((Vector3){-playAreaWidth/2, 0, playerPos.z + 1}, (Vector3){playAreaWidth/2, 0, playerPos.z + 1}, RED);
            DrawLine3D((Vector3){-playAreaWidth/2, 0, -playAreaHeight}, (Vector3){-playAreaWidth/2, 0, playerPos.z + 1}, RED);
            DrawLine3D((Vector3){playAreaWidth/2, 0, -playAreaHeight}, (Vector3){playAreaWidth/2, 0, playerPos.z + 1}, RED);
            
        EndMode3D();
        
        // Overlay text info
        DrawText("Use arrow keys or touch to move, spacebar/tap to shoot", 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("SCORE: %i", score), GetScreenWidth() - 150, 10, 20, BLACK);
        DrawText(TextFormat("LIVES: %i", lives), GetScreenWidth() - 150, 40, 20, BLACK);
        
        // Draw wave information
        bool isAggressive = wavePattern[(currentWave - 1) % WAVE_PATTERN_LENGTH];
        DrawText(TextFormat("WAVE: %i (%s)", currentWave, isAggressive ? "AGGRESSIVE" : "NORMAL"), 10, 70, 20, isAggressive ? RED : DARKGREEN);
        
        // If waiting to respawn, show countdown
        if (allBirdsDestroyed && !gameOver)
        {
            int countdownSeconds = (RESPAWN_DELAY_FRAMES - respawnDelay) / 60 + 1;
            DrawText(TextFormat("NEXT WAVE IN: %i", countdownSeconds), GetScreenWidth()/2 - 100, 40, 20, MAROON);
        }
        
        if (gameOver)
        {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));
            DrawText("GAME OVER", GetScreenWidth()/2 - MeasureText("GAME OVER", 40)/2,
                     GetScreenHeight()/2 - 40, 40, RED);
            DrawText(TextFormat("FINAL SCORE: %i", score),
                     GetScreenWidth()/2 - MeasureText(TextFormat("FINAL SCORE: %i", score), 20)/2,
                     GetScreenHeight()/2 + 10, 20, WHITE);
            DrawText("PRESS R TO RESTART", GetScreenWidth()/2 - MeasureText("PRESS R TO RESTART", 20)/2,
                     GetScreenHeight()/2 + 40, 20, WHITE);
        }
        
        DrawFPS(10, 40);
    EndDrawing();
}
#if 0
/*******************************************************************************************
*
*   phoenix_shooter
*
*   A modified raylib example demonstrating a phoenix-like shooter.
*   Use LEFT/RIGHT arrow keys (or press on left/right half of the screen) to move,
*   and SPACEBAR (or a quick tap) to fire.
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define MAX_BULLETS 32
#define MAX_BIRDS 10
#define MAX_LIVES 3

bool IsGestureTap(void) {
    //return false;
    return IsGestureDetected(GESTURE_TAP);
} 
// Structure to represent a bullet
typedef struct Bullet {
    Vector3 position;
    bool active;
    float radius;
} Bullet;

// Structure to represent a bird
typedef struct Bird {
    Vector3 position;
    Vector3 velocity;
    bool active;
    Color color;
    float size;
} Bird;

Bullet bullets[MAX_BULLETS] = {0};
Bird birds[MAX_BIRDS] = {0};

Camera camera = { 0 };
Vector3 playerPos = { 0.0f, 0.0f, 8.0f };  // Player position moved up
float playerSpeed = 0.4f;
float bulletSpeed = 0.8f;
float playAreaWidth = 30.0f;
float playAreaHeight = 20.0f;
int score = 0;
int lives = MAX_LIVES;
bool gameOver = false;
int invincibilityFrames = 0;  // Invincibility after being hit

static void UpdateDrawFrame(void);
static void InitBirds(void);
static void CheckCollisions(void);
static void CheckPlayerBirdCollisions(void);
static void ResetGame(void);

// Custom distance function to replace Vector3Distance
float CalculateDistance(Vector3 v1, Vector3 v2)
{
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;
    
    return sqrtf(dx*dx + dy*dy + dz*dz);
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
            // Spawn bullet ahead of the player
            bullets[i].position = (Vector3){ playerPos.x, 0.0f, playerPos.z - 1.0f };
            break;
        }
    }
}

// Initialize birds with random positions and colors
static void InitBirds(void)
{
    for (int i = 0; i < MAX_BIRDS; i++)
    {
        birds[i].position = (Vector3){ 
            GetRandomValue(-playAreaWidth/2 + 2, playAreaWidth/2 - 2), 
            0.0f, 
            GetRandomValue(-playAreaHeight + 2, 0) 
        };
        birds[i].velocity = (Vector3){ 
            GetRandomValue(-10, 10) / 100.0f, 
            0.0f, 
            GetRandomValue(-5, 5) / 100.0f 
        };
        birds[i].active = true;
        birds[i].size = GetRandomValue(10, 15) / 10.0f;
        
        // Random bird colors
        switch (GetRandomValue(0, 4))
        {
            case 0: birds[i].color = RED; break;
            case 1: birds[i].color = GREEN; break;
            case 2: birds[i].color = BLUE; break;
            case 3: birds[i].color = PURPLE; break;
            case 4: birds[i].color = ORANGE; break;
        }
    }
}

// Check for collisions between bullets and birds
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
                    // Use our custom distance function instead of Vector3Distance
                    if (CalculateDistance(bullets[i].position, birds[j].position) < (bullets[i].radius + birds[j].size * 0.7f))
                    {
                        bullets[i].active = false;
                        birds[j].active = false;
                        score++;
                        
                        // Respawn bird after a short delay (from the top)
                        birds[j].position = (Vector3){ 
                            GetRandomValue(-playAreaWidth/2 + 2, playAreaWidth/2 - 2), 
                            0.0f, 
                            GetRandomValue(-playAreaHeight + 2, -playAreaHeight/2) 
                        };
                        birds[j].velocity = (Vector3){ 
                            GetRandomValue(-15, 15) / 100.0f, 
                            0.0f, 
                            GetRandomValue(5, 15) / 100.0f
                        };
                        birds[j].active = true;
                        birds[j].size = GetRandomValue(10, 15) / 10.0f;
                    }
                }
            }
        }
    }
}

// Check for collisions between player and birds
static void CheckPlayerBirdCollisions(void)
{
    // Skip collision check during invincibility frames
    if (invincibilityFrames > 0) return;
    
    for (int i = 0; i < MAX_BIRDS; i++)
    {
        if (birds[i].active)
        {
            // Check if bird is near the player
            float playerSize = 1.0f;  // Approximate player collision size
            if (CalculateDistance(playerPos, birds[i].position) < (playerSize + birds[i].size))
            {
                lives--;
                invincibilityFrames = 120;  // 2 seconds at 60 FPS
                
                // Check if game over
                if (lives <= 0)
                {
                    gameOver = true;
                }
                
                // Reset bird position
                birds[i].position = (Vector3){ 
                    GetRandomValue(-playAreaWidth/2 + 2, playAreaWidth/2 - 2), 
                    0.0f, 
                    GetRandomValue(-playAreaHeight + 2, -playAreaHeight/2) 
                };
                
                // Only handle one collision at a time to prevent multiple lives lost in one frame
                break;
            }
        }
    }
}

// Reset the game state
static void ResetGame(void)
{
    score = 0;
    lives = MAX_LIVES;
    gameOver = false;
    
    // Reset player position
    playerPos = (Vector3){ 0.0f, 0.0f, 8.0f };
    
    // Deactivate all bullets
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].active = false;
    }
    
    // Reset birds
    InitBirds();
}

int main(void)
{
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "raylib - phoenix bird shooter");

    // Adjusted camera settings to make sure the player is visible
    camera.position = (Vector3){ 0.0f, 25.0f, 0.0f };
    camera.target   = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up       = (Vector3){ 0.0f, 0.0f, -1.0f };
    camera.fovy     = 30.0f;
    camera.projection = CAMERA_ORTHOGRAPHIC;

    // Initialize birds
    InitBirds();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
#endif

    // Main game loop
    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    // De-Initialization
    CloseWindow();

    return 0;
}

static void UpdateDrawFrame(void)
{
    // --- Input Handling ---
    if (!gameOver)
    {
        // Keyboard movement: left/right arrow keys
        if (IsKeyDown(KEY_LEFT))  playerPos.x -= playerSpeed;
        if (IsKeyDown(KEY_RIGHT)) playerPos.x += playerSpeed;

        // Constrain player to play area
        if (playerPos.x < -playAreaWidth/2) playerPos.x = -playAreaWidth/2;
        if (playerPos.x > playAreaWidth/2) playerPos.x = playAreaWidth/2;

        // Touch/Mouse input:
        // If the screen is pressed, check which half is being pressed for movement.
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            Vector2 pos = GetMousePosition();
            if (pos.x < GetScreenWidth()/2) playerPos.x -= playerSpeed;  // Left half of screen
            else                           playerPos.x += playerSpeed;  // Right half of screen
        }

        // Shooting: spacebar press OR a tap gesture (which represents a short press)
        static int frameCounter = 0;
        frameCounter++;
        
        if (IsKeyPressed(KEY_SPACE) || IsGestureTap())
        {
            SpawnBullet();
        }
        
        // Auto-fire every 10 frames
        //if (frameCounter % 10 == 0)
        //{
        //    SpawnBullet();
        //}

        // Decrease invincibility frames if active
        if (invincibilityFrames > 0)
        {
            invincibilityFrames--;
        }
    }
    else
    {
        // Game Over - wait for restart key
        if (IsKeyPressed(KEY_R))
        {
            ResetGame();
        }
    }

    // --- Bird Updates ---
    if (!gameOver)
    {
        for (int i = 0; i < MAX_BIRDS; i++)
        {
            if (birds[i].active)
            {
                // Move birds according to their velocity
                birds[i].position.x += birds[i].velocity.x;
                birds[i].position.z += birds[i].velocity.z;
                
                // Bounce birds off the edges of the play area
                if (birds[i].position.x < -playAreaWidth/2 || birds[i].position.x > playAreaWidth/2)
                {
                    birds[i].velocity.x = -birds[i].velocity.x;
                }
                
                // If birds reach the bottom, send them back to the top
                if (birds[i].position.z > playerPos.z - 2)
                {
                    birds[i].position.z = -playAreaHeight;
                    birds[i].position.x = GetRandomValue(-playAreaWidth/2 + 2, playAreaWidth/2 - 2);
                }
                
                // If birds go too high, bounce them back
                if (birds[i].position.z < -playAreaHeight)
                {
                    birds[i].velocity.z = -birds[i].velocity.z;
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
                // Move bullet forward (toward decreasing Z)
                bullets[i].position.z -= bulletSpeed;
                // Deactivate bullet if it goes too far (off-screen)
                if (bullets[i].position.z < -playAreaHeight - 2.0f)
                {
                    bullets[i].active = false;
                }
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
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
            // Draw the player spaceship as a more visible shape
            Color playerColor = RED;
            
            // Flash the player when invincible
            if (invincibilityFrames > 0 && (invincibilityFrames/10) % 2 == 0)
            {
                playerColor = LIGHTGRAY;
            }
            
            if (!gameOver)
            {
                DrawCube(playerPos, 2.0f, 0.3f, 1.5f, playerColor);
                DrawCubeWires(playerPos, 2.0f, 0.3f, 1.5f, MAROON);
                // Add player "wings" for better visibility
                DrawCube((Vector3){playerPos.x - 1.2f, playerPos.y, playerPos.z + 0.2f}, 
                         0.8f, 0.2f, 0.6f, DARKBLUE);
                DrawCube((Vector3){playerPos.x + 1.2f, playerPos.y, playerPos.z + 0.2f}, 
                         0.8f, 0.2f, 0.6f, DARKBLUE);
            }

            // Draw active bullets as larger blue spheres
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active)
                {
                    DrawSphere(bullets[i].position, bullets[i].radius, BLUE);
                }
            }
            
            // Draw birds
            for (int i = 0; i < MAX_BIRDS; i++)
            {
                if (birds[i].active)
                {
                    // Draw bird as a colored cube with wings (additional cubes)
                    Vector3 birdPos = birds[i].position;
                    float size = birds[i].size;
                    
                    DrawCube(birdPos, size, size * 0.3f, size, birds[i].color);
                    
                    // Wings - flap based on time
                    float wingOffset = sinf(GetTime() * 10) * 0.5f + 0.5f;
                    
                    DrawCube((Vector3){birdPos.x - size, birdPos.y, birdPos.z}, 
                             size * 0.8f, size * 0.2f, size * 0.8f * wingOffset, birds[i].color);
                    DrawCube((Vector3){birdPos.x + size, birdPos.y, birdPos.z}, 
                             size * 0.8f, size * 0.2f, size * 0.8f * wingOffset, birds[i].color);
                }
            }

            // Draw a grid representing the play area boundaries
            //DrawGrid(30, 1.0f);
            
            // Draw play area boundaries
            DrawLine3D((Vector3){-playAreaWidth/2, 0, -playAreaHeight}, (Vector3){playAreaWidth/2, 0, -playAreaHeight}, RED);
            DrawLine3D((Vector3){-playAreaWidth/2, 0, playerPos.z + 1}, (Vector3){playAreaWidth/2, 0, playerPos.z + 1}, RED);
            DrawLine3D((Vector3){-playAreaWidth/2, 0, -playAreaHeight}, (Vector3){-playAreaWidth/2, 0, playerPos.z + 1}, RED);
            DrawLine3D((Vector3){playAreaWidth/2, 0, -playAreaHeight}, (Vector3){playAreaWidth/2, 0, playerPos.z + 1}, RED);
            
        EndMode3D();

        // Draw instructions and score
        DrawText("Use arrow keys or touch screen to move, spacebar/tap to shoot", 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("SCORE: %i", score), GetScreenWidth() - 150, 10, 20, BLACK);
        
        // Draw lives
        DrawText(TextFormat("LIVES: %i", lives), GetScreenWidth() - 150, 40, 20, BLACK);
        
        // Draw game over screen
        if (gameOver)
        {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));
            DrawText("GAME OVER", GetScreenWidth()/2 - MeasureText("GAME OVER", 40)/2, GetScreenHeight()/2 - 40, 40, RED);
            DrawText(TextFormat("FINAL SCORE: %i", score), GetScreenWidth()/2 - MeasureText(TextFormat("FINAL SCORE: %i", score), 20)/2, GetScreenHeight()/2 + 10, 20, WHITE);
            DrawText("PRESS R TO RESTART", GetScreenWidth()/2 - MeasureText("PRESS R TO RESTART", 20)/2, GetScreenHeight()/2 + 40, 20, WHITE);
        }
        
        DrawFPS(10, 40);
    EndDrawing();
}

#endif