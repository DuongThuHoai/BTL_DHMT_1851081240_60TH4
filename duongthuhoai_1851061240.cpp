
#ifdef __APPLE_CC__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <cmath>
#include <GL/GLAUX.H>
#include <iostream>
#define PI 3.1415926535898
#define STB_IMAGE_IMPLEMENTATION
#include "Header.h"
using namespace std;

float de_tamX = 4.5;
float de_tamY = 0.2;
float de_tamZ = 4.0;

float dist_esf_base = 1.0;
GLUquadricObj* quadratic;
unsigned int _textureId;
GLfloat WHITE[] = { 1, 1.5, 1 };//màu sắc bóng
GLfloat RED[] = { 1, 0, 0 };
GLfloat GREEN[] = { 0, 1, 1 };
GLfloat MAGENTA[] = { 1, 0, 1 };
GLfloat YELLOW[] = { 1, 0.5, 0.5 };
GLfloat BLACK[] = { 0.0, 0.0, 0.0 };
GLfloat GREY[] = { 0.4, 0.3, 0.3 };
int refresh = 60;
float spin = 0.0;
float b = 1;
double p;
double incrs;


void DrawCircle(float x, float z, float r) {
    int i;
    int triangleAmount = 50; //# of triangles used to draw circle

    //GLfloat radius = 0.8f; //radius
    /*GLfloat twicePi = 2.0f * PI;*/

    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x, 0, z); // center of circle
    for (i = 0; i <= triangleAmount; i++) {
        float t = (2 * PI * i) / triangleAmount;
        glVertex3f(
            x + (r * sin(t)), 0.01,
            z + (r * cos(t))
        );
    }
    glEnd();
}


//////****Start*****//////
void loadTexture(const char* filename, unsigned int& texName) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texName);           // Bắt đầu quá trình gen texture.
    glBindTexture(GL_TEXTURE_2D, texName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    int width = 50, height = 50, channels = 0;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    stbi_set_flip_vertically_on_load(true);

    if (image != NULL) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        stbi_image_free(image);
    }
    else
        std::cout << "Failure to load texture";
}

void initt() {

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, WHITE);
    glLightfv(GL_LIGHT0, GL_SPECULAR, WHITE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, WHITE);
    glMaterialf(GL_FRONT, GL_SHININESS, 50);
}
void initRendering() {


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);
    quadratic = gluNewQuadric();
    gluQuadricNormals(quadratic, GLU_SMOOTH);
    gluQuadricTexture(quadratic, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, _textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    loadTexture("pink.bmp", _textureId);
}


// Máy ảnh, máy quay. Nó di chuyển theo chiều ngang trong một vòng tròn có tâm tại điểm gốc của bán kính r(bán kính càng lớn góc nhìn càng lớn).
class Camera {
    double theta;       // xác định vị trí x và z
    double y;          // vị trí y hiện tại
    double dTheta;     //  dtheta để xoay máy ảnh xung quanh(càng nhỏ góc di chuyển càng cứng và nhanh)
    double dy;         //  y để di chuyển máy ảnh lên / xuống (càng lớn góc di chuyển càng cứng và nhanh)
    double rd;         //radius_camera_inc
    double  r;          //radius_camera
public:
    Camera() : theta(0), y(3), dTheta(0.05), dy(0.3), rd(0.2), r(12.7) {}
    double getX() { return r * cos(theta); }
    double getY() { return y; }
    double getZ() { return r * sin(theta); }
    void moveRight() { theta += dTheta; }
    void moveLeft() { theta -= dTheta; }
    void moveUp() { y += dy; }
    void moveDown() { if (y > dy) y -= dy; }
    void zoomin() { r += rd; }
    void zoomout() { r -= rd; }

};

