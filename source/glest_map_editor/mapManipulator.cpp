// ==============================================================
//  This file is part of MegaGlest (www.megaglest.org)
//
//  Copyright (C) 2014 Sebastian Riedel
//
//  You can redistribute this code and/or modify it under
//  the terms of the GNU General Public License as published
//  by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version
// ==============================================================
#include "mapManipulator.hpp"
#include "mainWindow.hpp"
#include "renderer.hpp"
#include "selection.hpp"
#include "map_preview.h"
#include "tile.hpp"
#include <QAction>
#include <QGraphicsScene>
#include <QString>
#include <iostream>


namespace MapEditor{
    MapManipulator::MapManipulator(MainWindow *win){
        this->win = win;
        this->radius = 1;
    }

    MapManipulator::~MapManipulator(){
        delete this->win;
        delete this->renderer;
    }

    void MapManipulator::setRenderer( Renderer *renderer){
        this->renderer = renderer;
        this->selectionStartColumn = 0;
        this->selectionStartRow = 0;
        //std::cout << this->renderer->getMap() << std::endl;
        this->selectionEndColumn = this->renderer->getHeight()-1;
        this->selectionEndRow = this->renderer->getWidth()-1;

        this->selectionsquare = true;//selectionfield is a square
        //height and width of selectionfield is a power of two
    }

    MainWindow *MapManipulator::getWindow(){
        return this->win;
    }

    void MapManipulator::setRadius(QAction *radius){
        this->radius = radius->objectName().right(1).toInt();
    }

