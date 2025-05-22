#include <SDL2/SDL.h>
#include <vector>
#include <cstdlib>
#include <ctime>

const int PLAYER_SIZE = 100;
const int ENEMY_WIDTH = 80;
const int ENEMY_HEIGHT = 60;
const int BULLET_WIDTH = 10;
const int BULLET_HEIGHT = 40;

const int PLAYER_SPEED = 10;
const int BULLET_SPEED = 50;
const int ENEMY_SPEED = 2;

const int DASH_SPEED = 60;
const Uint32 DASH_DURATION = 150;  // ms
const Uint32 DASH_COOLDOWN = 500; // ms

struct GameObject
{
    SDL_Rect rect;
    bool active = true;
};

bool checkCollision(const SDL_Rect &a, const SDL_Rect &b)
{
    return SDL_HasIntersection(&a, &b);
}

void resetGame(SDL_Rect &player, std::vector<GameObject> &bullets, std::vector<GameObject> &enemies, int &lives, int screenWidth, int screenHeight)
{
    player = {screenWidth / 2 - PLAYER_SIZE / 2, screenHeight - PLAYER_SIZE - 10, PLAYER_SIZE, PLAYER_SIZE};
    bullets.clear();
    enemies.clear();
    lives = 3;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    int SCREEN_WIDTH = dm.w;
    int SCREEN_HEIGHT = dm.h;

    SDL_Window *window = SDL_CreateWindow("SDL2 Game with Dash, Fire Rate, Health, Game Over",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Rect player;
    std::vector<GameObject> bullets;
    std::vector<GameObject> enemies;
    int lives = 3;

    resetGame(player, bullets, enemies, lives, SCREEN_WIDTH, SCREEN_HEIGHT);

    std::srand(static_cast<unsigned>(std::time(0)));
    int enemySpawnTimer = 0;
    bool quit = false;
    bool gameOver = false;
    SDL_Event e;

    Uint32 lastBulletTime = 0;
    Uint32 fireCooldown = 150; // ms

    bool dashing = false;
    Uint32 dashStartTime = 0;
    Uint32 lastDashTime = 0;

    while (!quit)
    {
        Uint32 currentTime = SDL_GetTicks();
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                quit = true;

            if (gameOver && e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r)
            {
                gameOver = false;
                resetGame(player, bullets, enemies, lives, SCREEN_WIDTH, SCREEN_HEIGHT);
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        if (!gameOver)
        {
            // Dash logic
            bool canDash = (currentTime > lastDashTime + DASH_COOLDOWN);
            if (!dashing && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT)) && canDash)
            {
                dashing = true;
                dashStartTime = currentTime;
                lastDashTime = currentTime;
            }

            int moveSpeed = dashing ? DASH_SPEED : PLAYER_SPEED;
            if (keys[SDL_SCANCODE_A])
                player.x -= moveSpeed;
            if (keys[SDL_SCANCODE_D])
                player.x += moveSpeed;
            if (keys[SDL_SCANCODE_W])
                player.y -= moveSpeed;
            if (keys[SDL_SCANCODE_S])
                player.y += moveSpeed;

            // End dash
            if (dashing && currentTime > dashStartTime + DASH_DURATION)
                dashing = false;

            // Clamp
            player.x = std::max(0, std::min(player.x, SCREEN_WIDTH - PLAYER_SIZE));
            player.y = std::max(0, std::min(player.y, SCREEN_HEIGHT - PLAYER_SIZE));

            // Fire bullet
            if ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) && currentTime > lastBulletTime + fireCooldown)
            {
                GameObject bullet;
                bullet.rect = {player.x + PLAYER_SIZE / 2 - BULLET_WIDTH / 2, player.y, BULLET_WIDTH, BULLET_HEIGHT};
                bullets.push_back(bullet);
                lastBulletTime = currentTime;
            }

            // Spawn enemy
            if (++enemySpawnTimer > 60)
            {
                GameObject enemy;
                enemy.rect = {std::rand() % (SCREEN_WIDTH - ENEMY_WIDTH), 0, ENEMY_WIDTH, ENEMY_HEIGHT};
                enemies.push_back(enemy);
                enemySpawnTimer = 0;
            }

            // Update bullets
            for (auto &bullet : bullets)
            {
                if (bullet.active)
                {
                    bullet.rect.y -= BULLET_SPEED;
                    if (bullet.rect.y + BULLET_HEIGHT < 0)
                        bullet.active = false;
                }
            }

            // Update enemies
            for (auto &enemy : enemies)
            {
                if (enemy.active)
                {
                    enemy.rect.y += ENEMY_SPEED;
                    if (enemy.rect.y > SCREEN_HEIGHT)
                    {
                        enemy.active = false;
                        lives--;
                        if (lives <= 0)
                            gameOver = true;
                    }
                }
            }

            // Collision
            for (auto &bullet : bullets)
            {
                if (!bullet.active)
                    continue;
                for (auto &enemy : enemies)
                {
                    if (!enemy.active)
                        continue;
                    if (checkCollision(bullet.rect, enemy.rect))
                    {
                        bullet.active = false;
                        enemy.active = false;
                    }
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Player
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &player);

        // Bullets
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (const auto &bullet : bullets)
            if (bullet.active)
                SDL_RenderFillRect(renderer, &bullet.rect);

        // Enemies
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (const auto &enemy : enemies)
            if (enemy.active)
                SDL_RenderFillRect(renderer, &enemy.rect);

        // Health
        for (int i = 0; i < lives; ++i)
        {
            SDL_Rect heart = {20 + i * 30, 20, 20, 20};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &heart);
        }

        // Game over screen
        if (gameOver)
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_Rect box = {SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 50, 400, 100};
            SDL_RenderFillRect(renderer, &box);
            // You can render text here using SDL_ttf for "Game Over! Press R to restart"
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
