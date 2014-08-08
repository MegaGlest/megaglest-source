#include "glwidget.h"
#include <iostream>
#include <QFile>
//~ //~
//~ //~
namespace Shared{ namespace G3dViewer{

GLWidget::GLWidget( const QGLFormat& format, QWidget* parent):QGLWidget( format, parent ),renderer(Renderer::getInstance()),rotX(0),rotY(0),zoom(1){

}

void GLWidget::loadModel(QString path){
    try {
        if(path != "" && QFile(path).exists()) {
            this->modelPathList.push_back(path.toStdString());
            std::cout << "Adding model " << path.toStdString() << " list size " << this->modelPathList.size() << std::endl;
        }

        //~ string titlestring=winHeader;
        //~ for(unsigned int idx =0; idx < this->modelPathList.size(); idx++) {
            //~ string modelPath = this->modelPathList[idx];
//~
            //~ //printf("Loading model [%s] %u of " MG_SIZE_T_SPECIFIER "\n",modelPath.c_str(),idx, this->modelPathList.size());
//~
            //~ if(timer) timer->Stop();
            //~ delete model;
            //~ model = renderer? renderer->newModel(rsGlobal, modelPath): NULL;
//~
            //~ statusbarText = getModelInfo();
            //~ string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
            //~ GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));
            //~ if(timer) timer->Start(100);
            //~ titlestring =  extractFileFromDirectoryPath(modelPath) + " - "+ titlestring;
        //~ }
        //~ SetTitle(ToUnicode(titlestring));*
    }
    catch(std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        //wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
    }
}

void GLWidget::initializeGL(){
    GLuint err = glewInit();//init GLEW, renderer needs this
    if (GLEW_OK != err) {
        fprintf(stderr, "Error [main]: glewInit failed: %s\n", glewGetErrorString(err));
        //return 1;
        throw std::runtime_error((char *)glewGetErrorString(err));
    }
    renderer->init();
}
void GLWidget::resizeGL( int w, int h ){
}
void GLWidget::paintGL(){
    std::cout << "pain t!" << std::endl;
    int viewportW = width(), viewportH = height();

    if(renderer->getGrid() != true) {
        renderer->toggleGrid();
    }

    renderer->reset(viewportW, viewportH, Renderer::pcRed);
    renderer->transform(rotX, rotY, zoom);
    renderer->renderGrid();
    //context()->swapBuffers();
}

}}
