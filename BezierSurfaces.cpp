
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

#include "BezierSurfaces.h"


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
const int MAX_CHARS_PER_LINE = 512;
const int MAX_TOKENS_PER_LINE = 20;
const char* const DELIMITER = " ";

Viewport	viewport;
float rA, gA, bA;
float rD, gD, bD;
float rS, gS, bS, v;
std::vector<Light> lights;

string filename;
float subdivisionSize;
boolean isAdaptive;

int numberOfPatches;
vector<Surface> surface_list;

vector<Point> patch_points;

///////////////////////////////////////////////

Vector::Vector() {
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
}

Vector::Vector(float a, float b, float c) {
    x = a;
    y = b;
    z = c;
}

Vector::Vector(Point a, Point b) {
    float scale = sqrt(pow((b.x - a.x), 2) + pow((b.y - a.y), 2) + pow((b.z - a.z), 2));
    x = (b.x - a.x) / scale;
    y = (b.y - a.y) / scale;
    z = (b.z - a.z) / scale;
}

void Vector::normalize() {
    float scale = sqrt(pow((x), 2) + pow((y), 2) + pow((z), 2));
    x /= scale;
    y /= scale;
    z /= scale;
}

void Vector::scalarMult(float s) {
    x *= s;
    y *= s;
    z *= s;
}

Point::Point() {
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
}

Point::Point(float a, float b, float c) {
    x = a;
    y = b;
    z = c;
}

void Point::scalarMult(float s) {
    x *= s;
    y *= s;
    z *= s;
}

Point Point::add(Point p) {
    x += p.x;
    y += p.y;
    z += p.z;
}

Curve::Curve() {

}

Curve::Curve(Point a1, Point b1, Point c1, Point d1) {
    a = a1;
    b = b1;
    c = c1;
    d = d1;
}

Surface::Surface() {

}

Surface::Surface(Curve a1, Curve b1, Curve c1, Curve d1) {
    a = a1;
    b = b1;
    c = c1;
    d = d1;
}

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

/*void circle(float centerX, float centerY, float radius) {
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
}*/
//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {

    glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer

    glMatrixMode(GL_MODELVIEW);			        // indicate we are specifying camera transformations
    glLoadIdentity();				        // make sure transformation is "zero'd"


    // Start drawing
    //circle(viewport.w / 2.0, viewport.h / 2.0, min(viewport.w, viewport.h) * 0.45);


    glFlush();
    glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}

Vector cross(Vector a, Vector b) {
    return Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

Point bezcurveinterp(Curve curve, float u) {
    curve.a.scalarMult(1.0 - u);
    curve.b.scalarMult(u);
    Point a1 = curve.a.add(curve.b);
    curve.b.scalarMult(1.0 - u);
    curve.c.scalarMult(u);
    Point b1 = curve.b.add(curve.c);
    curve.c.scalarMult(1.0 - u);
    curve.d.scalarMult(u);
    Point c1 = curve.c.add(curve.d);

    a1.scalarMult(1.0 - u);
    b1.scalarMult(u);
    Point d1 = a1.add(b1);
    b1.scalarMult(1.0 - u);
    c1.scalarMult(u);
    Point e1 = b1.add(c1);

    d1.scalarMult(1.0 - u);
    e1.scalarMult(u);
    Point p = d1.add(e1);
    Vector der (d1, e1);
    der.scalarMult(3);
    p.derivative = der;

    return p;
}

Point bezpatchinterp(Surface patch, float u, float v) {
    Point va = bezcurveinterp(patch.a, u);
    Point vb = bezcurveinterp(patch.b, u);
    Point vc = bezcurveinterp(patch.c, u);
    Point vd = bezcurveinterp(patch.d, u);
    Curve vcurve(va, vb, vc, vd);

    Point ua = bezcurveinterp(patch.a, v);
    Point ub = bezcurveinterp(patch.b, v);
    Point uc = bezcurveinterp(patch.c, v);
    Point ud = bezcurveinterp(patch.d, v);
    Curve ucurve(ua, ub, uc, ud);

    Point pv = bezcurveinterp(vcurve, v);
    Point pu = bezcurveinterp(ucurve, u);

    Point* p = new Point();
    *p = pu;
    p->normal = cross(pu.derivative, pv.derivative);
    p->normal.normalize();
    return *p;
}

void subdividepatch(Surface patch, float step) {
    float epsilon = 0.0001; //TODO fix maybe
    float numdiv = ((1) / step);

    for (int iu = 0; iu < numdiv; iu++) {
        float u = iu*step;
        for (int iv = 0; iv < numdiv; iv++) {
            float v = iv*step;

            Point p = bezpatchinterp(patch, u, v);

            patch_points.push_back(p);
        }

    }
}

void processArgs(int argc, char *argv[]) {
    filename = string(argv[1]);
    char* temp = argv[1];
    subdivisionSize = strtof(argv[2], &temp);
}

void processFile(char* filename) {
    // create a file-reading object
    ifstream fin;
    fin.open(filename); // open a file
    if (!fin.good()) {
        return; // exit if file not found
    }

    // read each line of the file
    while (!fin.eof())
    {
        // read an entire line into memory
        char buf[MAX_CHARS_PER_LINE];
        fin.getline(buf, MAX_CHARS_PER_LINE);

        // parse the line into blank-delimited tokens
        int n = 0; // a for-loop index

        // array to store memory addresses of the tokens in buf
        const char* token[MAX_TOKENS_PER_LINE] = {}; // initialize to 0

        // parse the line
        token[0] = strtok(buf, DELIMITER); // first token
        if (token[0]) // zero if line is blank
        {
            for (n = 1; n < MAX_TOKENS_PER_LINE; n++)
            {
                token[n] = strtok(0, DELIMITER); // subsequent tokens
                if (!token[n]) break; // no more tokens
            }
        }

        char* temp = "";
        std::string str(token[0]);

        vector<Curve> curve_list;

        // process the tokens
        if (!token[1]) { //pls
            numberOfPatches = atoi(token[0]);
        }
        else {
            Point a;
            a.x = strtof(token[0], &temp);
            a.y = strtof(token[1], &temp);
            a.z = strtof(token[2], &temp);
            Point b;
            b.x = strtof(token[3], &temp);
            b.y = strtof(token[4], &temp);
            b.z = strtof(token[5], &temp);
            Point c;
            c.x = strtof(token[6], &temp);
            c.y = strtof(token[7], &temp);
            c.z = strtof(token[8], &temp);
            Point d;
            d.x = strtof(token[9], &temp);
            d.y = strtof(token[10], &temp);
            d.z = strtof(token[11], &temp);

            Curve curve(a, b, c, d);
            curve_list.push_back(curve);

            if (curve_list.size() == 4) {
                Surface s(curve_list[0], curve_list[1], curve_list[2], curve_list[3]);
                curve_list.clear(); //TODO hopefully this works
                surface_list.push_back(s);
            }
        }
    }
}

void keyboard(unsigned char key, int xmouse, int ymouse)
{
    switch (key) {
    case ' ':
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








