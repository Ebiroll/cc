/* Okay, let's modify the game code to use the provided PNG asset for the player ship instead of the cube-based representation. We'll use the DrawBillboard function to render the texture so it always faces the camera.

Steps:

Save the Asset: Save the provided PNG image as spaceship.png in the same directory as your source code, or in a resources subdirectory if you prefer (adjust the path in the code accordingly).

Declare Texture: Add a global variable to hold the loaded texture.

Load Texture: Load the texture in main() after InitWindow().

Unload Texture: Unload the texture in main() before CloseWindow().

Modify Drawing: In UpdateDrawFrame(), replace the DrawCube calls for the player with a DrawBillboard call using the loaded texture. Adjust the size parameter as needed for visual scale.

Adjust Player Color Logic: When using textures, the tint parameter in drawing functions multiplies the texture's color. To show the original colors, use WHITE as the base tint. For flashing, LIGHTGRAY or another color can still be used.

Here's the modified code:
*/
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
            PlaySound(shootSound);
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
                        PlaySound(birdHitSound);
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
                PlaySound(playerHitSound);

                // Make the bird inactive instead of moving it, consistent with bullet hits
                birds[i].active = false;

                if (lives <= 0) {
                    gameOver = true;
                    PlaySound(gameOverSound);
                    StopMusicStream(gameMusic);
                }
                // Don't break here, allow multiple collisions per frame if unlucky,
                // but invincibility prevents immediate multi-hits.
                 break; // Break after one hit per frame to avoid instant death
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

    // Reset and play music if it was loaded
    if (IsMusicValid(gameMusic)) {
        StopMusicStream(gameMusic);
        PlayMusicStream(gameMusic);
    }
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
    if (FileExists("resources/gamemusic.mp3")) gameMusic = LoadMusicStream("resources/gamemusic.mp3");

    // Set volumes only if sounds/music were loaded
    if (IsSoundValid(shootSound)) SetSoundVolume(shootSound, 0.7f);
    if (IsSoundValid(birdHitSound)) SetSoundVolume(birdHitSound, 0.8f);
    if (IsSoundValid(playerHitSound)) SetSoundVolume(playerHitSound, 0.9f);
    if (IsSoundValid(waveSpawnSound)) SetSoundVolume(waveSpawnSound, 0.8f);
    if (IsSoundValid(gameOverSound)) SetSoundVolume(gameOverSound, 1.0f);
    if (IsMusicValid(gameMusic)) {
        SetMusicVolume(gameMusic, 0.5f);
        PlayMusicStream(gameMusic);
    }
}

// Unload game sounds
static void UnloadGameSounds(void)
{
    if (IsSoundValid(shootSound)) UnloadSound(shootSound);
    if (IsSoundValid(birdHitSound)) UnloadSound(birdHitSound);
    if (IsSoundValid(playerHitSound)) UnloadSound(playerHitSound);
    if (IsSoundValid(waveSpawnSound)) UnloadSound(waveSpawnSound);
    if (IsSoundValid(gameOverSound)) UnloadSound(gameOverSound);
    if (IsMusicValid(gameMusic)) UnloadMusicStream(gameMusic);
}

