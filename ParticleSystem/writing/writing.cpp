#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

/* all the external library is in
 * the external directory under solution directory
 **/
#include <SDL.h>

/* From spark project
 * Get source from http://spark.developpez.com/index.php?page=features&lang=en
 * project build include two parts: 1)Spark core 2)openGL rendering 
 **/
#include "SPK.h"
#include "SPK_GL.h"
#ifdef _DEBUG
#pragma comment(lib, "sparkd.lib")
#else
#pragma comment(lib, "spark.lib")
#endif

/* In debug mode, show the console window for debugging
 * else in release, hide the console window
 */
#ifdef _DEBUG
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
#else
#pragma comment(linker, "/subsystem:windows /enry:mainCRTStartup")
#endif

using namespace std;
using namespace SPK;
using namespace SPK::GL;

float angleY = 0.0f;
float angleX = 0.0f;
float camPosZ = 5.0f;

int deltaTime = 0;

Group* particleGroup = NULL;
System* particleSystem = NULL;

const float PI = 3.14159265358979323846f;

int screenWidth;
int screenHeight;
float screenRatio;

// Converts an int into a string
string int2Str(int a)
{
    ostringstream stm;
    stm << a;
    return stm.str();
}

// Loads a texture
bool loadTexture(GLuint& index,char* path,GLuint type,GLuint clamp,bool mipmap)
{
    SDL_Surface *particleImg;
    particleImg = SDL_LoadBMP(path);
    if (particleImg == NULL) {
        cout << "Unable to load bitmap :" << SDL_GetError() << endl;
        return false;
    }

    // converts from BGR to RGB
    if ((type == GL_RGB)||(type == GL_RGBA)) {
        const int offset = (type == GL_RGB ? 3 : 4);
        unsigned char* iterator = static_cast<unsigned char*>(particleImg->pixels);
        unsigned char *tmp0,*tmp1;
        for (int i = 0; i < particleImg->w * particleImg->h; ++i) {
            tmp0 = iterator;
            tmp1 = iterator + 2;
            swap(*tmp0,*tmp1);
            iterator += offset;
        }
    }

    glGenTextures(1,&index);
    glBindTexture(GL_TEXTURE_2D,index);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,clamp);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,clamp);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	//if enable mipmap, set the min filter to be the correct mipmap
    if (mipmap) {
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);

        gluBuild2DMipmaps(GL_TEXTURE_2D,
                          type,
                          particleImg->w,
                          particleImg->h,
                          type,
                          GL_UNSIGNED_BYTE,
                          particleImg->pixels);
    } else {
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     type,
                     particleImg->w,
                     particleImg->h,
                     0,
                     type,
                     GL_UNSIGNED_BYTE,
                     particleImg->pixels);
    }

    SDL_FreeSurface(particleImg);

    return true;
}

void drawBoundingBox(const Group& group)
{
    if (!group.isAABBComputingEnabled())
        return;

    Vector3D AABBMin = group.getAABBMin();
    Vector3D AABBMax = group.getAABBMax();

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor3f(1.0f,0.0f,0.0f);

    glVertex3f(AABBMin.x,AABBMin.y,AABBMin.z);
    glVertex3f(AABBMax.x,AABBMin.y,AABBMin.z);

    glVertex3f(AABBMin.x,AABBMin.y,AABBMin.z);
    glVertex3f(AABBMin.x,AABBMax.y,AABBMin.z);

    glVertex3f(AABBMin.x,AABBMin.y,AABBMin.z);
    glVertex3f(AABBMin.x,AABBMin.y,AABBMax.z);

    glVertex3f(AABBMax.x,AABBMax.y,AABBMax.z);
    glVertex3f(AABBMin.x,AABBMax.y,AABBMax.z);

    glVertex3f(AABBMax.x,AABBMax.y,AABBMax.z);
    glVertex3f(AABBMax.x,AABBMin.y,AABBMax.z);

    glVertex3f(AABBMax.x,AABBMax.y,AABBMax.z);
    glVertex3f(AABBMax.x,AABBMax.y,AABBMin.z);

    glVertex3f(AABBMin.x,AABBMin.y,AABBMax.z);
    glVertex3f(AABBMax.x,AABBMin.y,AABBMax.z);

    glVertex3f(AABBMin.x,AABBMin.y,AABBMax.z);
    glVertex3f(AABBMin.x,AABBMax.y,AABBMax.z);

    glVertex3f(AABBMin.x,AABBMax.y,AABBMin.z);
    glVertex3f(AABBMax.x,AABBMax.y,AABBMin.z);

    glVertex3f(AABBMin.x,AABBMax.y,AABBMin.z);
    glVertex3f(AABBMin.x,AABBMax.y,AABBMax.z);

    glVertex3f(AABBMax.x,AABBMin.y,AABBMin.z);
    glVertex3f(AABBMax.x,AABBMax.y,AABBMin.z);

    glVertex3f(AABBMax.x,AABBMin.y,AABBMin.z);
    glVertex3f(AABBMax.x,AABBMin.y,AABBMax.z);
    glEnd();
}

