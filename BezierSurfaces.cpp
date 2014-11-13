
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <time.h>
#include <math.h>


#define PI 3.14159265  // Should be used from mathlib
inline float sqr(float x) { return x*x; }

#define SPACEBAR 32

using namespace std;

//****************************************************
// Some Classes
//****************************************************

class Viewport;

class Viewport {
public:
    int w, h; // width and height
};

class Light;

class Light {
public:
    float r;
    float g;
    float b;
    float x;
    float y;
    float z;
    char type;
};


//****************************************************
// Global Variables
//****************************************************
Viewport	viewport;
float rA, gA, bA;
float rD, gD, bD;
float rS, gS, bS, v;
std::vector<Light> lights;


//****************************************************
// Simple init function
//****************************************************
void initScene(){

    // Nothing to do here for this simple example.

}


//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
    viewport.w = w;
    viewport.h = h;

    glViewport(0, 0, viewport.w, viewport.h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, viewport.w, 0, viewport.h);

}


//****************************************************
// A routine to set a pixel by drawing a GL point.  This is not a
// general purpose routine as it assumes a lot of stuff specific to
// this example.
//****************************************************

void setPixel(int x, int y, GLfloat r, GLfloat g, GLfloat b) {
    glColor3f(r, g, b);
    glVertex2f(x + 0.5, y + 0.5);   // The 0.5 is to target pixel
    // centers 
    // Note: Need to check for gap
    // bug on inst machines.
}

//****************************************************
// Draw a filled circle.  
//****************************************************

std::vector<float> getAmbient(Light l){
    std::vector<float> amb = { rA*l.r, gA*l.g, bA*l.b };
    return amb;
}

std::vector<float> getDiffuse(Light l, std::vector<float> point, float radius){
    std::vector<float> dir;
    if (l.type == 'p') {
        float scale = sqrt(pow((point[0] - l.x), 2) + pow((point[1] - l.y), 2) + pow((point[2] - l.z),2));
        dir = { (point[0] - l.x) / scale, (point[1] - l.y) / scale, (point[2] - l.z) / scale };
    }
    else /*if (l.type == 'd')*/ {
        float scale = sqrt(pow((l.x), 2) + pow((l.y), 2) + pow((l.z), 2));
        dir = { (l.x) / scale, (l.y) / scale, (l.z) / scale };
    }
    float scaleN = sqrt(pow((0.0f - point[0]), 2) + pow((0.0f - point[1]), 2) + pow((0.0f - point[2]), 2));
    std::vector<float> normal = { (0.0f - point[0]) / scaleN, (0.0f - point[1]) / scaleN, (0.0f - point[2]) / scaleN };

    float term = max(dir[0] * normal[0] + dir[1] * normal[1] + dir[2] * normal[2], 0.0f);

    std::vector<float> diff = { rD*l.r*term, gD*l.g*term, bD*l.b*term };
    return diff;
}

std::vector<float> getSpecular(Light l, std::vector<float> point, float radius){
    std::vector<float> dir;
    float scaleN = sqrt(pow((0.0f - point[0]), 2) + pow((0.0f - point[1]), 2) + pow((0.0f - point[2]), 2));
    std::vector<float> normal = { (0.0f - point[0]) / scaleN, (0.0f - point[1]) / scaleN, (0.0f - point[2]) / scaleN };
    if (l.type == 'p') {
        float scale = sqrt(pow((point[0] - l.x), 2) + pow((point[1] - l.y), 2) + pow((point[2] - l.z), 2));
        dir = { (point[0] - l.x) / scale, (point[1] - l.y) / scale, (point[2] - l.z) / scale };
    }
    else /*if (l.type == 'd')*/ {
        float scale = sqrt(pow((l.x), 2) + pow((l.y), 2) + pow((l.z), 2));
        dir = { (l.x) / scale, (l.y) / scale, (l.z) / scale };
    }

    //Rr = Ri - 2 N (Ri . N)
    float dirDotN = dir[0] * normal[0] + dir[1] * normal[1] + dir[2] * normal[2];
    std::vector<float> ref = { dir[0] - (2 * normal[0] * dirDotN), dir[1] - (2 * normal[1] * dirDotN), dir[2] - (2 * normal[2] * dirDotN) };
    std::vector<float> view = { 0.0f, 0.0f, 1.0f};
    
    float term = pow(max(ref[0] * view[0] + ref[1] * view[1] + ref[2] * view[2], 0.0f), v);

    std::vector<float> spec = { rS*l.r*term, gS*l.g*term, bS*l.b*term };
    return spec;
}

