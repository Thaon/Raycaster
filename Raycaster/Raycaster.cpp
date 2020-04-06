#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <string>
#include <vector>


float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

class vec2
{
public:
	int x, y = 0;

	vec2()
	{
		x = 0;
		y = 0;
	}

	vec2(int X, int Y)
	{
		x = X;
		y = Y;
	}
};

class vec2f
{
public:
	float x, y = 0;

	vec2f()
	{
		x = 0;
		y = 0;
	}

	vec2f(float X, float Y)
	{
		x = X;
		y = Y;
	}
};

struct Billboard
{
public:
	Billboard() { pos = vec2f(0, 0); sprite = nullptr; };
	Billboard(vec2f p, olc::Sprite * spr) { pos = p; sprite = spr; }
	vec2f pos;
	olc::Sprite *sprite;
};

class Example : public olc::PixelGameEngine
{
public:
	//game window
	vec2 screen = vec2(120, 80);
	int screenResolution = 8;

	//map
	vec2 map = vec2(16, 16);
	olc::Sprite mapContents = olc::Sprite::Sprite("level.png");
		//map values
		olc::Pixel emptyColour = olc::Pixel(255, 255, 255, 255);
		olc::Pixel wallColour = olc::Pixel(0, 0, 0, 255);
		olc::Pixel bushColour = olc::Pixel(38, 127, 0, 255);

	//sprites
	olc::Sprite wallTexture = olc::Sprite::Sprite("wall.png");
	olc::Sprite bushTexture = olc::Sprite::Sprite("bush.png");

	//billboards
	std::vector<Billboard> billboards;
	float *depthBuffer = nullptr;


	//view
	float MATH_PI = 3.14159f;
	float FOV = MATH_PI / 4;
	float stepSize = .01f;
	vec2f playerPosition = vec2f(8, 8);
	float playerRotation = 0;
	float drawDistance = 16;
	float playerSpeed = 5;
	