void renderFirstFrame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapBuffers();
}

void render()
{
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f,screenRatio,0.01f,80.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	//enable depth test for drawing the bounding box
    glTranslatef(0.0f,0.0f,-camPosZ);

    glDisable(GL_BLEND);
    drawBoundingBox(*particleGroup);

    particleSystem->render();

    SDL_GL_SwapBuffers();
}

// Main function
int main(int argc, char *argv[])
{
    // random seed
    randomSeed = static_cast<unsigned int>(time(NULL));

    // Sets the update step
    //encase the update of the system
    System::setClampStep(true,0.1f);			// clamp the step to 100 ms
    System::useAdaptiveStep(0.001f,0.01f);		// use an adaptive step from 1ms to 10ms (1000fps to 100fps)

    SDL_Event event;

    // init SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("SPARK Writing Demo",NULL);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL,0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
    SDL_SetVideoMode(1024,768,32,SDL_OPENGL | SDL_DOUBLEBUF);
    SDL_ShowCursor(0);
	
    SDL_Surface screen = *SDL_GetVideoSurface();
    renderFirstFrame();

    // init openGL
    screenWidth = screen.w;
    screenHeight = screen.h;
    screenRatio = (float)screen.w / (float)screen.h;

    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glViewport(0,0,screen.w,screen.h);

    // Loads particle texture
    GLuint textureParticle;
    if (!loadTexture(textureParticle,"../res/point.bmp",GL_ALPHA,GL_CLAMP,false))
        return 1;

    // Inits Particle Engine & Render
    GLPointRenderer* pointRender = GLPointRenderer::create();

    pointRender->setType(SPK::POINT_SPRITE);
    pointRender->enableWorldSize(true);
    SPK::GL::GLPointRenderer::setPixelPerUnit(45.0f * 3.14159f / 180.f,screenHeight,camPosZ);
    pointRender->setSize(0.5f);
    pointRender->setTexture(textureParticle);
    pointRender->enableBlending(true);
    pointRender->setBlendingFunctions(GL_SRC_ALPHA, GL_ONE);
    pointRender->setTextureBlending(GL_MODULATE);
    pointRender->enableRenderingHint(DEPTH_TEST,false);

    // Model
    Model* particleModel = Model::create(FLAG_RED | FLAG_GREEN | FLAG_BLUE | FLAG_ALPHA | FLAG_SIZE,FLAG_ALPHA | FLAG_SIZE);
    particleModel->setParam(PARAM_ALPHA,1.0f,0.0f);	// the particles will fade as they die
    particleModel->setParam(PARAM_SIZE,1.0f,15.0f);	// the particles will enlarge over time
    particleModel->setLifeTime(5.0f,1000.0f);

    // Emitter : set up a spheric emitter that emits in all direction with a very small force in order to slightly displace the particles
    RandomEmitter* particleEmitter = RandomEmitter::create();
	SPK::Point* source = SPK::Point::create();
	particleEmitter->setZone(source);
    particleEmitter->setForce(0.01f,0.01f);
	particleEmitter->setFlow(-1);
	particleEmitter->setTank(1000);

    // Group
    particleGroup = Group::create(particleModel,14000);
    particleGroup->setFriction(-0.3f); // negative friction : The particles will accelerate over time
	particleGroup->setRenderer(pointRender);

    // System
    particleSystem = System::create();
    particleSystem->addGroup(particleGroup);

    // This computes the ratio to go from screen coordinates to universe coordinates
    float screenToUniverse = 2.0f * camPosZ * tan(45.0f * 0.5f * PI / 180.0f) / screenHeight;

    float oldX,oldY,oldZ;
    bool add = false;
    float offset = 0.0f;

    bool exit = false;
    bool paused = false;

    float step = 0.0f;

    SDL_ShowCursor(1);

    std::deque<unsigned int> frameFPS;
    frameFPS.push_back(SDL_GetTicks());

	//Main Event Loop
    while(!exit) {
        while (SDL_PollEvent(&event)) {
            // if esc is pressed, exit
            if ((event.type == SDL_KEYDOWN)&&(event.key.keysym.sym == SDLK_ESCAPE))
                exit = true;

            // if del is pressed, reinit the system
            if ((event.type == SDL_KEYDOWN)&&(event.key.keysym.sym == SDLK_DELETE))
                particleSystem->empty();

            // if F1 is pressed, we display or not the bounding boxes
            if ((event.type == SDL_KEYDOWN)&&(event.key.keysym.sym == SDLK_F1)) {
                particleGroup->enableAABBComputing(!particleGroup->isAABBComputingEnabled());

                if (paused)
                    particleSystem->computeAABB();
            }

            // if pause is pressed, the system is paused
            if ((event.type == SDL_KEYDOWN)&&(event.key.keysym.sym == SDLK_PAUSE))
                paused = !paused;

			// trace the mouse movement, record the last point
            oldZ = camPosZ;
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    add = true;
                    oldX = (event.motion.x - screenWidth * 0.5f) * screenToUniverse;
                    oldY = -(event.motion.y - screenHeight * 0.5f) * screenToUniverse;
                }
                if (event.button.button == SDL_BUTTON_WHEELDOWN)
                    camPosZ = min(18.0f,camPosZ + 0.5f);
                if (event.button.button == SDL_BUTTON_WHEELUP)
                    camPosZ = max(0.0f,camPosZ - 0.5f);
            }

            if (event.type == SDL_MOUSEBUTTONUP)
                if (event.button.button == SDL_BUTTON_LEFT)
                    add = false;

			//add the particle from the old point to the new point uniformly, and emitter the
			//particles in random directions
            if ((add)&&(!paused)) {
                screenToUniverse = camPosZ * tan(22.5f * 3.1415926 / 180) * 2 / screenHeight;
                float x = (event.motion.x - screenWidth * 0.5f) * screenToUniverse;
                float y = -(event.motion.y - screenHeight * 0.5f) * screenToUniverse;

                //here set z is zero means not to consider the projection matrix
                offset = particleGroup->addParticles(Vector3D(oldX,oldY,0.0f),Vector3D(x,y,0.0f),particleEmitter,0.025f,offset);
                oldX = x;
                oldY = y;
            }
        }

        if (!paused) {
            // Changes the color of the model over time
            step += deltaTime * 0.0005f;
            particleModel->setParam(PARAM_RED,0.6f + 0.4f * sin(step));
            particleModel->setParam(PARAM_GREEN,0.6f + 0.4f * sin(step + PI * 2.0f / 3.0f));
            particleModel->setParam(PARAM_BLUE,0.6f + 0.4f * sin(step + PI * 4.0f / 3.0f));

            // Updates particle system
            particleSystem->update(deltaTime * 0.001f); // 1 being a second
        }

        // Renders scene
        render();

        // Computes delta time
        int time = SDL_GetTicks();
        deltaTime = time - frameFPS.back();

        frameFPS.push_back(time);

		//delete the time if the time delta is over 1 second
        while((frameFPS.back() - frameFPS.front() > 1000)&&(frameFPS.size() > 2))
            frameFPS.pop_front();
    }

	//free the memory
    SPKFactory::getInstance().destroyAll();

	//free the SDL context
    SDL_Quit();

    return 0;
}