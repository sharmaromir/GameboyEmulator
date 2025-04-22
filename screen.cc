#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

static const int windowWidth = 160;
static const int windowHeight = 144;

// int main(int argc, char** argv) {
//     if (SDL_Init(SDL_INIT_VIDEO) < 0) {
//         fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
//         return 1;
//     }

//     SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

//     if (SDL_SetVideoMode(windowWidth, windowHeight, 32, SDL_OPENGL) == NULL) {
//         fprintf(stderr, "Failed to set video mode: %s\n", SDL_GetError());
//         return 1;
//     }

//     glViewport(0, 0, windowWidth, windowHeight);
//     glMatrixMode(GL_MODELVIEW);
//     glLoadIdentity();
//     glOrtho(0, windowWidth, windowHeight, 0, -1.0, 1.0);

//     glClearColor(0, 0, 0, 1.0);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glShadeModel(GL_FLAT);

//     glEnable(GL_TEXTURE_2D);
//     glDisable(GL_DEPTH_TEST);
//     glDisable(GL_CULL_FACE);
//     glDisable(GL_DITHER);
//     glDisable(GL_BLEND);

//     SDL_GL_SwapBuffers();

//     SDL_Delay(3000);

//     SDL_Quit();
//     return 0;
// }
