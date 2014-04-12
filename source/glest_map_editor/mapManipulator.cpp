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
#include <QApplication>
#include <QClipboard>
#include <QByteArray>
#include <QMimeData>

#include <QDebug>


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

    void MapManipulator::fitSelection(bool useUserSettings){
        //check if selection box is within the map
        if(this->selectionStartColumn < 0)
            this->selectionStartColumn = 0;
        if(this->selectionStartRow < 0)
            this->selectionStartRow = 0;
        if(this->selectionEndColumn > this->renderer->getMap()->getH()-1)
            this->selectionEndColumn = this->renderer->getMap()->getH()-1;
        if(this->selectionEndRow > this->renderer->getMap()->getW()-1)
            this->selectionEndRow = this->renderer->getMap()->getW()-1;
        //make the selection a square
        if(useUserSettings && this->selectionsquare){
            int size =  std::min( (this->selectionEndColumn - this->selectionStartColumn)
                                 ,(this->selectionEndRow - this->selectionStartRow) );
            this->selectionEndColumn = size + this->selectionStartColumn;
            this->selectionEndRow = size + this->selectionStartRow;
        }
        //update the visual rectangle
        this->renderer->getSelectionRect()->move(this->selectionStartColumn,this->selectionStartRow);
        this->renderer->getSelectionRect()->resize(this->selectionEndColumn - this->selectionStartColumn + 1,this->selectionEndRow - this->selectionStartRow + 1);
    }

    void MapManipulator::copy(){
        Shared::Map::MapPreview *map = this->renderer->getMap();
        //store the header infos
        SubMapHeader header;
        header.column = this->selectionStartColumn;
        header.row = this->selectionStartRow;
        header.height = this->selectionEndColumn - this->selectionStartColumn + 1;
        header.width = this->selectionEndRow - this->selectionStartRow + 1;

        std::cout << "column: " << header.column << "; row: " << header.row
            << "; width: " << header.width << "; height: " << header.height << std::endl;

        QByteArray subMapData( (const char*) &header, sizeof(SubMapHeader) );
        //store the heights
        for(int column = 0; column < header.height; column++){
            for(int row = 0; row < header.width; row++){
                float height = map->getHeight(header.column + column, header.row + row);
                //convert to bytes
                subMapData.append(QByteArray( (const char*) &height, sizeof(float) ));
                ///std::cout << height << ", ";
            }
            ///std::cout << std::endl;
        }
        ///std::cout << std::endl;

        //store the surfaces
        for(int column = 0; column < header.height; column++){
            for(int row = 0; row < header.width; row++){
                Shared::Map::MapSurfaceType surface = map->getSurface(header.column + column, header.row + row);
                //convert to bytes
                subMapData.append(QByteArray( (const char*) &surface, sizeof(Shared::Map::MapSurfaceType) ));
                ///std::cout << surface << ", ";
            }
            ///std::cout << std::endl;
        }
        ///std::cout << std::endl;

        //store the resources
        for(int column = 0; column < header.height; column++){
            for(int row = 0; row < header.width; row++){
                int resource = map->getResource(header.column + column, header.row + row);
                //convert to bytes
                subMapData.append(QByteArray( (const char*) &resource, sizeof(int) ));
                ///std::cout << resource << ", ";
            }
            ///std::cout << std::endl;
        }
        ///std::cout << std::endl;

        //store the objects
        for(int column = 0; column < header.height; column++){
            for(int row = 0; row < header.width; row++){
                int object = map->getObject(header.column + column, header.row + row);
                //convert to bytes
                subMapData.append(QByteArray( (const char*) &object, sizeof(int) ));
                ///std::cout << object << ", ";
            }
            ///std::cout << std::endl;
        }
        ///std::cout << std::endl;
        ///qDebug() << "copy (" << subMapData.size() << "): " << subMapData.toHex();

        //create a mime for clipboard
        QMimeData *mimeData = new QMimeData;
        mimeData->setData("application/mg-submap", subMapData);
        QApplication::clipboard()->setMimeData(mimeData);

    }
    void MapManipulator::paste(){
        Shared::Map::MapPreview *map = this->renderer->getMap();

        const QMimeData *mimeData = QApplication::clipboard()->mimeData();

        //check if clipboard contains a submap
        if(mimeData && mimeData->hasFormat("application/mg-submap")){
            QByteArray subMapData = mimeData->data("application/mg-submap");
            ///qDebug() << "paste (" << subMapData.size() << "): " << subMapData.toHex();

            //read the header
            SubMapHeader *header = (SubMapHeader*)( subMapData.left(sizeof(SubMapHeader)).constData() );
            ///std::cout << "column: " << header->column << "; row: " << header->row
            ///<< "; width: " << header->width << "; height: " << header->height << std::endl;

            int rowOffset = this->selectionStartRow;
            int columnOffset = this->selectionStartColumn;

            //only paste within the map
            int maxWidth = std::min(header->height,map->getH() - rowOffset);
            int maxHeight = std::min(header->width,map->getW() - columnOffset);

            //resize selection
            /*this->selectionEndRow = rowOffset + maxHeight - 1;
            this->selectionEndColumn = columnOffset + maxWidth - 1;*/
            //this->renderer->getSelectionRect()->resize(maxWidth, maxHeight);
            //this->fitSelection(false);

            //we need to know, where we want to read
            int position = sizeof(SubMapHeader);

            //paste the four parts of the map
            int byteSize;
            enum MapSection {HEIGHT, SURFACE, RESOURCE, OBJECT};
            for(int section = HEIGHT; section <= OBJECT; section++){
                switch(section){
                    case HEIGHT:
                        byteSize = sizeof(float);
                        break;
                    case SURFACE:
                        byteSize = sizeof(Shared::Map::MapSurfaceType);
                        break;
                    default:
                        byteSize = sizeof(int);
                        break;
                }
                for(int column = 0; column < maxHeight; column++){
                    for(int row = 0; row < maxWidth; row++){
                        switch(section){
                            case HEIGHT:{
                                //convert from byte to float
                                float height = *(float*)(subMapData.mid( position, byteSize ).data());
                                //insert into map
                                map->setHeight(columnOffset + column, rowOffset + row, height);
                                break;
                            }
                            case SURFACE:{
                                //convert from byte to surface
                                Shared::Map::MapSurfaceType surface = *(Shared::Map::MapSurfaceType*)(subMapData.mid( position, byteSize ).data());
                                map->setSurface(columnOffset + column, rowOffset + row, surface);
                                break;
                            }
                            case RESOURCE:{
                                //convert from byte to int
                                int resource = *(int*)(subMapData.mid( position, byteSize ).data());
                                map->setResource(columnOffset + column, rowOffset + row, resource);
                                break;
                            }
                            case OBJECT:{
                                //convert from byte to int
                                int object = *(int*)(subMapData.mid( position, byteSize ).data());
                                map->setObject(columnOffset + column, rowOffset + row, object);
                                break;
                            }
                        }
                        ///std::cout << "(" << subMapData.mid( position, byteSize ).toHex().data() << ") ";
                        ///std::cout << height << ", ";
                        position += byteSize;
                    }
                    //skip all cells not copied in this column
                    position += byteSize * (header->width - maxWidth);
                    ///std::cout << std::endl;
                }
                //skip all not copied rows
                position += byteSize * (header->width) * (header->height - maxHeight);
                ///std::cout << std::endl;
            }

            //this action should be reversible
            this->updateEverything();
            std::cout << "copy done" << std::endl;
        }
    }

    void MapManipulator::updateEverything(){//TODO: Just in selection
        this->renderer->updateMap();
        std::cout << "updeted map" << std::endl;
        this->renderer->recalculateAll();
        std::cout << "recalculation done" << std::endl;
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
