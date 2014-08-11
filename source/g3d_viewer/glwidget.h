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
class GLWidget ://Qt5.4 replaces QGLWidget
#if QT_VERSION >= 0x050400
public QOpenGLWidget {
#else
public QGLWidget {
#endif
    Q_OBJECT
public:
    GLWidget( const QGLFormat& format, QWidget* parent = 0 );


    void loadModel(QString path);
    void loadParticle(QString path);
    void loadSplashParticle(QString path);
    void loadProjectileParticle(QString path);
    void setBackgroundColor(const QColor &col);
    void screenshot(QString path, bool transparent);
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
    //virtual void resizeGL( int w, int h );//TODO: don’t change size in paintGL
    /**
     * Only Qt should calls this!
     * Please use updateGL() for triggering this
     */
    virtual void paintGL();
    //virtual void keyPressEvent( QKeyEvent* e );
    /**
     * Only Qt should calls this!
     * Event for scrolling
     */
    virtual void wheelEvent ( QWheelEvent* e);
//~
private:
    Renderer* renderer;
    int rotX , rotY;
    float zoom;
    QColor playerColor;
    std::vector<std::string> modelPathList, particlePathList;
    std::vector<UnitParticleSystem*> unitParticleSystems;
    std::vector<UnitParticleSystemType*> unitParticleSystemTypes;
    Model *model;
};
}}
#endif // GLWIDGET_H
