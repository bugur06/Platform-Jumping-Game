#include "Framework.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <string>

const char* FILE_PATH_PROJECTILE = "../data/stars1.png";
const char* FILE_PATH_SPRING_BOOTS_ICON = "../data/spring_boots.png";



double WIDTH = 600;
double HEIGHT = 800;
const double PLAYER_STARTER_POSX = 50;
const double PLAYER_STARTER_POSY = HEIGHT - HEIGHT / 4;
const double PLATFORM_DESTROY_DELAY = 3.0;
const double GRAVITY = 0.8;
const int ENEMY_SCORE = 10;
const int PROJECTILE_SPEED = 10;

double playerPosX = PLAYER_STARTER_POSX;

//playerPosY is not used on the player sprite, but is used to calculate the platforms' positions on the screen to create a camera effect.
double playerPosY = PLAYER_STARTER_POSY;
double playerVelocity = 0.0;
double jumpVelocity = 18;

int lastSteppedPlatform = 0;
double lastSteppedPlatformPosY = PLAYER_STARTER_POSY + 200;

bool springBoots = true; // boolean flag to check if the player has spring boots ability
bool springBootsActive = false; // boolean flag to check if the player has spring boots ability active
bool isShoot = false;
bool rightPressed = false;
bool leftPressed = false;
bool upPressed = false;
bool isGameOver = false;
int distanceTraveled = 0;
int platformCount = 0;
int score = 0;
int abilityUsedTime = 0; // time when the ability was used

bool isPlayerOnPlatform = false;


struct DestroyablePlatform {
    int platformIndex;
    bool isDestroyed = false;
    float destroyTime;

    DestroyablePlatform(int index, float time) : platformIndex(index), isDestroyed(false), destroyTime(time) {}
};


struct Projectile {
    double x;         // X position of the projectile
    double y;         // Y position of the projectile
    double mousex;    // X position of the mouse relative to the player
    double mousey;    // Y velocity of the mouse relative to the player
    double length = sqrt(mousex * mousex + mousey * mousey);
    double directionX = mousex / length;
    double directionY = mousey / length;

    Projectile(double xPos, double yPos, double mouseX, double mouseY)
        : x(xPos), y(yPos), mousex(mouseX), mousey(mouseY) {}

    void updatePosition() {
        x += directionX * 10;
        y += directionY * 10;
        x > WIDTH ? x = 0 : x;
        x < 0 ? x = WIDTH : x;
        
    }
};


// 2D vector to store the platforms' positions and types 0 - normal, 1 - breaks 3 seconds after touch
std::vector<std::vector<double>> platformPositions(1000, std::vector<double>(3));
// backup of the platforms' positions to restore them after restarting the game
std::vector<std::vector<double>> platformPositionsBackup(1000, std::vector<double>(3));
std::vector<DestroyablePlatform> DestroyablePlatforms;
std::vector<int> platformTimes;
std::vector<std::vector<double>> enemyPositions(1, std::vector<double>(2));
std::vector<std::vector<double>> enemyPositionsBackup(1, std::vector<double>(2));

class MyFramework : public Framework {
public:
    virtual void PreInit(int& width, int& height, bool& fullscreen)
    {
        width = WIDTH;
        height = HEIGHT;
        fullscreen = false;
    }

    virtual bool Init() {
        return true;
    }

    virtual void Close() {}

    virtual bool Tick() {
        if (isGameOver) {
            RestartGame();
        }
        
        HandleCollision();
        UpdateScore();
        DrawGameObjects();
        PlayerMovement();
        drawScoreSprites(distanceTraveled, lastSteppedPlatform);
        //GeneratePlatforms();
        //MoveCamera();

        return false;
    }

    virtual void onMouseMove(int x, int y, int xrelative, int yrelative) {
        if (isShoot) {
            double xRelativePlayer = x - playerPosX;
            double yRelativePlayer = y - PLAYER_STARTER_POSY;
            Projectile newProjectile(playerPosX, PLAYER_STARTER_POSY, xRelativePlayer, yRelativePlayer);
            projectiles.push_back(newProjectile);
            
            isShoot = false;
        }
    }

    virtual void onMouseButtonClick(FRMouseButton button, bool isReleased) {
        if (button == FRMouseButton::LEFT && !isReleased) {
            isShoot = true;
        }
    }

    virtual void onKeyPressed(FRKey k) {
        if (k == FRKey::RIGHT) {
            rightPressed = true;
        }
        else if (k == FRKey::LEFT) {
            leftPressed = true;
        }
        else if (k == FRKey::UP) {
			upPressed = true;
		}
    }

    virtual void onKeyReleased(FRKey k) {
        if (k == FRKey::RIGHT) {
            rightPressed = false;
        }
        else if (k == FRKey::LEFT) {
            leftPressed = false;
        }
        else if (k == FRKey::UP) {
            upPressed = false;
        }
    }

    virtual const char* GetTitle() override {
        return "Doodle Jump Clone";
    }




    std::vector<Projectile> projectiles;