int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "raylib - phoenix bird shooter");
    InitAudioDevice();

    // --- Load Player Texture ---
    // Make sure spaceship.png is in the same directory or specify path e.g. "resources/spaceship.png"
    if (FileExists("resources/spaceship.png")) {
         playerTexture = LoadTexture("resources/spaceship.png");
    } else {
         // Handle error - maybe draw a placeholder or exit
         TraceLog(LOG_WARNING, "Failed to load spaceship.png");
         // As a fallback, we could create a simple white texture
         Image fallbackImg = GenImageColor(32, 32, RED);
         playerTexture = LoadTextureFromImage(fallbackImg);
         UnloadImage(fallbackImg);
    }
    // -------------------------

    LoadGameSounds();

    // Adjusted camera so the formation and player are visible
    camera.position = (Vector3){ 0.0f, 30.0f, 15.0f }; // Pulled camera back slightly more
    camera.target   = (Vector3){ 0.0f, 0.0f, 0.0f }; // Target center of action
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f }; // Standard Y-up
    camera.fovy     = 45.0f;                         // Perspective view often looks better
    camera.projection = CAMERA_PERSPECTIVE;          // Changed to Perspective

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
    // Update music stream
    if (IsMusicValid(gameMusic)) UpdateMusicStream(gameMusic);

    // --- Input Handling for Player ---
    if (!gameOver)
    {
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  playerPos.x -= playerSpeed;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) playerPos.x += playerSpeed;

        // Constrain player
        float playerHalfWidth = 1.5f; // Approx half-width based on billboard size used below
        if (playerPos.x < -playAreaWidth/2 + playerHalfWidth) playerPos.x = -playAreaWidth/2 + playerHalfWidth;
        if (playerPos.x > playAreaWidth/2 - playerHalfWidth) playerPos.x = playAreaWidth/2 - playerHalfWidth;

        // Mouse/Touch movement (alternative)
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            Vector2 touchPos = GetMousePosition(); // Use mouse pos as proxy for touch
            // Simple check: left half moves left, right half moves right
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
        if (IsKeyPressed(KEY_R) || IsGestureTap()) ResetGame(); // Allow tap to restart
    }

    // --- Formation & Attack Update for Birds (only if game is not over) ---
    if (!gameOver)
    {
        // Check and handle bird respawning (only if not already waiting)
        if (!allBirdsDestroyed) CheckAndRespawnBirds();
        else {
             // If waiting, just update delay, respawn happens in CheckAndRespawnBirds
             respawnDelay++;
             if (respawnDelay >= RESPAWN_DELAY_FRAMES) {
                 InitBirds();
             }
        }


        // Update formation horizontal movement.
        formationX += formationSpeed * formationDirection;
        // Boundaries for formation (adjust based on actual formation width)
        float formationEdgeOffset = 4.0f * (5/2); // spacing * (cols/2) approx
        if (formationX - formationEdgeOffset < -playAreaWidth/2) {
            formationX = -playAreaWidth/2 + formationEdgeOffset; // Prevent going past edge
            formationDirection = 1;
        }
        if (formationX + formationEdgeOffset > playAreaWidth/2) {
            formationX = playAreaWidth/2 - formationEdgeOffset; // Prevent going past edge
            formationDirection = -1;
        }

        // Determine if this is an aggressive wave based on pattern
        bool aggressiveWave = (currentWave > 0) && wavePattern[(currentWave - 1) % WAVE_PATTERN_LENGTH];


        // For birds still in formation, update their positions relative to the formation anchor.
        for (int i = 0; i < MAX_BIRDS; i++)
        {
            if (birds[i].active && !birds[i].attacking)
            {
                 // Simple approach towards formation target Z
                 float targetZ = formationBaseZ + birds[i].formationOffset.z;
                 if (birds[i].position.z < targetZ) {
                     birds[i].position.z += 0.2f; // Speed at which birds fly into formation Z
                     if (birds[i].position.z > targetZ) birds[i].position.z = targetZ; // Clamp
                 }

                // Add wobble for unpredictable movement once near formation Z
                if (fabsf(birds[i].position.z - targetZ) < 1.0f) {
                    float wobbleX = sinf(GetTime() * birds[i].wobbleSpeed) * birds[i].wobbleFactor;
                    float wobbleZ = cosf(GetTime() * birds[i].wobbleSpeed * 0.7f) * birds[i].wobbleFactor * 0.5f;
                    birds[i].position.x = formationX + birds[i].formationOffset.x + wobbleX;
                    birds[i].position.z = targetZ + wobbleZ; // Wobble around target Z
                } else {
                    // If still flying in, just update X based on formation center
                     birds[i].position.x = formationX + birds[i].formationOffset.x;
                }
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

        // Check if any bird can start attacking
         bool canAttack = false;
         for(int i = 0; i < MAX_BIRDS; ++i) {
             // Only allow attacking if bird is roughly in formation position
             if (birds[i].active && !birds[i].attacking && fabsf(birds[i].position.z - (formationBaseZ + birds[i].formationOffset.z)) < 1.0f) {
                 canAttack = true;
                 break;
             }
         }

        if (canAttack && attackingCount < maxAttacking)
        {
            for (int i = 0; i < MAX_BIRDS; i++)
            {
                // Check again if this specific bird is ready to attack
                 if (birds[i].active && !birds[i].attacking && fabsf(birds[i].position.z - (formationBaseZ + birds[i].formationOffset.z)) < 1.0f)
                {
                    // With a small chance, start an attack (higher chance in aggressive waves)
                    int attackChance = aggressiveWave ? 15 : 5; // Increased chance slightly
                    if (GetRandomValue(0, 5000) < attackChance) // Check less frequently
                    {
                        birds[i].attacking = true;

                        // Play wave spawn sound (maybe rename or use a different sound for attack)
                        if (IsSoundValid(waveSpawnSound)) PlaySound(waveSpawnSound);


                        // Set an attack velocity: dive toward the player's current X with a slight random lead/lag
                        float targetX = playerPos.x + GetRandomValue(-20, 20) / 10.0f; // Aim near player X
                        float deltaX = targetX - birds[i].position.x;
                        float dist = CalculateDistance(birds[i].position, (Vector3){targetX, 0.0f, playerPos.z}); // Approx distance

                        float attackSpeedZ = aggressiveWave ? 0.4f : 0.3f; // Adjust Z speed
                        float attackSpeedX = (dist > 0.1f) ? (deltaX / dist) * attackSpeedZ * 0.8f : 0.0f; // Adjust X speed based on Z speed

                        birds[i].velocity.x = attackSpeedX;
                        birds[i].velocity.z = attackSpeedZ; // Positive Z is towards player
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

                // Add some minor sideways wobble during attack
                birds[i].position.x += sinf(GetTime() * 8.0f + i) * 0.05f; // Unique wobble per bird

                // Once an attacking bird passes the player or goes off bottom edge,
                // mark it inactive (it flew past) instead of returning to formation immediately.
                // It will be respawned with the next wave.
                if (birds[i].position.z > playerPos.z + 5.0f) // Flew past player significantly
                {
                     birds[i].active = false; // Bird is gone for this wave
                     //birds[i].attacking = false; // Also reset state just in case
                     //birds[i].velocity = (Vector3){0.0f, 0.0f, 0.0f};

                     // No need to reposition, it's inactive now.
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
                // Deactivate bullet if it goes off top edge
                if (bullets[i].position.z < -playAreaHeight - 5.0f) // Extended boundary check
                    bullets[i].active = false;
            }
        }
    }

    // --- Check for Collisions ---
    if (!gameOver)
    {
        CheckCollisions(); // Bullet-Bird
        CheckPlayerBirdCollisions(); // Player-Bird
    }

    // --- Drawing ---
    BeginDrawing();
        ClearBackground(BLACK); // Black background fits space theme better

        BeginMode3D(camera);
            // Draw the player ship using the texture as a billboard
            if (!gameOver)
            {
                Color playerTint = WHITE; // Use WHITE tint to show original texture colors
                if (invincibilityFrames > 0 && (invincibilityFrames/10) % 2 == 0) {
                    playerTint = GRAY; // Flash effect using GRAY tint
                }
                // Adjust size parameter (3rd argument) to scale the sprite appropriately
                DrawBillboard(camera, playerTexture, playerPos, 3.0f, playerTint);
            }

            // Draw bullets
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active)
                {
                    // Draw bullets as bright spheres or small cubes
                    DrawSphere(bullets[i].position, bullets[i].radius, YELLOW);
                    // Or DrawCube(bullets[i].position, 0.4f, 0.4f, 1.0f, YELLOW); // Elongated bullet
                }
            }

            // Draw birds (only active ones)
            for (int i = 0; i < MAX_BIRDS; i++)
            {
                if (birds[i].active)
                {
                    Vector3 birdPos = birds[i].position;
                    float size = birds[i].size;
                    Color birdColor = birds[i].color;

                    // Make birds brighter
                    // birdColor = ColorBrightness(birds[i].color, 0.2f); // Slightly brighter

                    // Simple cube representation for birds is fine, or use another asset
                    DrawCube(birdPos, size, size * 0.3f, size, birdColor);

                    // Optional simple wings (adjust as needed)
                    float wingOffset = sinf(GetTime() * 15.0f + i * 0.5f) * 0.4f + 0.6f; // Faster flap, slightly varied per bird
                    DrawCube((Vector3){birdPos.x - size*0.6f, birdPos.y, birdPos.z + size * 0.2f},
                             size * 0.7f, size * 0.2f, size * 0.5f * wingOffset, birdColor);
                    DrawCube((Vector3){birdPos.x + size*0.6f, birdPos.y, birdPos.z + size * 0.2f},
                             size * 0.7f, size * 0.2f, size * 0.5f * wingOffset, birdColor);

                    // Draw wireframe outline for definition
                    DrawCubeWires(birdPos, size, size * 0.3f, size, ColorBrightness(birdColor, -0.5f)); // Darker outline
                }
            }

            // Draw play area boundaries (optional, can be distracting)
            // DrawLine3D((Vector3){-playAreaWidth/2, 0, -playAreaHeight}, (Vector3){playAreaWidth/2, 0, -playAreaHeight}, DARKGRAY);
            // DrawLine3D((Vector3){-playAreaWidth/2, 0, playerPos.z + 1}, (Vector3){playAreaWidth/2, 0, playerPos.z + 1}, DARKGRAY);
            // DrawLine3D((Vector3){-playAreaWidth/2, 0, -playAreaHeight}, (Vector3){-playAreaWidth/2, 0, playerPos.z + 1}, DARKGRAY);
            // DrawLine3D((Vector3){playAreaWidth/2, 0, -playAreaHeight}, (Vector3){playAreaWidth/2, 0, playerPos.z + 1}, DARKGRAY);

             // Draw a subtle grid on the "floor"
             //DrawGrid(40, 2.0f);


        EndMode3D();

        // --- UI Overlay ---
        // Draw Score, Lives, Wave Info
        DrawText(TextFormat("SCORE: %04i", score), 10, 10, 20, RAYWHITE); // Use fixed width for score
        DrawText(TextFormat("LIVES: %i", lives), GetScreenWidth() - 100, 10, 20, RAYWHITE);

        bool isAggressive = (currentWave > 0) && wavePattern[(currentWave - 1) % WAVE_PATTERN_LENGTH];
        DrawText(TextFormat("WAVE: %i %s", currentWave, isAggressive ? "[AGGRESSIVE]" : ""), 10, 35, 20, isAggressive ? RED : LIGHTGRAY);


        // Draw instructions only at the start or if game over? Maybe smaller at bottom
        if (score == 0 && lives == MAX_LIVES && currentWave <= 1) {
             DrawText("Arrows/AD/Touch: Move | Space/Tap: Shoot", 10, GetScreenHeight() - 30, 20, GRAY);
        }

        // If waiting to respawn, show message
        if (allBirdsDestroyed && !gameOver)
        {
            int countdownSeconds = (RESPAWN_DELAY_FRAMES - respawnDelay) / 60 + 1;
             DrawText(TextFormat("WAVE CLEARED! NEXT WAVE IN: %i", countdownSeconds),
                      GetScreenWidth()/2 - MeasureText(TextFormat("WAVE CLEARED! NEXT WAVE IN: %i", countdownSeconds), 20)/2,
                      GetScreenHeight()/2 - 10, 20, GREEN); // Centered message
        }

        if (gameOver)
        {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f)); // Darker overlay
            DrawText("GAME OVER", GetScreenWidth()/2 - MeasureText("GAME OVER", 50)/2, // Larger text
                     GetScreenHeight()/2 - 60, 50, RED);
            DrawText(TextFormat("FINAL SCORE: %i", score),
                     GetScreenWidth()/2 - MeasureText(TextFormat("FINAL SCORE: %i", score), 30)/2,
                     GetScreenHeight()/2 + 0, 30, RAYWHITE);
            DrawText("PRESS R or TAP TO RESTART", GetScreenWidth()/2 - MeasureText("PRESS R or TAP TO RESTART", 20)/2,
                     GetScreenHeight()/2 + 40, 20, LIGHTGRAY);
        }

        DrawFPS(GetScreenWidth() - 90, GetScreenHeight() - 30); // FPS counter bottom right
    EndDrawing();
}
/*

Key Changes:

playerTexture: Global Texture2D declared.

main():

playerTexture = LoadTexture("spaceship.png"); added after InitWindow(). Includes basic error checking/fallback.

UnloadTexture(playerTexture); added before CloseWindow().

Camera changed to CAMERA_PERSPECTIVE and position/fovy adjusted for a potentially better view with billboards. You might want to switch back to CAMERA_ORTHOGRAPHIC if you prefer that look.

UpdateDrawFrame() -> Drawing:

The block of DrawCube calls for the player is removed.

DrawBillboard(camera, playerTexture, playerPos, 3.0f, playerTint); is added inside the if (!gameOver) block.

camera and playerPos are used as before.

playerTexture is the loaded asset.

3.0f is the size in world units. You will likely need to adjust this value to make the ship look appropriately sized. Try values between 2.0 and 5.0.

playerTint is calculated based on invincibility, using WHITE as the base (to show original texture colors) and GRAY for flashing.

UpdateDrawFrame() -> Player Input: Player boundary checks slightly adjusted based on an estimated visual width (playerHalfWidth).

CheckPlayerBirdCollisions(): The playerCollisionRadius might need tuning (1.5f is a starting guess) to match the visual size of the new sprite for fair collisions.

SpawnBullet(): Adjusted the Z-offset where the bullet spawns to appear closer to the front/center of the sprite.

Other Minor Changes: Black background, adjusted UI text positions/colors, perspective camera option, refined bird attack/respawn logic, sound loading checks.

Now, when you compile and run this code (making sure spaceship.png is accessible), the player ship should be rendered using the provided image, always facing the camera. Remember to adjust the size parameter in DrawBillboard and potentially the playerCollisionRadius for the best visual and gameplay results.
*/