//  Một quả bóng có bán kính, màu sắc và nảy lên xuống giữa độ cao lớn nhất và mặt phẳng xz. 
// Do đó tọa độ x và z của nó được sửa. 
// Nó sử dụng một thuật toán nảy, chỉ cần di chuyển lên hoặc giảm đơn vị tại mỗi khung hình.
class Ballg {
    double radius;
    GLfloat* color;
    double maximumHeight;
    double maximumWidth;
    double maximumDepth;
    double x;
    double y;
    double z;
    int directionx;
    int directiony;
    int directionz;
public:
    Ballg(double r, GLfloat* c, double h, double w, double d, double dr) :
        radius(r), color(c), maximumHeight(h), maximumWidth(w), maximumDepth(d), directionx(dr), directiony(dr), directionz(dr),
        y(h), x(w), z(d) {
    }
    void update() {
        x += directionx * 0.1;//hướng tốc độ di chuyển của bóng khi va chạm tới bề rộng tối đa
        if (x > maximumWidth) {
            x = maximumWidth; directionx = -1;
        }
        else if (x < radius) {
            x = radius; directionx = 1;
        }
        y += directiony * 0.4;//hướng tốc độ di chuyển của bóng khi va chạm tới chiều cao tối đa
        if (y > maximumHeight) {
            y = maximumHeight; directiony = -1;
        }
        else if (y < radius) {
            y = radius; directiony = 1;
        }

        z += directionz * 0.2;//hướng tốc độ di chuyển của bóng khi va chạm tới chiều sâu tối đa
        if (z > maximumDepth) {
            z = maximumDepth; directionz = -1;
        }
        else if (z < radius) {
            z = radius; directionz = 1;
        }

        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
       /* glDisable(GL_NORMALIZE);*/
        glDisable(GL_TEXTURE_2D);
        glShadeModel(GL_SMOOTH);
        glDisable(GL_COLOR_MATERIAL);
        glPushMatrix();
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
        glTranslated(x, y, z);//di chuyển quả bóng theo hướng tốc độ x,y,z
        glRotated(-spin, x, y, z);
        glutSolidSphere(radius, 30, 30);
        glEnable(GL_TEXTURE_2D);
        glPopMatrix();

        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        /*glDisable(GL_NORMALIZE);*/
        glDisable(GL_TEXTURE_2D);
        glShadeModel(GL_SMOOTH);
        glDisable(GL_COLOR_MATERIAL);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, GREY);
        glTranslatef(0.4f, 0.0f, 0.0f);
        glTranslatef(x, 0, z);
        /*glRotated(-spin, 0, 0, 1);*/

        DrawCircle(0.0, 0.0, 0.8);
        glEnable(GL_TEXTURE_2D);
        glPopMatrix();

        //tốc độ xoay
        b = b + 0.05;//b càng lớn xoay càng nhanh
        spin = spin + 0.5;//spin càng lớn xoay càng nhanh
        if (spin > 360)
        {
            spin = spin - 360;
            b = 1;
        }


    }
};
class Ballp {
    double radius;
    GLfloat* color;
    double maximumHeight;
    double maximumWidth;
    double maximumDepth;
    double x;
    double y;
    double z;
    int directionx;
    int directiony;
    int directionz;
public:
    Ballp(double r, GLfloat* c, double h, double w, double d, double dr) :
        radius(r), color(c), maximumHeight(h), maximumWidth(w), maximumDepth(d), directionx(dr), directiony(dr), directionz(dr),
        y(h), x(w), z(d) {
    }
    void update() {
        x += directionx * 0.2;//hướng tốc độ di chuyển của bóng khi va chạm tới bề rộng tối đa
        if (x > maximumWidth) {
            x = maximumWidth; directionx = -1;
        }
        else if (x < radius) {
            x = radius; directionx = 1;
        }
        y += directiony * 0.5;//hướng tốc độ di chuyển của bóng khi va chạm tới chiều cao tối đa
        if (y > maximumHeight) {
            y = maximumHeight; directiony = -1;
        }
        else if (y < radius) {
            y = radius; directiony = 1;
        }

        z += directionz * 0.2;//hướng tốc độ di chuyển của bóng khi va chạm tới chiều sâu tối đa
        if (z > maximumDepth) {
            z = maximumDepth; directionz = -1;
        }
        else if (z < radius) {
            z = radius; directionz = 1;
        }
        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        /* glDisable(GL_NORMALIZE);*/
        glDisable(GL_TEXTURE_2D);
        glShadeModel(GL_SMOOTH);
        glDisable(GL_COLOR_MATERIAL);
        glPushMatrix();
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
        glTranslated(x, y, z);//di chuyển quả bóng theo hướng tốc độ x,y,z
        glRotated(-spin, x, y, z);
        glutSolidSphere(radius, 30, 30);
        glEnable(GL_TEXTURE_2D);
        glPopMatrix();

        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        /*glDisable(GL_NORMALIZE);*/
        glDisable(GL_TEXTURE_2D);
        glShadeModel(GL_SMOOTH);
        glDisable(GL_COLOR_MATERIAL);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, GREY);
        glTranslatef(0.4f, 0.0f, 0.0f);
        glTranslatef(x, 0, z);
        /*glRotated(-spin, 0, 0, 1);*/

