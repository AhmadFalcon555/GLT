#include <irrKlang/irrKlang.h>

#include <iostream>
#include <sstream>

#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "game_object.h"
#include "ball_object.h"
#include "text_renderer.h"

using namespace irrklang;
// Game-related State data
SpriteRenderer  *Renderer;
GameObject* emptyLater;
BallObject* Players;
ISoundEngine* SoundEngine = createIrrKlangDevice();
TextRenderer* Text;

Game::Game(unsigned int width, unsigned int height) 
    : State(GAME_MENU), Keys(), Width(width), Height(height)
{ 

}

Game::~Game()
{
    delete Renderer;
    delete emptyLater;
    delete Players;
    SoundEngine->drop();
}

void Game::Init()
{
    // load shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), 
        static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    // set render-specific controls
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    // load bg textures
    ResourceManager::LoadTexture("textures/MainMenu.png", true, "background");
    ResourceManager::LoadTexture("textures/1.png", false, "background2");
    ResourceManager::LoadTexture("textures/2.png", true, "background3");
    ResourceManager::LoadTexture("textures/3.png", true, "background4");
    ResourceManager::LoadTexture("textures/4.png", true, "background5");
    ResourceManager::LoadTexture("textures/back1.png", false, "back1");
    ResourceManager::LoadTexture("textures/back2.png", false, "back2");
    // load texture
    ResourceManager::LoadTexture("textures/jump.png", true, "player");
    ResourceManager::LoadTexture("textures/tropi.png", true, "block");
    ResourceManager::LoadTexture("textures/tile.png", true, "block_solid");
    // unused
    ResourceManager::LoadTexture("textures/jump.png", true, "paddle");
    // load levels
    GameLevel zero; zero.Load("levels/zero.lvl", this->Width, this->Height);
    GameLevel one; one.Load("levels/one.lvl", this->Width, this->Height);
    GameLevel two; two.Load("levels/two.lvl", this->Width, this->Height );
    GameLevel three; three.Load("levels/three.lvl", this->Width, this->Height );
    GameLevel four; four.Load("levels/four.lvl", this->Width, this->Height);
    this->Levels.push_back(zero);
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Level = 0;
    // configure game objects
    glm::vec2 playerPos = glm::vec2(2000,2000); // initial tempat pertama kali di panggil
    emptyLater = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle")); // posisi, ukuran, sprite yang dipake
    glm::vec2 ballPos = glm::vec2((this->Width / 2.0f) - BALL_RADIUS / 2.0f,100);
    Players = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("player"));
    // audio
    SoundEngine->play2D("audio/menuTheme.ogg", true);
    SoundEngine->setSoundVolume(0.8f);
    // font 
    Text = new TextRenderer(this->Width, this->Height);
    Text->Load("fonts/quicksand.ttf", 24);

}

void Game::Update(float dt)
{
    if (!Level == 0)
    {
        gravity = 0.5f;
    }
    // update objects
    Players->Move(dt, this->Width);
    // check for collisions
    this->DoCollisions();
    // check loss condition
    if (Players->Position.y >= this->Height) // did ball reach bottom edge?
    {
        this->PrevLevel();
        this->ResetPlayer();
    }
    if (Players->Position.y <= -100 && Level == 4)
    {
        this->State = GAME_WIN;
    }
    if (Players->Position.y <= -100 && Level < 4)
    {
        this->NextLevel();
        this->ResetPlayer();
    }

}

void Game::ProcessInput(float dt)
{

    if (this->State == GAME_MENU)
    {
        if (this->Keys[GLFW_KEY_SPACE] && Level == 0)
        {
            this->State = GAME_ACTIVE;
            NextLevel();
            SoundEngine->stopAllSounds();
            SoundEngine->play2D("audio/inGame.ogg", true);
            SoundEngine->setSoundVolume(0.4f);
        }
    }
    if (this->State == GAME_ACTIVE)
    {
        float velocity = PLAYER_VELOCITY * dt;
        // move playerboard
        if (this->Keys[GLFW_KEY_A])
        {
            flip = 0;
            if (Players->Position.x >= 0.0f)
            {
                Players->Position.x -= velocity;
            }
        }
        if (this->Keys[GLFW_KEY_D])
        {
            flip = 1;
            if (Players->Position.x <= this->Width - Players->Size.x)
            {
                Players->Position.x += velocity;
            }
        }
        if (this->Keys[GLFW_KEY_SPACE] && onground)
        {
            Players->Velocity.y = -8.0f;
            onground = false;
        }

        if (this->Keys[GLFW_KEY_M])
        {
            //mute
            SoundEngine->setAllSoundsPaused(true);
        }
        if (this->Keys[GLFW_KEY_N])
        {
            SoundEngine->setAllSoundsPaused(false);
        }
    }
    if (this->State == GAME_WIN)
    {
        if (this->Keys[GLFW_KEY_SPACE])
        {
            this->State = GAME_MENU;
            Level = 0;
        }
    }
    
    Players->Velocity.y += gravity / 4;
    Players->Position.y += Players->Velocity.y;

}