	Example()
	{
		sAppName = "Raycaster";
	}

public:
	bool OnUserCreate() override
	{
		map.x = mapContents.width;
		map.y = mapContents.height;

		//init depth buffer
		depthBuffer = new float[screen.x];

		//cache billboards
		for (int y = 0; y < mapContents.height; y++)
		{
			for (int x = 0; x < mapContents.width; x++)
			{
				if (mapContents.GetPixel(x, y) == bushColour)
				{
					billboards.push_back(Billboard(vec2f(x + .5f, y + .5f), &bushTexture));
				}
			}
		}
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		//process input
		if (GetKey(olc::Key::LEFT).bHeld)
			playerRotation -= 2.5f * fElapsedTime;

		if (GetKey(olc::Key::RIGHT).bHeld)
			playerRotation += 2.5f * fElapsedTime;

		if (GetKey(olc::Key::UP).bHeld)
		{
			playerPosition.x += sin(playerRotation) * playerSpeed * fElapsedTime;
			playerPosition.y += cos(playerRotation) * playerSpeed * fElapsedTime;
			if (mapContents.GetPixel(playerPosition.x, playerPosition.y) == wallColour)
			{
				playerPosition.x -= sin(playerRotation) * playerSpeed * fElapsedTime;
				playerPosition.y -= cos(playerRotation) * playerSpeed * fElapsedTime;
			}
		}

		if (GetKey(olc::Key::DOWN).bHeld)
		{
			playerPosition.x -= sin(playerRotation) * playerSpeed * fElapsedTime;
			playerPosition.y -= cos(playerRotation) * playerSpeed * fElapsedTime;
			if (mapContents.GetPixel(playerPosition.x, playerPosition.y) == wallColour)
			{
				playerPosition.x += sin(playerRotation) * playerSpeed * fElapsedTime;
				playerPosition.y += cos(playerRotation) * playerSpeed * fElapsedTime;
			}
		}

		//draw
		DrawView();

		//draw billboards
		for (auto &billboard : billboards)
		{
			//can the object be seen?
			vec2f relPos = vec2f(billboard.pos.x - playerPosition.x, billboard.pos.y - playerPosition.y);
			float distanceFromPlayer = sqrtf(pow(relPos.x, 2) + pow(relPos.y, 2)); //Pythagoras' theorem

			//is the object within the FOV?
			vec2f view = vec2f(sin(playerRotation), cos(playerRotation));
			float billboardAngle = atan2f(view.y, view.x) - atan2f(relPos.y, relPos.x); //angle between 2 vectors
			//cull if not in frustum
			if (billboardAngle < -MATH_PI)
				billboardAngle += MATH_PI * 2.0f;
			if (billboardAngle > MATH_PI)
				billboardAngle -= MATH_PI * 2.0f;

			bool isInView = fabs(billboardAngle) < FOV / 2.0f;

			//discard billboards that are too close or too far
			if (isInView && distanceFromPlayer > .2f && distanceFromPlayer < drawDistance)
			{
				//height
				int billCeilHeight = (float)(screen.y / 2) - (screen.y / (float)distanceFromPlayer);
				int billFloorHeight = screen.y - billCeilHeight;
				float billHeight = billFloorHeight - billCeilHeight;

				//width
				float aspectRatio = float(billboard.sprite->height) / float(billboard.sprite->width);
				float billWidth = billHeight / aspectRatio;

				//set position in screen space
				float billMid = (.5f * (billboardAngle / (FOV / 2.0f)) + .5f) * float(screen.x);

				//finally, draw the billboard
				for (int bx = 0; bx < billWidth; bx++)
				{
					for (int by = 0; by < billHeight; by++)
					{
						vec2f sampledTex = vec2f(bx / billWidth, by / billHeight);
						int billColumn = int(billMid + bx - (billWidth / 2.0f));
						if (billColumn > 0 && billColumn < screen.x)
						{
							olc::Pixel p = billboard.sprite->GetPixel(sampledTex.x * billboard.sprite->width, sampledTex.y * billboard.sprite->height);

							//check if the billboard's column is behind another 3d object
							if (distanceFromPlayer <= depthBuffer[billColumn])
							{
								//draw with alpha here!
								SetPixelMode(olc::Pixel::Mode::ALPHA);
								Draw(
									billColumn,
									billCeilHeight + by,
									p
								);
								SetPixelMode(olc::Pixel::Mode::NORMAL);
							}
						}
					}
				}
			}
		}

		//draw map
		DrawSprite(0, 0, &mapContents);
		Draw(playerPosition.x, playerPosition.y, olc::Pixel(255, 0, 0));

		//let's draw a bit of info about the rotation too
		float viewX = sin(playerRotation);
		float viewY = cos(playerRotation);
		float playerFwdX = playerPosition.x + viewX;
		float playerFwdY = playerPosition.y + viewY;

		Draw(playerFwdX, playerFwdY, olc::Pixel(0, 255, 0));

		return true;
	}