        DrawCircle(0.0, 0.0, 0.5);
        glEnable(GL_TEXTURE_2D);
        glPopMatrix();

        if (spin > 360)
        {
            spin = spin - 360;
            b = 1;
        }
       
    }
};
class Ballr {
    double radius;
    GLfloat* color;
    double maximumHeight;
    double maximumWidth;
    double maximumDepth;
    double x;
    double y;
    double z;
    int directionx;
    int directiony;
    int directionz;
public:
    Ballr(double r, GLfloat* c, double h, double w, double d, double dr) :
        radius(r), color(c), maximumHeight(h), maximumWidth(w), maximumDepth(d), directionx(dr), directiony(dr), directionz(dr),
        y(h), x(w), z(d) {
    }
    void update() {
        x += directionx * 0.3;//hướng tốc độ di chuyển của bóng khi va chạm tới bề rộng tối đa
        if (x > maximumWidth) {
            x = maximumWidth; directionx = -1;
        }
        else if (x < radius) {
            x = radius; directionx = 1;
        }
        y += directiony * 0.4;//hướng tốc độ di chuyển của bóng khi va chạm tới chiều cao tối đa
        if (y > maximumHeight) {
            y = maximumHeight; directiony = -1;
        }
        else if (y < radius) {
            y = radius; directiony = 1;
        }

        z += directionz * 0.5;//hướng tốc độ di chuyển của bóng khi va chạm tới chiều sâu tối đa
        if (z > maximumDepth) {
            z = maximumDepth; directionz = -1;
        }
        else if (z < radius) {
            z = radius; directionz = 1;
        }
        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        /* glDisable(GL_NORMALIZE);*/
        glDisable(GL_TEXTURE_2D);
        glShadeModel(GL_SMOOTH);
        glDisable(GL_COLOR_MATERIAL);
        glPushMatrix();
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
        glTranslated(x, y, z);//di chuyển quả bóng theo hướng tốc độ x,y,z
        glRotated(-spin, x, y, z);
        glutSolidSphere(radius, 30, 30);
        glEnable(GL_TEXTURE_2D);
        glPopMatrix();

        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        /*glDisable(GL_NORMALIZE);*/
        glDisable(GL_TEXTURE_2D);
        glShadeModel(GL_SMOOTH);
        glDisable(GL_COLOR_MATERIAL);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, GREY);
        glTranslatef(0.4f, 0.0f, 0.0f);
        glTranslatef(x, 0, z);
        /*glRotated(-spin, 0, 0, 1);*/

        DrawCircle(0.0, 0.0, 0.4);
        glEnable(GL_TEXTURE_2D);
        glPopMatrix();

        // tốc độ xoay
        b = b + 0.000009;//b càng nhỏ xoay càng chậm
        spin = spin + 0.0009;//spin càng lớn xoay càng chậm
        if (spin > 360)
        {
            spin = spin - 360;
            b = 1;
        }
      
    }
};