    void MapManipulator::changeTile(int column, int row){
        QString penName = this->win->getPen()->objectName();
        if(!penName.compare("actionGrass")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Grass, this->radius);
        }else if(!penName.compare("actionSecondary_Grass")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Secondary_Grass, this->radius);
        }else if(!penName.compare("actionRoad")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Road, this->radius);
        }else if(!penName.compare("actionStone")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Stone, this->radius);
        }else if(!penName.compare("actionGround")){
            this->renderer->getMap()->changeSurface(column, row, Shared::Map::st_Ground, this->radius);
        }else if(!penName.compare("actionGold_unwalkable")){
            this->renderer->getMap()->changeResource(column, row, Shared::Map::st_Ground, this->radius);
        }else if(!penName.compare("actionGold_unwalkable")){
            this->renderer->getMap()->changeResource(column, row, Shared::Map::st_Ground, this->radius);
        }else if(!penName.compare("actionGold_unwalkable")){
            this->renderer->getMap()->changeResource(column, row, Shared::Map::st_Ground, this->radius);
        }else if(penName.contains("actionHeight")){
            int height = penName.right(1).toInt();
            if(penName.contains("actionHeight_")){//_ means -
                height = 0-height;
            }
            this->renderer->getMap()->glestChangeHeight(column, row, height, this->radius);
            this->renderer->updateMap();
        }else if(penName.contains("actionGradient")){
            int height = penName.right(1).toInt();
            if(penName.contains("actionGradient_")){//_ means -
                height = 0-height;
            }
            this->renderer->getMap()->pirateChangeHeight(column, row, height, this->radius);
            this->renderer->updateMap();
        }else if(penName.contains("actionResource")){
            int resource = penName.right(1).toInt();
            this->renderer->getMap()->changeResource(column, row, resource, this->radius);
        }else if(penName.contains("actionObject")){
            int object = penName.right(1).toInt(0,16);//hexadecimal A=10 etc.
            this->renderer->getMap()->changeObject(column, row, object, this->radius);
        }else if(penName.contains("actionPlayer")){
            int player = penName.right(1).toInt()-1;
            //std::cout << "move player" << player << std::endl;
            this->renderer->getMap()->changeStartLocation(column, row, player);
            this->renderer->updatePlayerPositions();
        }//else nothing to do

        //this->renderer->recalculateAll();
    }

    void MapManipulator::click(int column, int row){
        this->renderer->getMap()->setRefAlt(column, row);
        this->changeTile(column, row);
    }

    void MapManipulator::reset(int width, int height, int surface, float altitude, int players){
        //dostuff
        Shared::Map::MapSurfaceType surf = Shared::Map::st_Grass;
        switch(surface){
            case 1:
                surf = Shared::Map::st_Secondary_Grass;
                break;
            case 2:
                surf = Shared::Map::st_Road;
                break;
            case 3:
                surf = Shared::Map::st_Stone;
                break;
            case 4:
                surf = Shared::Map::st_Ground;
                break;
        }
        this->renderer->getMap()->reset(width, height, altitude, surf);
        //this->renderer->getMap()->resize(int w, int h, float alt, MapSurfaceType surf);
        this->renderer->getMap()->resetFactions(players);
        this->renderer->updatePlayerPositions();
        this->renderer->updateMaxPlayers();
        this->renderer->resize();//changes width and height; updates and recalculates map
        this->renderer->clearHistory();
        this->fitSelection();
    }

    void MapManipulator::flipDiagonal(){
        this->flipX();
        this->renderer->forgetLast();
        this->flipY();
    }

    void MapManipulator::flipX(){
        int midColumn = this->selectionStartColumn + ( (this->selectionEndColumn - this->selectionStartColumn) / 2);
        this->axisTool('s', midColumn, this->selectionEndRow, true, false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::flipY(){
        int midRow = this->selectionStartRow + ( (this->selectionEndRow - this->selectionStartRow) / 2);
        this->axisTool('s', this->selectionEndColumn, midRow, false, true);
        this->renderer->updatePlayerPositions();
        this->updateEverything();;
    }

    void MapManipulator::randomizeHeights(){//TODO: Just in selection
        this->renderer->getMap()->randomizeHeights();
        this->updateEverything();
    }

    void MapManipulator::randomizeHeightsAndPlayers(){//TODO: Just in selection
        this->renderer->getMap()->randomize();
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::copyL2R(){
        int midColumn = this->selectionStartColumn + ( (this->selectionEndColumn - this->selectionStartColumn) / 2);
        this->axisTool('c', midColumn, this->selectionEndRow, true, false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::copyT2B(){
        int midRow = this->selectionStartRow + ( (this->selectionEndRow - this->selectionStartRow) / 2);
        this->axisTool('c', this->selectionEndColumn, midRow, false, true);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    //a square would be handy
    void MapManipulator::copyBL2TR(){//shit
        //this->renderer->getMap()->;
        //this->axisTool('c', this->selectionEndColumn, this->selectionEndRow, true, true);
        diagonalTool('c', this->selectionEndColumn, this->selectionEndRow, false, false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::rotateL2R(){
        int midColumn = this->selectionStartColumn + ( (this->selectionEndColumn - this->selectionStartColumn) / 2);
        this->axisTool('c', midColumn, this->selectionEndRow, true, true);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::rotateT2B(){
        int midRow = this->selectionStartRow + ( (this->selectionEndRow - this->selectionStartRow) / 2);
        this->axisTool('c', this->selectionEndColumn, midRow, true, true);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }
    void MapManipulator::rotateBL2TR(){
        //this->renderer->getMap()->;
        diagonalTool('c', this->selectionEndColumn, this->selectionEndRow, true, true);
        this->updateEverything();
    }
    void MapManipulator::rotateTL2BR(){
        //this->renderer->getMap()->;
        this->updateEverything();
    }
    void MapManipulator::switchSurfaces(int a, int b){//TODO: Just in selection
        //this->renderer->getMap()->;
        this->updateEverything();
    }

    void MapManipulator::setInfo(const std::string &title, const std::string &description, const std::string &author){
        this->renderer->getMap()->setTitle(title);
        this->renderer->getMap()->setDesc(description);
        this->renderer->getMap()->setAuthor(author);
        this->updateEverything();
    }

    void MapManipulator::setAdvanced(int heightFactor, int waterLevel, int cliffLevel, int cameraHeight){
        this->renderer->getMap()->setAdvanced(heightFactor, waterLevel, cliffLevel, cameraHeight);
        this->updateEverything();
    }

    void MapManipulator::resetPlayers(int amount){
        this->renderer->getMap()->resetFactions(amount);
        this->updateEverything();
    }

    void MapManipulator::resize(int width, int height){
        //TODO: refit the selection and player positions!
        //this->renderer->getMap()->
        this->fitSelection();
        this->updateEverything();
    }

    void MapManipulator::setSelection(int startColumn, int startRow, int endColumn, int endRow){
        if(endRow == -1 || endColumn == -1 || startRow == -1 || startColumn == -1){
            //select all
            this->selectionStartColumn = 0;
            this->selectionStartRow = 0;
            this->selectionEndColumn = this->renderer->getMap()->getH()-1;
            this->selectionEndRow = this->renderer->getMap()->getW()-1;
        }else{
            this->selectionStartColumn = startColumn;
            this->selectionStartRow = startRow;
            this->selectionEndColumn = endColumn;
            this->selectionEndRow = endRow;
        }
        this->fitSelection();
    }

    void MapManipulator::enableSelctionsquare(bool enable){
        this->selectionsquare = enable;
    }

    void MapManipulator::selectionChanged(){
        //std::cout << "selected something" << std::endl;
        QRectF selection = this->renderer->getScene()->selectionArea().boundingRect();
        this->selectionStartColumn = selection.x() / Tile::getSize();
        this->selectionStartRow = selection.y() / Tile::getSize();
        this->selectionEndColumn = (selection.x() + selection.width()) / Tile::getSize();
        this->selectionEndRow = (selection.y() + selection.height()) / Tile::getSize();
        this->fitSelection();
    }

    void MapManipulator::fitSelection(){
        if(this->selectionStartColumn < 0)
            this->selectionStartColumn = 0;
        if(this->selectionStartRow < 0)
            this->selectionStartRow = 0;
        if(this->selectionEndColumn > this->renderer->getMap()->getH()-1)
            this->selectionEndColumn = this->renderer->getMap()->getH()-1;
        if(this->selectionEndRow > this->renderer->getMap()->getW()-1)
            this->selectionEndRow = this->renderer->getMap()->getW()-1;
        if(this->selectionsquare){
            int size =  std::min( (this->selectionEndColumn - this->selectionStartColumn)
                                 ,(this->selectionEndRow - this->selectionStartRow) );
            this->selectionEndColumn = size + this->selectionStartColumn;
            this->selectionEndRow = size + this->selectionStartRow;
        }
        this->renderer->getSelectionRect()->move(this->selectionStartColumn,this->selectionStartRow);
        this->renderer->getSelectionRect()->resize(this->selectionEndColumn - this->selectionStartColumn + 1,this->selectionEndRow - this->selectionStartRow + 1);
    }

    void MapManipulator::updateEverything(){//TODO: Just in selection
        this->renderer->updateMap();
        this->renderer->recalculateAll();
        this->renderer->addHistory();
    }

    void MapManipulator::axisTool(char modus, int columnLimit, int rowLimit, bool invertColumn, bool invertRow){
        //c: copy to; s: swap to
        Shared::Map::MapPreview *gmap = this->renderer->getMap();
        for(int column = this->selectionStartColumn; column <= columnLimit; column++){
            for(int row = this->selectionStartRow; row <= rowLimit; row++){
                int destinationColumn = column;
                if(invertColumn){
                    destinationColumn = this->selectionEndColumn - (destinationColumn - this->selectionStartColumn);
                }
                int destinationRow = row;
                if(invertRow){
                    destinationRow = this->selectionEndRow - (destinationRow - this->selectionStartRow);
                }
                switch(modus){
                 case 's':
                    gmap->swapXY(destinationColumn, destinationRow, column, row);
                    break;
                 case 'c':
                    gmap->copyXY(destinationColumn, destinationRow, column, row);
                    break;
                }
            }
        }
        int max = gmap->getMaxFactions();
        int freeForChange = 0;//remember which player got repositioned
        for(int i = 0; i < max; i++){
            int x = gmap->getStartLocationX(i);
            int y = gmap->getStartLocationY(i);
            //use positions of players in top part

            int destinationX = x;
            if(invertColumn){
                destinationX = this->selectionEndColumn - (x - this->selectionStartColumn);
            }
            int destinationY = y;
            if(invertRow){
                destinationY = this->selectionEndRow - (y - this->selectionStartRow);
            }
            switch(modus){
             case 's':
                if(x >= selectionStartColumn && y >= selectionStartRow && x <= selectionEndColumn && y <= selectionEndRow){
                    gmap->changeStartLocation(destinationX, destinationY, i);
                }
                break;
             case 'c':
                if(x >= selectionStartColumn && y >= selectionStartRow && x <= columnLimit && y <= rowLimit){
                    bool foundSth = false;
                    while(freeForChange < max && !foundSth){
                        int changeX = gmap->getStartLocationX(freeForChange);
                        int changeY = gmap->getStartLocationY(freeForChange);
                        //reposition players in bottom part
                        //only respect the limit if not divided
                        if((columnLimit==selectionEndColumn ? changeX >= selectionStartColumn : changeX > columnLimit)
                        && (rowLimit==selectionEndRow ? changeY >= selectionStartRow : changeY > rowLimit)
                        && changeX <= selectionEndColumn && changeY <= selectionEndRow){
                            gmap->changeStartLocation(destinationX, destinationY, freeForChange);
                            foundSth = true;
                        }
                        freeForChange++;
                    }
                }
                break;
            }
        }

    }

    void MapManipulator::diagonalTool(char modus, int columnLimit, int rowLimit, bool invertColumn, bool invertRow){
        //c: copy to; s: swap to
        Shared::Map::MapPreview *gmap = this->renderer->getMap();
        for(int column = this->selectionStartColumn; column <= columnLimit; column++){
            for(int row = this->selectionStartRow + column; row <= rowLimit; row++){
                int destinationColumn = column;
                if(invertColumn){
                    destinationColumn = this->selectionEndColumn - (destinationColumn - this->selectionStartColumn);
                }
                int destinationRow = row;
                if(invertRow){
                    destinationRow = this->selectionEndRow - (destinationRow - this->selectionStartRow);
                }
                switch(modus){
                 case 's':
                    gmap->swapXY(destinationColumn, destinationRow, column, row);
                    break;
                 case 'c':
                    gmap->copyXY(destinationColumn, destinationRow, column, row);
                    break;
                }
            }
        }
        /*int max = gmap->getMaxFactions();
        int freeForChange = 0;//remember which player got repositioned
        for(int i = 0; i < max; i++){
            int x = gmap->getStartLocationX(i);
            int y = gmap->getStartLocationY(i);
            //use positions of players in top part

            int destinationX = x;
            if(invertColumn){
                destinationX = this->selectionEndColumn - (x - this->selectionStartColumn);
            }
            int destinationY = y;
            if(invertRow){
                destinationY = this->selectionEndRow - (y - this->selectionStartRow);
            }
            switch(modus){
             case 's':
                if(x >= selectionStartColumn && y >= selectionStartRow && x <= selectionEndColumn && y <= selectionEndRow){
                    gmap->changeStartLocation(destinationX, destinationY, i);
                }
                break;
             case 'c':
                if(x >= selectionStartColumn && y >= selectionStartRow && x <= columnLimit && y <= rowLimit){
                    bool foundSth = false;
                    while(freeForChange < max && !foundSth){
                        int changeX = gmap->getStartLocationX(freeForChange);
                        int changeY = gmap->getStartLocationY(freeForChange);
                        //reposition players in bottom part
                        //only respect the limit if not divided
                        if((columnLimit==selectionEndColumn ? changeX >= selectionStartColumn : changeX > columnLimit)
                        && (rowLimit==selectionEndRow ? changeY >= selectionStartRow : changeY > rowLimit)
                        && changeX <= selectionEndColumn && changeY <= selectionEndRow){
                            gmap->changeStartLocation(destinationX, destinationY, freeForChange);
                            foundSth = true;
                        }
                        freeForChange++;
                    }
                }
                break;
            }
        }*/

    }
}