	void DrawView()
	{
		for (int x = 0; x < screen.x; x++)
		{
			//get the center of our field of view
			float rayAngle = (playerRotation - (FOV / 2.0f)) + (((float)x / (float)screen.x) * FOV);

			//calculate distance of each ray to the wall
			float distanceToWall = 0;
			bool hitWall = false;

			//get view forward vector
			vec2f view = vec2f(sin(rayAngle), cos(rayAngle));

			//texture sampling value (only X as Y is implied in the column rendering)
			float sampleX = 0.0f;

			while (!hitWall && distanceToWall < drawDistance)
			{
				distanceToWall += stepSize;

				//move in the forward vector direction and test again
				vec2 hitTest = vec2(
									int(playerPosition.x + (distanceToWall * view.x)), //int because it's a grid
									int(playerPosition.y + (distanceToWall * view.y))  //int because it's a grid
									);

				//discard edge cases
				if (hitTest.x < 0 || hitTest.x >= map.x || hitTest.y < 0 || hitTest.y >= map.y)
				{
					hitWall = true;
					distanceToWall = drawDistance;
				}
				else
				{
					//check cells
					if (mapContents.GetPixel(hitTest.x, hitTest.y) == wallColour)
					{
						hitWall = true;
						//distance to wall is retained from the outside loop

						//get wall mid point (halfway through a unit cell)
						vec2f midPoint = vec2f(float(hitTest.x) + .5f, float(hitTest.y) + .5f);
						//get collision point
						vec2f collPoint = vec2f(playerPosition.x + (distanceToWall * view.x), playerPosition.y + (distanceToWall * view.y));
						/**
						*	now that I have the points, I need the angle between them to determine the sampling main axis
						*	The atan2() and atan2f() functions compute the principal value of the arc tangent of y/x, using the signs of both arguments to determine the quadrant of the return value.
						*	from: https://www.mkssoftware.com/docs/man3/atan2.3.asp
						*/
						float samplingAngle = atan2f(collPoint.y - midPoint.y, collPoint.x - midPoint.x);
						
						float horizontalQuadrant = MATH_PI * .25f; //rotating bisecting axes to fit the quardants to the blocks model
						float verticalQuadrant = MATH_PI * .75f; //rotating bisecting axes to fit the quardants to the blocks model
						
						//check for top and bottom sampling
						if (samplingAngle >= -horizontalQuadrant && samplingAngle < horizontalQuadrant)
							sampleX = collPoint.y - float(hitTest.y);

						if (samplingAngle >= horizontalQuadrant && samplingAngle < verticalQuadrant)
							sampleX = collPoint.x - float(hitTest.x);

						if (samplingAngle < -horizontalQuadrant && samplingAngle >= -verticalQuadrant)
							sampleX = collPoint.x - float(hitTest.x);

						if (samplingAngle >= verticalQuadrant || samplingAngle < -verticalQuadrant)
							sampleX = collPoint.y - float(hitTest.y);
					}
				}
			}

			//we made our checks, let's draw the world!
			int ceilHeight = (float)(screen.y / 2) - (screen.y / (float)distanceToWall);
			int floorHeight = screen.y - ceilHeight;

			//set depth buffer
			depthBuffer[x] = distanceToWall;

			for (int y = 0; y < screen.y; y++)
			{
				if (y < ceilHeight) //ceiling
				{
					Draw(x, y, olc::Pixel(183, 239, 205));
				}
				else if(y >= ceilHeight && y < floorHeight) //wall
				{
					//calculate distance and shade the walls
					//float shade = 54 * (1 - (distanceToWall / drawDistance));
					//Draw(x, y, olc::Pixel(shade, shade, shade));

					if (distanceToWall < drawDistance)
					{
						float fog = (distanceToWall / drawDistance);

						//sample texture Y
						float sampleY = (float(y) - float(ceilHeight)) / (float(floorHeight) - float(ceilHeight));
						olc::Pixel p = wallTexture.GetPixel(sampleX * wallTexture.width, sampleY * wallTexture.height);
						p.r = lerp(p.r, 183, fog);
						p.g = lerp(p.g, 239, fog);
						p.b = lerp(p.b, 205, fog);
						Draw(x, y, p);
					}
					else //draw fog if too far away
					{
						Draw(x, y, olc::Pixel(183, 239, 205));
					}

				}
				else //floor
				{
					//shade the floor
					float shade = (y - screen.y / 2.0f) / (screen.y / 2.0f);

					Draw(x, y, olc::Pixel(76 * shade, 211 * shade, 194 * shade));
				}
			}
		}
	}
};


int main()
{
	Example demo;
	if (demo.Construct(demo.screen.x, demo.screen.y, demo.screenResolution, demo.screenResolution))
		demo.Start();

	return 0;
}