//miếng đỡ ở dưới để dễ hình dung các mặt Ox,Oy,Oz
class Yard {
    int displayListId;
    int width;
    int depth;
public:
    Yard(int width, int depth) : width(width), depth(depth) {}
    double centerx() { return width / 2; }
    double centerz() { return depth / 2; }
    void create() {
        /*glEnable(GL_BLEND);*/
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        displayListId = glGenLists(1);
        glNewList(displayListId, GL_COMPILE);
        GLfloat lightPosition[] = { 5, 6, 7, 2 };// vị trí chiếu sáng
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
        glBegin(GL_QUADS);
        glNormal3d(0, 1, 0);
        for (int x = 0; x < width - 1; x++) {
            for (int z = 0; z < depth - 1; z++) {
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE,
                    (x + z) % 6 == 0 ? YELLOW : GREEN);//vị trí các ô màu trên miếng đỡ
                glVertex3d(x, 0, z);
                glVertex3d(x + 1, 0, z);
                glVertex3d(x + 1, 0, z + 1);
                glVertex3d(x, 0, z + 1);
            }
        }
       /* glDisable(GL_BLEND);*/
        glEnable(GL_TEXTURE_2D);
        glEnd();
        glEndList();
    }
    void draw() {
        glCallList(displayListId);
    }
};

class wall {
    int displayListId;
    int width;
    int height;
public:
    wall(int width, int hgh) : width(width), height(hgh) {}// nhìn theo hai trục tọa độ Ox, Oz
    double centerx() { return width / 2; }
    double centery() { return height / 2; }
    void create() {
       /* glEnable(GL_BLEND);*/
        displayListId = glGenLists(1);
        glNewList(displayListId, GL_COMPILE);
        GLfloat lightPosition[] = { 4, 3, 7, 1 };// vị trí chiếu sáng
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
        glBegin(GL_QUADS);
        glNormal3d(0, 0, 1);
        for (int x = 0; x < width - 1; x++) {
            for (int y = 0; y < height - 1; y++) {
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE,
                    (x + y) % 6 == 0 ? WHITE : MAGENTA);//vị trí các ô màu trên miếng đỡ
                glVertex3d(x, y, 0);
                glVertex3d(x + 1, y, 0);
                glVertex3d(x + 1, y + 1, 0);
                glVertex3d(x, y + 1, 0);
            }
        }
      /*  glDisable(GL_BLEND);*/
        glEnd();
        
        glEndList();
    }
    void draw() {
        glCallList(displayListId);
    }
};
void drawBase() {
    glPushMatrix();
    glTranslatef(4.5f, (10.0 / 2 + dist_esf_base + de_tamY ), 4.0f);
    initRendering();

    glBegin(GL_QUADS);
    //Mặt trên
    glNormal3f(0.0, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-de_tamX , de_tamY , de_tamZ ); //Bottom Left of Texture & Plane
    glTexCoord2f(1.0f, 0.0f); glVertex3f(de_tamX , de_tamY , de_tamZ ); // Bottom Right of Texture & Plane
    glTexCoord2f(1.0f, 1.0f); glVertex3f(de_tamX , de_tamY , -de_tamZ ); // Top Right of Texture & Plane
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-de_tamX , de_tamY , -de_tamZ ); // Top Left of Texture & Plane
    //Mặt dưới
    glNormal3f(0.0, -1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-de_tamX , -de_tamY , de_tamZ );
    glTexCoord2f(1.0f, 0.0f); glVertex3f(de_tamX , -de_tamY, de_tamZ);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(de_tamX , -de_tamY , -de_tamZ );
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-de_tamX , -de_tamY, -de_tamZ );
    //Mặt trái
    glNormal3f(-1.0, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-de_tamX , -de_tamY , -de_tamZ );
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-de_tamX , -de_tamY , de_tamZ );
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-de_tamX , de_tamY , de_tamZ );
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-de_tamX , de_tamY, -de_tamZ );
    //Mặt phải**
    glNormal3f(1.0, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(de_tamX , -de_tamY , -de_tamZ );
    glTexCoord2f(1.0f, 0.0f); glVertex3f(de_tamX , -de_tamY , de_tamZ );
    glTexCoord2f(1.0f, 1.0f); glVertex3f(de_tamX, de_tamY, de_tamZ );
    glTexCoord2f(0.0f, 1.0f); glVertex3f(de_tamX , de_tamY , -de_tamZ );
    //mặt trước
    glNormal3f(0.0, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-de_tamX , -de_tamY , de_tamZ );
    glTexCoord2f(1.0f, 0.0f); glVertex3f(de_tamX , -de_tamY , de_tamZ  );
    glTexCoord2f(1.0f, 1.0f); glVertex3f(de_tamX , de_tamY , de_tamZ );
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-de_tamX , de_tamY , de_tamZ );
    //mặt sau**
    glNormal3f(0.0, 0.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-de_tamX , -de_tamY , -de_tamZ );
    glTexCoord2f(1.0f, 0.0f); glVertex3f(de_tamX , -de_tamY , -de_tamZ );
    glTexCoord2f(1.0f, 1.0f); glVertex3f(de_tamX, de_tamY , -de_tamZ );
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-de_tamX , de_tamY , -de_tamZ );
    glEnd();
    //initRendering();
    glPopMatrix();
}
// Các biến toàn cục:xoay góc nhìn, miếng đỡ và một số quả bóng.
Yard yrd(10, 10);
wall wl(10, 7);
Camera camera;
Ballg balls[] = {
  Ballg(0.7, GREEN, 5, 6, 6, 9)//(bán kính bóng, màu sắc, height, width, depth, direction)
};
Ballp ballss[] = {

  Ballp(0.5, MAGENTA, 5, 6, 6, 9)//(bán kính bóng, màu sắc, height, width, depth, direction)

};
Ballr ballsss[] = {

  Ballr(0.4, RED, 5, 6, 6, 15)//(bán kính bóng, màu sắc, height, width, depth, direction)
};



