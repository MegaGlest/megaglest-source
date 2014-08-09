#ifndef GLWIDGET_H
#define GLWIDGET_H
//~ //~
#include <GL/glew.h>
#define QT_NO_OPENGL_ES_2
#include "renderer.h"
#include <QGLWidget>
#include <vector>
#include <QColor>
//includes openGL, fucks GLEW
//~ //~
//#include <QGLBuffer>
//#include <QGLShaderProgram>
//~ //~
namespace Shared{ namespace G3dViewer{
class GLWidget : public QGLWidget{
    Q_OBJECT
public:
    GLWidget( const QGLFormat& format, QWidget* parent = 0 );


    void loadModel(QString path);
    void setBackgroundColor(const QColor &col);
    void screenshot(QString path, bool transparent);
//~
protected:
    virtual void initializeGL();
    //virtual void resizeGL( int w, int h );
    virtual void paintGL();
//~ //~
    //virtual void keyPressEvent( QKeyEvent* e );
    virtual void wheelEvent ( QWheelEvent* e);
//~
private:
    Renderer* renderer;
    int rotX , rotY;
    float zoom;
    vector<std::string> modelPathList;
    Model *model;
    //bool prepareShaderProgram( const QString& vertexShaderPath,
    //                           const QString& fragmentShaderPath );
//~ //~
    //QGLShaderProgram m_shader;
    //QGLBuffer m_vertexBuffer;
};
//~ //~
}}
#endif // GLWIDGET_H