void Game::Render()
{
    std::stringstream ss; ss << this->Point;
    if (this->State == GAME_MENU)
    {
        Level = 0;
        Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
    }
    if (this->State == GAME_WIN)
    {
        Text->RenderText(
            "You WON!!! Thanks for playing", Width / 2 - 150, Height /2 - 20.0, 1.0, glm::vec3(0.0, 1.0, 0.0)
        );
        Text->RenderText(
            "Press space to RETRY, esc to EXIT", Width /  2 - 150, Height / 2, 1.0, glm::vec3(1.0, 1.0, 0.0)
        );
        Text->RenderText(
            "Your Point is " + ss.str(), Width / 2 - 150, Height / 2 + 20, 1.0, glm::vec3(1.0, 0.0, 0.0)
        );
    }
    if (this->State == GAME_ACTIVE)
    {
        // draw background
        if (Level == 1)
        {
            Renderer->DrawSprite(ResourceManager::GetTexture("background2"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        }
        else if (Level == 2)
        {
            Renderer->DrawSprite(ResourceManager::GetTexture("back2"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
            Renderer->DrawSprite(ResourceManager::GetTexture("background3"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        }
        else if (Level == 3)
        {
            Renderer->DrawSprite(ResourceManager::GetTexture("back1"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
            Renderer->DrawSprite(ResourceManager::GetTexture("background4"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        }
        else if (Level == 4)
        {
            Renderer->DrawSprite(ResourceManager::GetTexture("back1"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
            Renderer->DrawSprite(ResourceManager::GetTexture("background5"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        }
        // draw level
        this->Levels[this->Level].Draw(*Renderer);
        // draw player
        Players->Draw(*Renderer);
        if (Level == 1)
        {
            Text->RenderText("Level: 1", 100.0f, 5.0f, 1.5f);
        }
        else if (Level == 2)
        {
            Text->RenderText("Level: 2", 100.0f, 5.0f, 1.5f);
        }
        else if (Level == 3)
        {
            Text->RenderText("Level: 3", 100.0f, 5.0f, 1.5f);
        }
        else if (Level == 4)
        {
            Text->RenderText("Level: 4", 100.0f, 5.0f, 1.5f);
        }
        
        Text->RenderText("point : " + ss.str(), 5.0f, 300.0f, 1.5f);
    }
}
void Game::PrevLevel()
{
    this->Level -= 1;
    ResetLevel();
}

void Game::NextLevel()
{
    this->Level += 1;
    ResetLevel();
}
void Game::ResetLevel()
{
    if (this->Level == 0)
        this->Levels[0].Load("levels/zero.lvl", this->Width, this->Height );
    else if (this->Level == 1)
        this->Levels[1].Load("levels/one.lvl", this->Width, this->Height );
    else if (this->Level == 2)
        this->Levels[2].Load("levels/two.lvl", this->Width, this->Height );
    else if (this->Level == 3)
        this->Levels[3].Load("levels/three.lvl", this->Width, this->Height);
    else if (this->Level == 4)
        this->Levels[4].Load("levels/four.lvl", this->Width, this->Height);
}

void Game::ResetPlayer()
{
    // reset player/ball stats
    emptyLater->Size = PLAYER_SIZE;
    emptyLater->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    Players->Reset(emptyLater->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f)), INITIAL_BALL_VELOCITY);
}

// collision detection
bool IsCollided(float x1, float y1, float width1, float height1, float x2, float y2, float width2, float height2);
bool CheckCollision(GameObject& one, GameObject& two);
Collision CheckCollision(BallObject& one, GameObject& two);
Direction VectorDirection(glm::vec2 closest);

void Game::DoCollisions()
{
    for (GameObject& box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
        {
            Collision collision = CheckCollision(*Players, box);
            if (std::get<0>(collision)) // if collision is true
            {
                // destroy block if not solid
                if (!box.IsSolid)
                {
                    //nambah poin
                    Point++;
                    box.Destroyed = true;

                }
                // collision resolution
                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);
                if (dir == LEFT || dir == RIGHT) // horizontal collision
                {
                    Players->Velocity.x = 0; // reverse horizontal velocity
                    // relocate
                    float penetration = Players->Radius - std::abs(diff_vector.x);
                    if (dir == LEFT)
                        Players->Position.x += penetration; 
                    else
                        Players->Position.x -= penetration; 
                }
                else // vertical collision
                {
                    onground = true;
                    Players->Velocity.y = 0; // reverse vertical velocity
                    // relocate
                    float penetration = Players->Radius - std::abs(diff_vector.y);
                    if (dir == UP)
                        Players->Position.y -= penetration; 
                    else
                        Players->Position.y += penetration; 
                }
            }
        }
    }
}

bool CheckCollision(GameObject& one, GameObject& two) // AABB - AABB collision
{
    // collision x-axis?
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
        two.Position.x + two.Size.x >= one.Position.x;
    // collision y-axis?
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
        two.Position.y + two.Size.y >= one.Position.y;
    // collision only if on both axes
    return collisionX && collisionY;
}

Collision CheckCollision(BallObject& one, GameObject& two) // AABB - Circle collision
{
    // get center point circle first 
    glm::vec2 center(one.Position + one.Radius);
    // calculate AABB info (center, half-extents)
    glm::vec2 aabb_half_extents(two.Size.x / 2.0f, two.Size.y / 2.0f);
    glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
    // get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    // now that we know the the clamped values, add this to AABB_center and we get the value of box closest to circle
    glm::vec2 closest = aabb_center + clamped;
    // now retrieve vector between center circle and closest point AABB and check if length < radius
    difference = closest - center;

    if (glm::length(difference) < one.Radius) // not <= since in that case a collision also occurs when object one exactly touches object two, which they are at the end of each collision resolution stage.
        return std::make_tuple(true, VectorDirection(difference), difference);
    else
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
}

// calculates which direction a vector is facing (N,E,S or W)
Direction VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),	// up
        glm::vec2(1.0f, 0.0f),	// right
        glm::vec2(0.0f, -1.0f),	// down
        glm::vec2(-1.0f, 0.0f)	// left
    };
    float max = 0.0f;
    unsigned int best_match = -1;
    for (unsigned int i = 0; i < 4; i++)
    {
        float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction)best_match;
}