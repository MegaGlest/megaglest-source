#ifndef GLWIDGET_H
#define GLWIDGET_H
//~ //~
#include <GL/glew.h>
#define QT_NO_OPENGL_ES_2
#include "renderer.h"

//includes openGL, fucks GLEW
#if QT_VERSION >= 0x050400
#include <QOpenGLWidget>
#else
#include <QGLWidget>
#endif

#include <vector>
#include <QColor>

namespace Shared{ namespace G3dViewer{

/*class GLFormat ://Qt5.4 replaces QGLFormat
#if QT_VERSION >= 0x050400
public QSurfaceFormat {
#else
public QGLFormat {
#endif

};*/

class GLWidget ://Qt5.4 replaces QGLWidget
#if QT_VERSION >= 0x050400
public QOpenGLWidget {
#else
public QGLWidget {
#endif
    Q_OBJECT
public:
    GLWidget( /*const GLFormat& format,*/ QWidget* parent = 0 );


    void loadModel(QString path);
    void loadParticle(QString path);
    void loadSplashParticle(QString path);
    void loadProjectileParticle(QString path);
    void setBackgroundColor(const QColor &col);
    void screenshot(QString path, bool transparent);
    void setPlayerColor(const QColor &col);
    QColor getPlayerColor() {return playerColor;};
public slots:
    void toggleNormals();
    void toggleWireframe();
    void toggleGrid();
    /**
     * Forget all loaded models and particles
     */
    void clearAll();

protected:
    /**
     * Only Qt should calls this!
     */
    virtual void initializeGL();
    /**
     * resizes the viewport and fits the scene
     * Only Qt should calls this!
     */
    virtual void resizeGL(int w, int h);
    /**
     * Only Qt should calls this!
     * Please use updateGL() for triggering this
     */
    virtual void paintGL();
    /**
     * Only Qt should calls this!
     * Event for scrolling
     */
    virtual void wheelEvent(QWheelEvent* e);
    /**
     * Only Qt should calls this!
     * Event for scrolling
     */
    virtual void mousePressEvent(QMouseEvent* e);
    /**
     * Only Qt should calls this!
     * Event for scrolling
     */
    virtual void mouseMoveEvent(QMouseEvent* e);

#if QT_VERSION >= 0x050400
    /**
     * Qt 5.4 does not have this, calls update() instead
     */
    void updateGL();
#endif
//~
private:
    Renderer* renderer;
    float rotX , rotY, oldRotX, oldRotY;
    QPoint oldPos;
    float zoom;
    QColor playerColor;
    std::vector<std::string> modelPathList, particlePathList;
    std::vector<UnitParticleSystem*> unitParticleSystems;
    std::vector<UnitParticleSystemType*> unitParticleSystemTypes;
    Model *model;
};
}}
#endif // GLWIDGET_H