void circle(float centerX, float centerY, float radius) {
    // Draw inner circle
    glBegin(GL_POINTS);

    // We could eliminate wasted work by only looping over the pixels
    // inside the sphere's radius.  But the example is more clear this
    // way.  In general drawing an object by loopig over the whole
    // screen is wasteful.

    int i, j;  // Pixel indices

    int minI = max(0, (int)floor(centerX - radius));
    int maxI = min(viewport.w - 1, (int)ceil(centerX + radius));

    int minJ = max(0, (int)floor(centerY - radius));
    int maxJ = min(viewport.h - 1, (int)ceil(centerY + radius));

    

    for (i = 0; i < viewport.w; i++) {
        for (j = 0; j < viewport.h; j++) {
            

            // Location of the center of pixel relative to center of sphere
            float x = (i + 0.5 - centerX);
            float y = (j + 0.5 - centerY);

            float dist = sqrt(sqr(x) + sqr(y));

            if (dist <= radius) {

                // This is the front-facing Z coordinate
                float z = sqrt(radius*radius - dist*dist);
                float r = 0.0f;
                float b = 0.0f;
                float g = 0.0f;

                for (Light p : lights) {
                    if (i == 150 && j == 150) {
                        int c = 2;
                    }//TODO

                    std::vector<float> amb = getAmbient(p);
                    r += amb[0];
                    g += amb[1];
                    b += amb[2];

                    std::vector<float> point = { x / radius, y / radius, z / radius };
                    std::vector<float> diff = getDiffuse(p, point, radius);
                    r += diff[0];
                    g += diff[1];
                    b += diff[2];

                    std::vector<float> spec = getSpecular(p, point, radius);
                    r += spec[0];
                    g += spec[1];
                    b += spec[2];

                    if (i == 150 && j == 150) {
                        int c = 2;
                    }//TODO
                }
                if (i == 150 && j == 150) {
                    int c = 2;
                }//TODO

                setPixel(i, j, r, g, b);

                // This is amusing, but it assumes negative color values are treated reasonably.
                // setPixel(i,j, x/radius, y/radius, z/radius );
            }


        }
    }


    glEnd();
}
//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {

    glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer

    glMatrixMode(GL_MODELVIEW);			        // indicate we are specifying camera transformations
    glLoadIdentity();				        // make sure transformation is "zero'd"


    // Start drawing
    circle(viewport.w / 2.0 , viewport.h / 2.0 , min(viewport.w, viewport.h) * 0.45);
    

    glFlush();
    glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}

void processArgs(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        char* temp = argv[i];
        std::string str(argv[i]);

        if (str.compare("-ka") == 0) {
            rA = strtof(argv[i + 1], &temp);
            gA = strtof(argv[i + 2], &temp);
            bA = strtof(argv[i + 3], &temp);
            i = i + 3;
        }
        else if (str.compare("-kd") == 0) {
            rD = strtof(argv[i + 1], &temp);
            gD = strtof(argv[i + 2], &temp);
            bD = strtof(argv[i + 3], &temp);
            i = i + 3;
        }
        else if (str.compare("-ks") == 0) {
            rS = strtof(argv[i + 1], &temp);
            gS = strtof(argv[i + 2], &temp);
            bS = strtof(argv[i + 3], &temp);
            i = i + 3;
        }
        else if (str.compare("-sp") == 0) {
            v = strtof(argv[i + 1], &temp);
            i = i + 1;
        }
        else if (str.compare("-pl") == 0) {
            Light l;
            l.x = strtof(argv[i + 1], &temp);
            l.y = strtof(argv[i + 2], &temp);
            l.z = strtof(argv[i + 3], &temp);
            l.r = strtof(argv[i + 4], &temp);
            l.b = strtof(argv[i + 5], &temp);
            l.g = strtof(argv[i + 6], &temp);
            l.type = 'p';
            lights.push_back(l);
            i = i + 6;
        }
        else if (str.compare("-dl") == 0) {
            Light l;
            l.x = strtof(argv[i + 1], &temp);
            l.y = strtof(argv[i + 2], &temp);
            l.z = strtof(argv[i + 3], &temp);
            l.r = strtof(argv[i + 4], &temp);
            l.b = strtof(argv[i + 5], &temp);
            l.g = strtof(argv[i + 6], &temp);
            l.type = 'd';
            lights.push_back(l);
            i = i + 6;
        }
    }
}

void keyboard(unsigned char key, int xmouse, int ymouse)
{
    if (key == ' '){
        exit(0);
    }
}

//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {
    processArgs(argc, argv);

    //This initializes glut
    glutInit(&argc, argv);

    //This tells glut to use a double-buffered window with red, green, and blue channels 
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // Initalize theviewport size
    viewport.w = 400;
    viewport.h = 400;

    //The size and position of the window
    glutInitWindowSize(viewport.w, viewport.h);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(argv[0]);

    initScene();							// quick function to set up scene

    glutDisplayFunc(myDisplay);				// function to run when its time to draw something
    glutReshapeFunc(myReshape);				// function to run when the window gets resized

    glutMainLoop();							// infinite loop that will keep drawing and resizing
    // and whatever else
    glutKeyboardFunc(keyboard);

    return 0;
}








