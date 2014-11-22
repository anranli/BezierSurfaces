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

class Point;

class Vector {
public:
    float x, y, z;
    Vector();
    Vector(float a, float b, float c);
    Vector(Point a, Point b);
    void normalize();
    Vector scalarMult(float s);


    /*Vector add(Vector);
    Vector sub(Vector);
    Vector mul(float);
    Vector div(float);
    Vector normalize();*/
};

class Point {
public:
    float x, y, z;
    Vector derivative, normal1, normal2;
    Point();
    Point(float a, float b, float c);
    Point scalarMult(float s);
    Point add(Point p);
    Point midpoint(Point p);
    float distance(Point p);
    //Point sub(Vector);
};

class Curve {
public:
    Point a, b, c, d;
    Curve();
    Curve(Point a1, Point b1, Point c1, Point d1);
};

class Surface {
public:
    Curve a, b, c, d;
    //vector<vector<Point>> points;
    Surface();
    Surface(Curve a1, Curve b1, Curve c1, Curve d1);
};

class Triangle {
public:
    Point a, b, c;
    float au, av, bu, bv, cu, cv;
    Triangle();
    Triangle(Point a1, Point b1, Point c1);
    
};