void init() {
    glEnable(GL_DEPTH_TEST);
    initRendering();
    initt();
    yrd.create();
    wl.create();
   
}



void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_2D);
    glLoadIdentity();
    glTranslatef(0.0f, 1.0f, -16.0f);
    gluLookAt(camera.getX(), camera.getY(), camera.getZ(),
        yrd.centerx(), 0.0, yrd.centerz(),
        0.0, 1.0, 0.0);
    yrd.draw();
    wl.draw();
    drawBase();
    
    for (int i = 0; i < sizeof balls / sizeof(Ballg); i++)
    {

        balls[i].update();
    }
    // Vòng lặp kiểm tra cập nhật vị trí sau khi di chuyển và xoay bóng
    for (int j = 0; j < sizeof ballss / sizeof(Ballp); j++)
    {
        ballss[j].update();
    }
    for (int k = 0; k < sizeof ballsss / sizeof(Ballr); k++)
    {
        ballsss[k].update();
    }
  
    glEnable(GL_TEXTURE_2D);
    glFlush();
    glutSwapBuffers();
}


void reshape(GLint w, GLint h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, GLfloat(w) / GLfloat(h), 1.0, 150.0);
    glMatrixMode(GL_MODELVIEW);
}


void timer(int v) {
    glutPostRedisplay();
    glutTimerFunc(refresh, timer, v);
}
// các phím nhấn để di chuyển góc nhìn và phóng to thu nhỏ
void special(int key, int, int) {
    switch (key) {
    case GLUT_KEY_LEFT: camera.moveLeft(); break;
    case GLUT_KEY_RIGHT: camera.moveRight(); break;
    case GLUT_KEY_UP: camera.moveUp(); break;
    case GLUT_KEY_DOWN: camera.moveDown(); break;
    case GLUT_KEY_PAGE_UP: camera.zoomin(); break;
    case GLUT_KEY_PAGE_DOWN: camera.zoomout(); break;
    case GLUT_KEY_F10: p *= 0; break;//Pause
    case GLUT_KEY_F9: incrs *= 1; break;//Tăng tốc độ khi giữ nhấn
    }
    glutPostRedisplay();
}

//ESC sự kiện nhấn để thoát chương trình
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 27:     // ESC để thoát
        exit(0);
        break;
    }
}


// Initializes GLUT and enters the main loop.
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(80, 80);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Duong Thu Hoai - 1851061240 - Bouncyball");
    //initRendering();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(special);
    glutTimerFunc(100, timer, 0);
    glutKeyboardFunc(keyboard);
    init();
    glutMainLoop();
}
