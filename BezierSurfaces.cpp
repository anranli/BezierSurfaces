
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
using namespace std;

//****************************************************
// Some Classes
//****************************************************

class Viewport;

class Viewport {
public:
    int w, h; // width and height
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

string filename;
float subdivisionSize;
boolean isAdaptive;
bool flatShading;
bool filledPolys;
bool keyBuffer[256];
bool prevKeyBuffer[256];
int numberOfPatches;
float xVal;
float yVal;
float xRotVal;
float yRotVal;
float zoom;
vector<Surface> surface_list;

int numdiv;
std::vector<std::vector<Point> > patch_points;
vector<Triangle> triangle_list;

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

Vector Vector::scalarMult(float s) {
    return Vector(x*s, y*s, z*s);
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

Point Point::scalarMult(float s) {
    return Point(x*s, y*s, z*s);
}

Point Point::add(Point p) {
    return Point(x + p.x, y + p.y, z + p.z);
}

float Point::distance(Point p) {
    return sqrt(pow((x - p.x), 2) + pow((y - p.y), 2) + pow((z - p.z), 2));
}

Point Point::midpoint(Point p) {
    return Point((x + p.x) / 2, (y - p.x) / 2, (z - p.z) / 2);
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

Triangle::Triangle() {

}

Triangle::Triangle(Point a1, Point b1, Point c1){
    a = a1;
    b = b1;
    c = c1;
}

//****************************************************
// logic below
//***************************************************



Vector cross(Vector a, Vector b) {
    return Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

Point bezcurveinterp(Curve curve, float u) {
    Point a1 = curve.a.scalarMult(1.0 - u).add(curve.b.scalarMult(u));
    Point b1 = curve.b.scalarMult(1.0 - u).add(curve.c.scalarMult(u));
    Point c1 = curve.c.scalarMult(1.0 - u).add(curve.d.scalarMult(u));

    Point d1 = a1.scalarMult(1.0 - u).add(b1.scalarMult(u));
    Point e1 = b1.scalarMult(1.0 - u).add(c1.scalarMult(u));

    Point p = d1.scalarMult(1.0 - u).add(e1.scalarMult(u));
    Vector der(d1, e1); //TODO is this normalized??
    p.derivative = der.scalarMult(3);

    return p;
}

Point bezpatchinterp(Surface patch, float u, float v) {
    Point va = bezcurveinterp(patch.a, u);
    Point vb = bezcurveinterp(patch.b, u);
    Point vc = bezcurveinterp(patch.c, u);
    Point vd = bezcurveinterp(patch.d, u);
    Curve vcurve(va, vb, vc, vd);

    Curve c1(patch.a.a, patch.b.a, patch.c.a, patch.d.a);
    Curve c2(patch.a.b, patch.b.b, patch.c.b, patch.d.b);
    Curve c3(patch.a.c, patch.b.c, patch.c.c, patch.d.c);
    Curve c4(patch.a.d, patch.b.d, patch.c.d, patch.d.d);
    Point ua = bezcurveinterp(c1, v);
    Point ub = bezcurveinterp(c2, v);
    Point uc = bezcurveinterp(c3, v);
    Point ud = bezcurveinterp(c4, v);
    Curve ucurve(ua, ub, uc, ud);

    Point pv = bezcurveinterp(vcurve, v);
    Point pu = bezcurveinterp(ucurve, u);

    Point* p = new Point();
    *p = pu;
    p->normal1 = cross(pu.derivative, pv.derivative);
    p->normal1.normalize();
    p->normal2 = cross(pv.derivative, pu.derivative);
    p->normal2.normalize();
    return *p;
}

void subdividepatchadaptive(Surface patch, float epsilon, Triangle t, float depth) {
    Point e1m = t.a.midpoint(t.b);
    Point e2m = t.b.midpoint(t.c);
    Point e3m = t.c.midpoint(t.a);

    float abu = (t.au + t.bu) / 2;
    float abv = (t.av + t.bv) / 2;
    float bcu = (t.bu + t.cu) / 2;
    float bcv = (t.bv + t.cv) / 2;
    float cau = (t.cu + t.au) / 2;
    float cav = (t.cv + t.av) / 2;

    Point e1i = bezpatchinterp(patch, abu, abv);
    Point e2i = bezpatchinterp(patch, bcu, bcv);
    Point e3i = bezpatchinterp(patch, cau, cav);

    bool e1 = e1m.distance(e1i) < epsilon;
    bool e2 = e2m.distance(e2i) < epsilon;
    bool e3 = e3m.distance(e3i) < epsilon;

    if (e1 && e2 && e3 || depth > 2) {
        triangle_list.push_back(t);
    }
    else if (!e1 && e2 && e3){
        Triangle t1(t.a, e1i, t.c);
        t1.au = t.au;
        t1.av = t.av;
        t1.bu = abu;
        t1.bv = abv;
        t1.cu = t.cu;
        t1.cv = t.cv;
        subdividepatchadaptive(patch, epsilon, t1, depth + 1);

        Triangle t2(e1i, t.b, t.c);
        t2.au = abu;
        t2.av = abv;
        t2.bu = t.bu;
        t2.bv = t.bv;
        t2.cu = t.cu;
        t2.cv = t.cv;
        subdividepatchadaptive(patch, epsilon, t2, depth + 1);
    }
    else if (e1 && !e2 && e3) {
        Triangle t1(t.a, t.b, e2i);
        t1.au = t.au;
        t1.av = t.av;
        t1.bu = t.bu;
        t1.bv = t.bv;
        t1.cu = bcu;
        t1.cv = bcv;
        subdividepatchadaptive(patch, epsilon, t1, depth + 1);

        Triangle t2(t.a, e2i, t.c);
        t2.au = t.au;
        t2.av = t.av;
        t2.bu = bcu;
        t2.bv = bcv;
        t2.cu = t.cu;
        t2.cv = t.cv;
        subdividepatchadaptive(patch, epsilon, t2, depth + 1);
    }
    else if (e1 && e2 && !e3) {
        Triangle t1(t.a, t.b, e3i);
        t1.au = t.au;
        t1.av = t.av;
        t1.bu = t.bu;
        t1.bv = t.bv;
        t1.cu = cau;
        t1.cv = cav;
        subdividepatchadaptive(patch, epsilon, t1, depth + 1);

        Triangle t2(e3i, t.b, t.c);
        t2.au = cau;
        t2.av = cav;
        t2.bu = t.bu;
        t2.bv = t.bv;
        t2.cu = t.cu;
        t2.cv = t.cv;
        subdividepatchadaptive(patch, epsilon, t2, depth + 1);
    }
    else if (!e1 && !e2 && e3) {
        Triangle t1(t.a, e1i, e2i);
        t1.au = t.au;
        t1.av = t.av;
        t1.bu = abu;
        t1.bv = abv;
        t1.cu = bcu;
        t1.cv = bcv;
        subdividepatchadaptive(patch, epsilon, t1, depth + 1);

        Triangle t2(e1i, t.b, e2i);
        t2.au = abu;
        t2.av = abv;
        t2.bu = t.bu;
        t2.bv = t.bv;
        t2.cu = bcu;
        t2.cv = bcv;
        subdividepatchadaptive(patch, epsilon, t2, depth + 1);

        Triangle t3(t.a, e2i, t.c);
        t3.au = t.au;
        t3.av = t.av;
        t3.bu = bcu;
        t3.bv = bcv;
        t3.cu = t.cu;
        t3.cv = t.cv;
        subdividepatchadaptive(patch, epsilon, t3, depth + 1);
    }
    else if (e1 && !e2 && !e3) {
        Triangle t1(t.a, t.b, e3i);
        t1.au = t.au;
        t1.av = t.av;
        t1.bu = t.bu;
        t1.bv = t.bv;
        t1.cu = cau;
        t1.cv = cav;
        subdividepatchadaptive(patch, epsilon, t1, depth + 1);

        Triangle t2(e3i, t.b, e2i);
        t2.au = cau;
        t2.av = cav;
        t2.bu = t.bu;
        t2.bv = t.bv;
        t2.cu = bcu;
        t2.cv = bcv;
        subdividepatchadaptive(patch, epsilon, t2, depth + 1);

        Triangle t3(e3i, e2i, t.c);
        t3.au = cau;
        t3.av = cav;
        t3.bu = bcu;
        t3.bv = bcv;
        t3.cu = t.cu;
        t3.cv = t.cv;
        subdividepatchadaptive(patch, epsilon, t3, depth + 1);
    }
    else if (!e1 && e2 && !e3) {
        Triangle t1(t.a, e1i, e3i);
        t1.au = t.au;
        t1.av = t.av;
        t1.bu = abu;
        t1.bv = abv;
        t1.cu = cau;
        t1.cv = cav;
        subdividepatchadaptive(patch, epsilon, t1, depth + 1);

        Triangle t2(e1i, e3i, t.c);
        t2.au = abu;
        t2.av = abv;
        t2.bu = cau;
        t2.bv = cav;
        t2.cu = t.cu;
        t2.cv = t.cv;
        subdividepatchadaptive(patch, epsilon, t2, depth + 1);

        Triangle t3(e1i, t.b, t.c);
        t3.au = abu;
        t3.av = abv;
        t3.bu = t.bu;
        t3.bv = t.bv;
        t3.cu = t.cu;
        t3.cv = t.cv;
        subdividepatchadaptive(patch, epsilon, t3, depth + 1);
    }
    else {
        Triangle t1(t.a, e1i, e3i);
        t1.au = t.au;
        t1.av = t.av;
        t1.bu = abu;
        t1.bv = abv;
        t1.cu = cau;
        t1.cv = cav;
        subdividepatchadaptive(patch, epsilon, t1, depth + 1);

        Triangle t2(e1i, t.b, e2i);
        t2.au = abu;
        t2.av = abv;
        t2.bu = t.bu;
        t2.bv = t.bv;
        t2.cu = bcu;
        t2.cv = bcv;
        subdividepatchadaptive(patch, epsilon, t2, depth + 1);

        Triangle t3(e3i, e2i, t.c);
        t3.au = cau;
        t3.av = cav;
        t3.bu = bcu;
        t3.bv = bcv;
        t3.cu = t.cu;
        t3.cv = t.cv;
        subdividepatchadaptive(patch, epsilon, t3, depth + 1);

        Triangle t4(e1i, e2i, e3i);
        t4.au = abu;
        t4.av = abv;
        t4.bu = bcu;
        t4.bv = bcv;
        t4.cu = cau;
        t4.cv = cav;
        subdividepatchadaptive(patch, epsilon, t4, depth + 1);
    }
}

void subdividepatch(Surface patch, float step) {
    //adaptive
    if (isAdaptive) {
        Triangle t1(patch.a.a, patch.d.a, patch.d.d);
        t1.au = 0;
        t1.av = 0;
        t1.bu = 0;
        t1.bv = 1;
        t1.cu = 1;
        t1.cv = 1;
        subdividepatchadaptive(patch, step, t1, 1);

        Triangle t2(patch.a.a, patch.a.d, patch.d.d);
        t2.au = 0;
        t2.av = 0;
        t2.bu = 1;
        t2.bv = 0;
        t2.cu = 1;
        t2.cv = 1;
        subdividepatchadaptive(patch, step, t2, 1);
    }
    else {
        //float epsilon = 0.0001; //TODO fix maybe
        numdiv = (1 / step);
        float newstep = 1.0 / numdiv;

        for (int iu = 0; iu <= numdiv; iu++) {
            float u = iu*newstep;
            patch_points.push_back(vector<Point>());
            for (int iv = 0; iv <= numdiv; iv++) {
                float v = iv*newstep;

                Point p = bezpatchinterp(patch, u, v);
                patch_points[iu].push_back(p);
                //patch_points[iu][iv] = p;
            }

        }
    }
}

//****************************************************
// Simple init function
//****************************************************
void initScene(){
    // Enable lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    //glEnable(GL_COLOR_MATERIAL); // Enables color to work together with lighting
    //glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GLfloat light_position[] = { 1.0f, -1.0f, -.5f, 0.0f }; 
    GLfloat light_color[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // White light
    GLfloat ambient_color[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Weak white light
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
    //glLightfv(GL_LIGHT0, GL_SPECULAR, light_color);
    //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);



    xVal = 0.0;
    yVal = 0.0;
    xRotVal = 0.0;
    yRotVal = 0.0;
    zoom = 1.0;
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
    //gluOrtho2D(0, viewport.w, 0, viewport.h);
    //glOrtho(-1, 1, -1, 1, 1, -1);    // resize type = stretch
    glOrtho(-3, 3, -3, 3, 3, -3);    // resize type = stretch

}

bool checkKey(unsigned int s) {
    return keyBuffer[s] && !prevKeyBuffer[s];
}

//void handleKeyboardInput() {
//	
//    if (checkKey('w')) {
//        printf("Switching fill mode.\n");
//        prevKeyBuffer['w'] = true;
//        filledPolys = !filledPolys;
//    }
//    if (checkKey('s')) {
//        printf("Switching shading mode.\n");
//        prevKeyBuffer['s'] = true;
//        flatShading = !flatShading;
//    }
//    if (checkKey(GLUT_KEY_UP) && mod == GLUT_ACTIVE_SHIFT) {
//        printf("UP\n");
//        prevKeyBuffer[GLUT_KEY_UP] = true;
//        glTranslatef(0.0, 10.0, 0.0);
//    }
//    if (checkKey(GLUT_KEY_DOWN)) {
//        printf("DOWN\n");
//        prevKeyBuffer[GLUT_KEY_DOWN] = true;
//        glTranslatef(0.0, -10.0, 0.0);
//    }
//    if (checkKey(GLUT_KEY_RIGHT)) {
//        printf("RIGHT\n");
//        prevKeyBuffer[GLUT_KEY_RIGHT] = true;
//        glTranslatef(10.0, 0.0, 0.0);
//    }
//    if (checkKey(GLUT_KEY_LEFT)) {
//        printf("LEFT\n");
//        prevKeyBuffer[GLUT_KEY_LEFT] = true;
//        glTranslatef(-10.0, 0.0, 0.0);
//    }
//}

void drawRectangle(Point bl, Point tl, Point tr, Point br) {
    glBegin(GL_QUADS);                         // draw rectangle 
    //glVertex3f(x val, y val, z val (won't change the point because of the projection type));
    glNormal3f(bl.normal1.x, bl.normal1.y, bl.normal1.z);
    glVertex3f(bl.x, bl.y, bl.z);               // bottom left corner of rectangle
    glNormal3f(tl.normal1.x, tl.normal1.y, tl.normal1.z);
    glVertex3f(tl.x, tl.y, tl.z);               // top left corner of rectangle
    glNormal3f(tr.normal1.x, tr.normal1.y, tr.normal1.z);
    glVertex3f(tr.x, tr.y, tr.z);               // top right corner of rectangle
    glNormal3f(br.normal1.x, br.normal1.y, br.normal1.z);
    glVertex3f(br.x, br.y, br.z);               // bottom right corner of rectangle
    glEnd();

    /*glBegin(GL_QUADS);                         // draw rectangle
    //glVertex3f(x val, y val, z val (won't change the point because of the projection type));
    glNormal3f(bl.normal2.x, bl.normal2.y, bl.normal2.z);
    glVertex3f(bl.x, bl.y, bl.z);               // bottom left corner of rectangle
    glNormal3f(tl.normal2.x, tl.normal2.y, tl.normal2.z);
    glVertex3f(tl.x, tl.y, tl.z);               // top left corner of rectangle
    glNormal3f(tr.normal2.x, tr.normal2.y, tr.normal2.z);
    glVertex3f(tr.x, tr.y, tr.z);               // top right corner of rectangle
    glNormal3f(br.normal2.x, br.normal2.y, br.normal2.z);
    glVertex3f(br.x, br.y, br.z);               // bottom right corner of rectangle
    glEnd();*/

}

void drawTriangle(Point bl, Point tl, Point tr) {
    glBegin(GL_TRIANGLES);                         
    //glVertex3f(x val, y val, z val (won't change the point because of the projection type));
    glNormal3f(bl.normal1.x, bl.normal1.y, bl.normal1.z);
    glVertex3f(bl.x, bl.y, bl.z);               
    glNormal3f(tl.normal1.x, tl.normal1.y, tl.normal1.z);
    glVertex3f(tl.x, tl.y, tl.z);               
    glNormal3f(tr.normal1.x, tr.normal1.y, tr.normal1.z);
    glVertex3f(tr.x, tr.y, tr.z);               
    glEnd(); 

    /*glBegin(GL_TRIANGLES);
    //glVertex3f(x val, y val, z val (won't change the point because of the projection type));
    glNormal3f(bl.normal2.x, bl.normal2.y, bl.normal2.z);
    glVertex3f(bl.x, bl.y, bl.z);
    glNormal3f(tl.normal2.x, tl.normal2.y, tl.normal2.z);
    glVertex3f(tl.x, tl.y, tl.z);
    glNormal3f(tr.normal2.x, tr.normal2.y, tr.normal2.z);
    glVertex3f(tr.x, tr.y, tr.z);
    glEnd();*/
}

void drawSurface(){

    for (Surface s : surface_list) {
        subdividepatch(s, subdivisionSize);
        
        if (!isAdaptive) {
            for (int iu = 0; iu + 1 <= numdiv; iu++) {
                for (int iv = 0; iv + 1 <= numdiv; iv++) {
                    Point ll, lr, ul, ur;
                    ll = patch_points[iu][iv];
                    lr = patch_points[iu][iv + 1];
                    ur = patch_points[iu + 1][iv + 1];
                    ul = patch_points[iu + 1][iv];
                    drawRectangle(ll, ul, ur, lr);
                }

            }
            patch_points.clear();
        }
    }

    if (isAdaptive) {
        for (Triangle t : triangle_list) {
            drawTriangle(t.a, t.b, t.c);

        }
    }

}

void myDisplay() {


    glMatrixMode(GL_MODELVIEW);			        // indicate we are specifying camera transformations

    //handleKeyboardInput();

    filledPolys ? glPolygonMode(GL_FRONT_AND_BACK, GL_FILL) : glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    flatShading ? glShadeModel(GL_FLAT) : glShadeModel(GL_SMOOTH);

    glLoadIdentity();				        // make sure transformation is "zero'd"

    glTranslatef(xVal, yVal, 0.0);
    glRotatef(xRotVal, 1.0, 0.0, 0.0);
    glRotatef(yRotVal, 0.0, 1.0, 0.0);
    glScalef(zoom, zoom, zoom);

    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer
    // Start drawing

    // ...


    GLfloat cyan[] = { 0.f, .8f, .8f, 1.f };
    GLfloat mat_specularColor[] = { 1.f, 1.f, 1.f, 1.0f };
    GLfloat mat_shininess[] = { 120.0 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, cyan);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, cyan);
    //glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specularColor);
    //glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    drawSurface();


    glFlush();
    glutSwapBuffers();					// swap buffers (we earlier set double buffer)
}


void processFile(char* filename) {
    // create a file-reading object
    vector<Curve> curve_list;
    //vector<vector<Point>> surface_in_list;
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


        if (!token[0]){
            continue;
        }

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


        // process the tokens
        if (!token[1]) { //pls
            numberOfPatches = atoi(token[0]);
        }
        else {
            //vector<Point> curve_in_list;
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

            /*curve_in_list.push_back(a);
            curve_in_list.push_back(b);
            curve_in_list.push_back(c);
            curve_in_list.push_back(d);*/

            Curve curve(a, b, c, d);
            curve_list.push_back(curve);
            //surface_in_list.push_back(curve_in_list);

            if (curve_list.size() == 4) {
                Surface s(curve_list[0], curve_list[1], curve_list[2], curve_list[3]);
                //s.points = surface_in_list;

                curve_list.clear(); //TODO hopefully this works
                //surface_in_list.clear();

                surface_list.push_back(s);
            }
        }
    }
}

void processArgs(int argc, char *argv[]) {
    filename = string(argv[1]);
    char* temp = argv[1];
    subdivisionSize = strtof(argv[2], &temp);
    processFile(argv[1]);
    if (argv[3]) {
        string ad(argv[3]);
        if ( ad == "-a"){
            isAdaptive = true;
        }
    }
}

void toggleShading() {
    printf("Switch shading mode\n");
    flatShading = !flatShading;
    //prevKeyBuffer[key] = true;
    //keyBuffer[key] = false;
}

void toggleFill() {
    filledPolys = !filledPolys;
    printf("Switching fill mode.\n");
}
void key(unsigned char key, int x, int y) {
    //prevKeyBuffer[key] = false;
    //keyBuffer[key] = true;
    switch (key) {
    case ' ':
        exit(0);
        break;
    case 'w':
        toggleFill();
        break;
    case 's':
        toggleShading();
        break;
    case '+':
        zoom += 0.2;
        break;
    case '-':
        zoom -= 0.2;
        break;
    }
}

void keyUp(unsigned char key, int x, int y) {
    //prevKeyBuffer[key] = true;
    //keyBuffer[key] = false;
}

void specKey(int key, int x, int y) {
    int mod = glutGetModifiers();
    switch (key) {

    case GLUT_KEY_UP:
        if (mod == GLUT_ACTIVE_SHIFT) {
            printf("SHIFT + UP\n");
            prevKeyBuffer[GLUT_KEY_UP] = true;
            yVal += 1;

        }
        else {
            printf("Rotate up\n");
            xRotVal += 22.5;
        }
        break;
    case GLUT_KEY_DOWN:
        if (mod == GLUT_ACTIVE_SHIFT) {
            printf("SHIFT + DOWN\n");
            prevKeyBuffer[GLUT_KEY_DOWN] = true;
            yVal -= 1;
            //glTranslatef(xVal, yVal, 0.0);
        }
        else {
            //Rotate
            printf("Rotate down\n");
            xRotVal -= 22.5;
        }
        break;
    case GLUT_KEY_RIGHT:
        if (mod == GLUT_ACTIVE_SHIFT) {
            printf("SHIFT + RIGHT\n");
            prevKeyBuffer[GLUT_KEY_RIGHT] = true;
            xVal += 1;
            //glTranslatef(xVal, yVal, 0.0);
        }
        else {
            //Rotate
            printf("Rotate right\n");
            yRotVal += 45.0;
        }
        break;
    case GLUT_KEY_LEFT:
        if (mod == GLUT_ACTIVE_SHIFT) {
            printf("SHIFT + LEFT\n");
            prevKeyBuffer[GLUT_KEY_LEFT] = true;
            xVal -= 1;
            //glTranslatef(xVal, yVal, 0.0);
        }
        else {
            //Rotate
            printf("Rotate left\n");
            yRotVal -= 45.0;
        }
        break;
    }
    //prevKeyBuffer[key] = false;
    //keyBuffer[key] = true;
}

void specKeyUp(int key, int x, int y) {
    //prevKeyBuffer[key] = true;
    //keyBuffer[key] = false;
}


void idle() {
    //nothing here for now
#ifdef _WIN32
    Sleep(10);                                   //give ~10ms back to OS (so as not to waste the CPU)
#endif
    glutPostRedisplay(); // forces glut to call the display function (myDisplay())
}

//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {
    processArgs(argc, argv);


    flatShading = true;
    filledPolys = true;
    //This initializes glut
    glutInit(&argc, argv);

    //This tells glut to use a double-buffered window with red, green, and blue channels 
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_ALPHA);

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
    glutKeyboardFunc(key);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specKey);
    glutSpecialUpFunc(specKeyUp);

    glutIdleFunc(idle);
    glutMainLoop();							// infinite loop that will keep drawing and resizing
    // and whatever else


    return 0;
}








