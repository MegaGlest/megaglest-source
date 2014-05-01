#include "mainWindow.hpp"
#include <QMainWindow>
#include <QApplication>
#include <iostream>
//otherwise linking error in Visual Studio
#include <SDL.h>

//initialize and open the window
int main(int argc, char *argv[]){
    //string name = "MegaGlest Editor";
    std::string version = "1.7.0";

    QApplication a(argc, argv);
    QStringList args = a.arguments();//all arguments stripped of those Qt uses itself

    if(args.contains("--help") || args.contains("-h") || args.contains("-H") || args.contains("-?") || args.contains("?") || args.contains("-help") ){
        std::cout << "MegaGlest map editor v" << version << std::endl << std::endl;
        std::cout << "glest_map_editor [GBM OR MGM FILE]" << std::endl << std::endl;
        std::cout << "Creates or edits glest/megaglest maps." << std::endl;
        std::cout << "Draw with left mouse button (select what and how large area in menu or toolbar)" << std::endl;
        std::cout << "Pan trough the map with right mouse button" << std::endl;
        std::cout << "Zoom with middle mouse button or mousewheel" << std::endl;
        std::cout << std::endl;
        exit (0);
    }else if(args.contains("--version")){
        std::cout << version << std::endl;
        std::cout << std::endl;
        exit (0);
    }else{
        MapEditor::MainWindow w;
        w.show();
        return a.exec();
    }


}