    void PlayerMovement() {

        if (playerPosX < 0) {
			playerPosX = WIDTH;
		}
        else if (playerPosX > WIDTH) {
			playerPosX = 0;
		}

        double speedMultiplier = (playerPosX > WIDTH / 2) ? 1.5 : 1.0; // if the player is on the right side of the screen, increase the speed by %50
        double PLAYER_SPEED = 8.0 * speedMultiplier;

        if (rightPressed && !leftPressed) {
            playerPosX += PLAYER_SPEED;
        }
        else if (leftPressed && !rightPressed) {
            playerPosX -= PLAYER_SPEED;
        }



        if (playerPosY < HEIGHT - 100) {
            if (playerVelocity < 3) {
				playerVelocity += GRAVITY;
			}
        }

        if (playerPosY > HEIGHT -100 || isPlayerOnPlatform) {
            playerVelocity -= jumpVelocity;
            isPlayerOnPlatform = false;
        }

        
        if (upPressed && springBoots) {
            abilityUsedTime = springBootsAbility(); // this function both sets the jumpVelocity and returns the time when the ability was used
            springBootsActive = true;
        }
        if (getTickCount() - abilityUsedTime > 20000) {
            springBootsActive = false;
			jumpVelocity = 15;
		}







        playerPosY += playerVelocity; // gravity effect

    }

    void HandleCollision() {
        
        for (size_t i = 0; i < platformPositions.size(); i++) {
            double platformPosX = platformPositions[i][0];
            double platformPosY = platformPositions[i][1];
            if (playerPosX + 50 > platformPosX && playerPosX < platformPosX + 100 &&
                playerPosY + 60 > platformPosY && playerPosY < platformPosY + 30 && playerVelocity > 0)  {
                isPlayerOnPlatform = true;

                


                if (lastSteppedPlatform < i) {
					lastSteppedPlatform = i;

                    if (platformPositions[i][2] == 1) {
                        DestroyablePlatforms.push_back(DestroyablePlatform(lastSteppedPlatform, getTickCount() + 3000));
                    }

				}
                
                for (auto& destroyablePlatform : DestroyablePlatforms) {
                    if (destroyablePlatform.platformIndex == i) {
                        if (getTickCount() > destroyablePlatform.destroyTime && !destroyablePlatform.isDestroyed) {
							platformPositions.erase(platformPositions.begin() + destroyablePlatform.platformIndex);
                            destroyablePlatform.isDestroyed = true;
						}
					}
				}


                lastSteppedPlatformPosY = platformPosY;
                



                break;
            }
            
        }

        
        

        for (auto& enemy : enemyPositions) {
            if (playerPosX > enemy[0] && playerPosX < enemy[0] + 80 &&
                playerPosY + 50 > enemy[1] && playerPosY < enemy[1] + 80 && enemy[0] != 0) {
				isGameOver = true;
				break;
			}
		}
        
        //std::cout << lastSteppedPlatformPosY - playerPosY << " " << isGameOver << std::endl;
        

        if (playerPosY > lastSteppedPlatformPosY + 100) {
            isGameOver = true;
        }
    }

    void UpdateScore() {
        distanceTraveled = (PLAYER_STARTER_POSY - playerPosY) > 0 ? (PLAYER_STARTER_POSY - playerPosY) : 0;
        platformCount = static_cast<int>(platformPositions.size());
    }

    virtual void DrawGameObjects() {
        Sprite* platform1 = createSprite("../data/platform1.png");
        Sprite* platform2 = createSprite("../data/platform2.png");
        Sprite* background = createSprite("../data/jungle-bck@2x.png");
        drawSprite(background, 0, 0);
        destroySprite(background);
        // Draw player
        Sprite* myPlayer = createSprite("../data/blue-lik-puca.png");
        drawSprite(myPlayer, (int)playerPosX, (int)PLAYER_STARTER_POSY);
        destroySprite(myPlayer);
        //std::cout << platformPositions.size() << std::endl;
        // Draw platforms
        for (size_t i = 0; i < platformPositions.size(); i++) {
            double platformPosX = platformPositions[i][0];
            double platformPosY = platformPositions[i][1] + PLAYER_STARTER_POSY - playerPosY;
            if (platformPositions[i][2] == 0) {
                drawSprite(platform1, platformPosX, platformPosY);
            }
            else {
                drawSprite(platform2, platformPosX, platformPosY);
            }
                //std::cout << "Platform position: " << platformPositions[i] << std::endl;

        }

        // Draw projectiles
        Sprite* projectileSprite = createSprite("../data/stars1.png");


        int projectileCounter = 0;
        for (auto& projectile : projectiles) { // check if the projectile hit an enemy.
            projectile.updatePosition();
            int enemyCounter = 0;
            for (auto& enemyPosition : enemyPositions) {
                double enemyPosX = enemyPosition[0];
                double enemyPosY = enemyPosition[1] + PLAYER_STARTER_POSY - playerPosY;
                if (projectile.x + 50 > enemyPosX && projectile.x < enemyPosX + 80 &&
                    projectile.y + 50 > enemyPosY && projectile.y < enemyPosY + 150 && enemyPosX != 0) {
                    
                    enemyPosition[0] = -200;
                    enemyPosition[1] = -200;

                    projectiles.erase(projectiles.begin() + projectileCounter);
                    score += ENEMY_SCORE;
                    enemyCounter++;
                    break;
                }

            }


            drawSprite(projectileSprite, projectile.x, projectile.y);

            if (projectile.y > HEIGHT) {
                projectiles.erase(projectiles.begin() + projectileCounter);
            }
            else if (projectile.y < 0) {
                projectiles.erase(projectiles.begin() + projectileCounter);
            }

            projectileCounter++;
        }
        

        Sprite* enemySprite = createSprite("../data/doodlestein-puca@2x.png");
        for (auto& enemyPosition : enemyPositions) {
            
			double posX = enemyPosition[0];
			double posY = enemyPosition[1] + PLAYER_STARTER_POSY - playerPosY;
			
            if (posX) {
                drawSprite(enemySprite, posX, posY);
			}
			
            
		}
        destroySprite(enemySprite);


        if (springBootsActive) { // if the player has the spring boots ability, draw the shoe beneath the player.
			Sprite* springBootsAbilitySprite = createSprite("../data/ach-spring@2x.png");
			drawSprite(springBootsAbilitySprite, 0, 50);
			destroySprite(springBootsAbilitySprite);

            Sprite* springBootsSprite = createSprite("../data/springshoes.png");
            drawSprite(springBootsSprite, playerPosX +15, PLAYER_STARTER_POSY + 50);
            destroySprite(springBootsSprite);
		}
        


        destroySprite(platform1);
        destroySprite(platform2);
        destroySprite(projectileSprite);



    }

    


