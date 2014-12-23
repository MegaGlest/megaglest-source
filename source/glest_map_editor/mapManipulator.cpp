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

#include <QAction>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QString>
#include <QApplication>
#include <QClipboard>
#include <QByteArray>
#include <QMimeData>
#include <QMessageBox>
#include <QDebug>

#ifdef WIN32
    #include <winsock2.h>
#endif

#include "mapManipulator.hpp"

#include "mainWindow.hpp"
#include "renderer.hpp"
#include "selection.hpp"
#include "tile.hpp"

#include "map_preview.h"
#include "platform_util.h"

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
        this->selectAll();

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

    void MapManipulator::flipDiagonal(){
        /*this->flipX();
        this->renderer->forgetLast();
        this->flipY();*/
        axisTool('-',true,true,false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::flipX(){
        //int midColumn = this->selectionStartColumn + ( (this->selectionEndColumn - this->selectionStartColumn) / 2);
        //this->axisTool('s', midColumn, this->selectionEndRow, true, false);
        axisTool('-',true,false,false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::flipY(){
        //int midRow = this->selectionStartRow + ( (this->selectionEndRow - this->selectionStartRow) / 2);
        //this->axisTool('s', this->selectionEndColumn, midRow, false, true);
        axisTool('|',true,false,false);
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
        //int midColumn = this->selectionStartColumn + ( (this->selectionEndColumn - this->selectionStartColumn) / 2);
        //this->axisTool('c', midColumn, this->selectionEndRow, true, false);
        axisTool('|',false,false,false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::copyT2B(){
        //int midRow = this->selectionStartRow + ( (this->selectionEndRow - this->selectionStartRow) / 2);
        //this->axisTool('c', this->selectionEndColumn, midRow, false, true);
        axisTool('-',false,false,false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    //a square would be handy
    void MapManipulator::copyBL2TR(){//shit
        //this->renderer->getMap()->;
        //this->axisTool('c', this->selectionEndColumn, this->selectionEndRow, true, true);
        //diagonalTool('c', this->selectionEndColumn, this->selectionEndRow, false, true, true);
        axisTool('\\',false,false,false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::rotateL2R(){
        //int midColumn = this->selectionStartColumn + ( (this->selectionEndColumn - this->selectionStartColumn) / 2);
        //this->axisTool('c', midColumn, this->selectionEndRow, true, true);
        axisTool('|',false,true,false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::rotateT2B(){
        //int midRow = this->selectionStartRow + ( (this->selectionEndRow - this->selectionStartRow) / 2);
        //this->axisTool('c', this->selectionEndColumn, midRow, true, true);
        axisTool('-',false,true,false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::rotateBL2TR(){
        //this->renderer->getMap()->;
        //diagonalTool('c', this->selectionEndColumn, this->selectionEndRow, false, true, false);
        axisTool('\\',false,true,false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::rotateTL2BR(){
        //this->renderer->getMap()->;
        //diagonalTool('c', this->selectionEndColumn, this->selectionEndRow, true, false, false);
        axisTool('/',false,true,false);
        this->renderer->updatePlayerPositions();
        this->updateEverything();
    }

    void MapManipulator::switchSurfaces(int a, int b){
        Shared::Map::MapPreview *map = this->renderer->getMap();
        //cast to surface, surfaces start with 1
        Shared::Map::MapSurfaceType firstSurface = (Shared::Map::MapSurfaceType)(a + 1);
        Shared::Map::MapSurfaceType secondSurface = (Shared::Map::MapSurfaceType)(b + 1);
        //switch everything in selection
        for(int column = this->selectionStartColumn; column <= this->selectionEndColumn; column++){
            for(int row = this->selectionStartRow; row <= this->selectionEndRow; row++){
                if(map->getSurface(column, row) == firstSurface){
                    map->setSurface(column, row, secondSurface);
                }else if(map->getSurface(column, row) == secondSurface){
                    map->setSurface(column, row, firstSurface);
                }
            }
        }
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

    void MapManipulator::reset(int width, int height, int surface, float altitude, int players){
        try{
            this->renderer->getMap()->reset(width, height, altitude, (Shared::Map::MapSurfaceType)(surface+1));
            this->renderer->getMap()->resetFactions(players);
            this->renderer->updatePlayerPositions();
            this->renderer->updateMaxPlayers();
            this->renderer->resize();//changes width and height; updates and recalculates map
            this->renderer->clearHistory();
            this->selectAll();
        }catch(const Shared::Platform::megaglest_runtime_error &ex) {
            this->win->showRuntimeError("Can’t reset map, wrong parameters.", ex);
        }
    }

    void MapManipulator::changePlayerAmount(int amount){
        this->renderer->getMap()->resetFactions(amount);
        this->renderer->restorePlayerPositions();
        this->renderer->updateMaxPlayers();
        this->updateEverything();
    }

    void MapManipulator::resize(int width, int height, int surface, float altitude){
        try{
            this->renderer->getMap()->resize(width, height, altitude, (Shared::Map::MapSurfaceType)(surface+1));
            this->renderer->resize();
            this->fitSelection();
            this->updateEverything();
        }catch(const Shared::Platform::megaglest_runtime_error &ex) {
            this->win->showRuntimeError("Can’t resize map, wrong parameters.", ex);
        }
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

    void MapManipulator::selectAll(){
        this->selectionStartColumn = 0;
        this->selectionStartRow = 0;
        //std::cout << this->renderer->getMap() << std::endl;
        this->selectionEndColumn = this->renderer->getHeight()-1;
        this->selectionEndRow = this->renderer->getWidth()-1;
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
            int size =  min( (this->selectionEndColumn - this->selectionStartColumn)
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

        //~ std::cout << "column: " << header.column << "; row: " << header.row
            //~ << "; width: " << header.width << "; height: " << header.height << std::endl;

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

        int rowOffset = this->selectionStartRow;
        int columnOffset = this->selectionStartColumn;

        QGraphicsView* v = win->getView();
        QPointF p = v->mapToScene(v->mapFromGlobal(QCursor::pos()));
        p /= Tile::getSize(); //reduce position to tiles
        //~ std::cout << "x: " << p.x() << "; y: " << p.y() << std::endl;
        if(p.x() >= 0 && p.y() >= 0 &&
           p.x() < getWidth() && p.y() < getHeight() ){
            rowOffset = p.y();
            columnOffset = p.x();
        }

        //check if clipboard contains a submap
        if(mimeData && mimeData->hasFormat("application/mg-submap")){
            QByteArray subMapData = mimeData->data("application/mg-submap");
            ///qDebug() << "paste (" << subMapData.size() << "): " << subMapData.toHex();

            //read the header
            SubMapHeader header = *(SubMapHeader*)(subMapData.left(sizeof(SubMapHeader)).constData());
            //~ std::cout << "column: " << header->column << "; row: " << header->row
            //~ << "; width: " << header->width << "; height: " << header->height << std::endl;


            //only paste within the map
            int maxHeight = min(header.height,renderer->getHeight() - rowOffset);
            int maxWidth = min(header.width,renderer->getWidth() - columnOffset);

            //resize selection
            this->selectionStartRow = rowOffset;
            this->selectionStartColumn = columnOffset;
            this->selectionEndRow = rowOffset + maxHeight - 1;
            this->selectionEndColumn = columnOffset + maxWidth - 1;
            this->fitSelection(false);

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
                for(int column = 0; column < maxWidth; column++){
                    for(int row = 0; row < maxHeight; row++){
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
                    position += byteSize * (header.width - maxHeight);
                    ///std::cout << std::endl;
                }
                //skip all not copied rows
                position += byteSize * (header.width) * (header.height - maxWidth);
                ///std::cout << std::endl;
            }

            //this action should be reversible
            this->updateEverything();
        }
    }

    int MapManipulator::getHeight(){
        return this->renderer->getHeight();
    }
    int MapManipulator::getWidth(){
        return this->renderer->getWidth();
    }
    int MapManipulator::getPlayerAmount(){
        return this->renderer->getMap()->getMaxFactions();
    }

    void MapManipulator::updateEverything(){//TODO: Just in selection
        //only makes sense if paste changes selected area
        this->renderer->updateMap();
        this->renderer->recalculateAll();
        this->renderer->addHistory();
    }

    //TODO: diagonal projection seems buggy
    int MapManipulator::projectCol(int col, int row, bool rotate, char modus){//axis: '\' '/' '-' '|'
        int width = (this->selectionEndColumn - this->selectionStartColumn);
        //int height = (this->selectionEndRow - this->selectionStartRow);
        if(rotate){//just mirror in both directions
            return width - col;
        }else{//mirroring is more complicated
            switch(modus){
              case '\\':
                return row;//transpone
              case '/':
                return width - row;//first mirror to / and transpone then
              case '|'://just mirror
                return width - col;
              default://do nothing
                return col;
            }
        }
    }

    //TODO: diagonal projection seems buggy
    int MapManipulator::projectRow(int col, int row, bool rotate, char modus){//axis: '\' '/' '-' '|'
        int width = (this->selectionEndColumn - this->selectionStartColumn);
        int height = (this->selectionEndRow - this->selectionStartRow);
        if(rotate){//just mirror in both directions
            return height - row;
        }else{//mirroring is more complicated
            switch(modus){
              case '\\':
                return col;//transpone
              case '/':
                return width - col;//first mirror to / and transpone then
              case '-'://just mirror
                return height - row;
              default://do nothing
                return row;
            }
        }
    }

    //rotation by pi is just mirroring twice
    //transponing is just switching x and y axis
    void MapManipulator::axisTool(char modus, bool swap, bool rotate, bool inverted){
        //axis: '\' '/' '-' '|'
        int width = (this->selectionEndColumn - this->selectionStartColumn);
        int height = (this->selectionEndRow - this->selectionStartRow);

        Shared::Map::MapPreview *gmap = this->renderer->getMap();

        int column = 0;
        int maxcol = width;
        if(modus == '|'){
            if(inverted){
                column = 0;
            }else{
                maxcol = width/2.0;
            }
        }
        for(; column <= maxcol; column++){
            int maxrow = height;
            int row = 0;
            switch(modus){
              case '\\':
                if(inverted){
                    maxrow = column;
                }else{
                    row = column;
                }
                break;
              case '/':
                if(inverted){
                    row = height-column;
                }else{
                    maxrow = height-column;
                }
                break;
              case '-':
                if(inverted){
                    row = 0;
                }else{
                    maxrow = height/2.0;
                }
                break;
            }

            for(; row <= maxrow; row++){
                if(swap){
                    gmap->swapXY(selectionStartColumn+projectCol(column,row,rotate,modus),
                                 selectionStartRow+projectRow(column,row,rotate,modus),
                                 selectionStartColumn+column, selectionStartRow+row);
                }else{
                    gmap->copyXY(selectionStartColumn+projectCol(column,row,rotate,modus),
                                 selectionStartRow+projectRow(column,row,rotate,modus),
                                 selectionStartColumn+column, selectionStartRow+row);
                }
            }
        }

        //TODO: it seems like in most cases player positions don’t change
        int max = gmap->getMaxFactions();
        int positionFlag[Shared::Map::MAX_MAP_FACTIONCOUNT];
        //find and flag all players
        for(int i = 0; i < max; i++){
            int x = gmap->getStartLocationX(i) - this->selectionStartColumn;
            int y = gmap->getStartLocationY(i) - this->selectionStartRow;

            int flag = 0;//0: not in selected area or already mirrored; 1: source; -1: destination, will be moved
            if(x >= 0 && y >= 0 && x < width && y < height){//or <= ?
                //we are definitely source or destination! or on the mirror axis
                if(swap){
                    flag = 1;
                }else{
                    switch(modus){
                      case '\\':
                        y = height - y;//first mirror to /
                      case '/':// just for cubic selection
                        if(y+x < width+1){
                            flag = 1;
                        }
                        if(y+x > width+1){
                            flag = -1;
                        }
                        break;
                      case '-'://just mirror
                        if(y < height/2.0){
                            flag = 1;
                        }
                        if(y > height/2.0){
                            flag = -1;
                        }
                        break;
                      case '|'://do nothing
                        if(x < width/2.0){
                            flag = 1;
                        }
                        if(x > width/2.0){
                            flag = -1;
                        }
                        break;
                    }
                }
            }
            //do *-1 for mirroring here
            if(inverted){
                flag *= -1;
            }
            positionFlag[i] = flag;
        }

        //do the actual mirroring
        for(int i = 0; i < max; i++){
            if(positionFlag[i] == 1){//get a source

                int x = gmap->getStartLocationX(i) - this->selectionStartColumn;
                int y = gmap->getStartLocationY(i) - this->selectionStartRow;
                int target = i;
                if(!swap){// if we don’t swap this, we need another player
                    target = -1;
                    for(int j = 0; j < max && target < 0; j++){//get a destination
                        if(positionFlag[j] == -1){
                            target = j;
                        }
                    }
                }
                gmap->changeStartLocation(selectionStartColumn+projectCol(x,y,rotate,modus),
                                          selectionStartRow+projectRow(x,y,rotate,modus), target);
                //mark as visited
                positionFlag[i] = 0;
                positionFlag[target] = 0;
            }
        }

    }

}