    void drawScoreSprites(int score, int highestPlatform) {
        std::vector<int> a;         // to store the digits
        std::vector<int> b;         // to store the digits
        const char* SCORE_GENERAL_PATH = "../data/num";

        while (score) {             // loop to extract all digits
            a.push_back(score % 10);
            score /= 10;
        }

        while (highestPlatform) {    // loop to extract all digits
            b.push_back(highestPlatform % 10);
            highestPlatform /= 10;
        }

        for (int j = a.size() - 1; j >= 0; j--) {     // loop to display the digits score
            std::string path = SCORE_GENERAL_PATH + std::to_string(a[j]) + ".png";
            Sprite* scoreSprite = createSprite(path.c_str());
            drawSprite(scoreSprite, 10 + (a.size() - j) * 10, 10);
            destroySprite(scoreSprite);
        }

        for (int j = b.size() - 1; j >= 0; j--) {     // loop to display the digits platform
            std::string path = SCORE_GENERAL_PATH + std::to_string(b[j]) + ".png";
            Sprite* scoreSprite = createSprite(path.c_str());
            drawSprite(scoreSprite, 10 + (b.size() - j) * 10, 30);
            destroySprite(scoreSprite);
        }


    }



    int springBootsAbility() {

        int time;

        if (springBoots) {
            time = getTickCount();
            jumpVelocity *= 1.58;
			springBoots = false;
		}
		return time;
    }



    void RestartGame() {
        playerPosX = PLAYER_STARTER_POSX;
        playerPosY = PLAYER_STARTER_POSY;
        isShoot = false;
        rightPressed = false;
        leftPressed = false;
        isGameOver = false;
        springBoots = true;
        springBootsActive = false;
        abilityUsedTime = 0;
        distanceTraveled = 0;
        platformCount = 0;
        score = 0;
        lastSteppedPlatformPosY = PLAYER_STARTER_POSY + 200;
        lastSteppedPlatform = 0;
        projectiles.clear();
        DestroyablePlatforms.clear();
        platformPositions = platformPositionsBackup;
        enemyPositions = enemyPositionsBackup;
        jumpVelocity = 18;
    }
};


void GeneratePlatforms() {
    for (int i = 0; i < platformPositions.size(); i++) {
        double newPlatformPosX = rand() % ((int)WIDTH - 120);
        double newPlatformPosY = lastSteppedPlatformPosY -  (rand() %50) - 100 - i * 100 ;
        

        platformPositions[i][0] = newPlatformPosX;
        platformPositions[i][1] = newPlatformPosY;
        //platformDestroyFlags.push_back(false);
        if (rand() % 10 == 0) {
            enemyPositions.push_back({ newPlatformPosX - 30, newPlatformPosY - 100 });
        }
        else if (rand() % 10 == 1) {
			platformPositions[i][2] = 1;
		}
    }
    platformPositionsBackup = platformPositions;
    enemyPositionsBackup = enemyPositions;
}







int main(int argc, char* argv[]) {


    if (argc == 3) {
        std::string size = argv[2];
        std::string::size_type xPos = size.find('x');
        std::string widthStr = size.substr(0, xPos);
        std::string heightStr = size.substr(xPos + 1);

        try {
            WIDTH = std::stoi(widthStr);
            HEIGHT = std::stoi(heightStr);
        }
        catch (const std::exception& e) {
            std::cout << "Invalid window size values: " << e.what() << std::endl;
            return 1;
        }
    }
    GeneratePlatforms();

    return run(new MyFramework);
}

