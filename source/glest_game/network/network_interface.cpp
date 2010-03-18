// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "network_interface.h"

#include <exception>
#include <cassert>

#include "types.h"
#include "conversion.h"
#include "platform_util.h"

#include "leak_dumper.h"
#include <fstream>

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkInterface
// =====================================================

const int NetworkInterface::readyWaitTimeout= 60000;	//1 minute

bool NetworkInterface::allowGameDataSynchCheck  = false;
bool NetworkInterface::allowDownloadDataSynch   = false;
DisplayMessageFunction NetworkInterface::pCB_DisplayMessage = NULL;

void NetworkInterface::sendMessage(const NetworkMessage* networkMessage){
	Socket* socket= getSocket();

	networkMessage->send(socket);
}

NetworkMessageType NetworkInterface::getNextMessageType(bool checkHasDataFirst)
{
	Socket* socket= getSocket();
	int8 messageType= nmtInvalid;

    if(checkHasDataFirst == false ||
       (checkHasDataFirst == true &&
        socket != NULL &&
        socket->hasDataToRead() == true))
    {
        //peek message type
        int dataSize = socket->getDataToRead();
        if(dataSize >= sizeof(messageType)){
            if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] socket->getDataToRead() dataSize = %d\n",__FILE__,__FUNCTION__,dataSize);

            int iPeek = socket->peek(&messageType, sizeof(messageType));

            if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] socket->getDataToRead() iPeek = %d, messageType = %d\n",__FILE__,__FUNCTION__,iPeek,messageType);
        }

        //sanity check new message type
        if(messageType<0 || messageType>=nmtCount){
            throw runtime_error("Invalid message type: " + intToStr(messageType));
        }
    }

	return static_cast<NetworkMessageType>(messageType);
}

bool NetworkInterface::receiveMessage(NetworkMessage* networkMessage){

    if(Socket::enableNetworkDebugInfo) printf("In [%s::%s]\n",__FILE__,__FUNCTION__);

	Socket* socket= getSocket();

	return networkMessage->receive(socket);
}

bool NetworkInterface::isConnected(){
    //if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] START\n",__FILE__,__FUNCTION__);

    bool result = (getSocket()!=NULL && getSocket()->isConnected());

    //if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] END\n",__FILE__,__FUNCTION__);

	return result;
}

void NetworkInterface::DisplayErrorMessage(string sErr, bool closeSocket) {

    if(closeSocket == true && getSocket() != NULL)
    {
        close();
    }

    if(pCB_DisplayMessage != NULL) {
        pCB_DisplayMessage(sErr.c_str(), false);
    }
    else {
        throw runtime_error(sErr);
    }


}

// =====================================================
//	class GameNetworkInterface
// =====================================================

GameNetworkInterface::GameNetworkInterface(){
	quit= false;
}

void GameNetworkInterface::requestCommand(const NetworkCommand *networkCommand, bool insertAtStart) {
    if(insertAtStart == false) {
        requestedCommands.push_back(*networkCommand);
    }
    else {
        requestedCommands.insert(requestedCommands.begin(),*networkCommand);
    }
}

// =====================================================
//	class FileTransferSocketThread
// =====================================================

const int32 SEND_FILE = 0x20;
const int32 ACK       = 0x47;

FileTransferSocketThread::FileTransferSocketThread(FileTransferInfo fileInfo)
{
    this->info = fileInfo;
    this->info.serverPort += 100;
}

void FileTransferSocketThread::execute()
{
    if(info.hostType == eServer)
    {
        ServerSocket serverSocket;
        //serverSocket.setBlock(false);
        serverSocket.bind(this->info.serverPort);
        serverSocket.listen(1);
        Socket *clientSocket = serverSocket.accept();

        char data[513]="";
        memset(data, 0, 256);

        clientSocket->receive(data,256);
        if(*data == SEND_FILE)
        {
            FileInfo file;

            memcpy(&file, data+1, sizeof(file));

            *data=ACK;
            clientSocket->send(data,256);

            Checksum checksum;
            checksum.addFile(file.fileName);
            file.filecrc  = checksum.getSum();

            ifstream infile(file.fileName.c_str(), ios::in | ios::binary | ios::ate);
            if(infile.is_open() == true)
            {
                file.filesize = infile.tellg();
                infile.seekg (0, ios::beg);

                memset(data, 0, 256);
                *data=SEND_FILE;
                memcpy(data+1,&file,sizeof(file));

                clientSocket->send(data,256);
                clientSocket->receive(data,256);
                if(*data != ACK)
                   ;//transfer error

                int remain=file.filesize % 512 ;
                int packs=(file.filesize-remain)/512;

                while(packs--)
                {
                    infile.read(data,512);
                    //if(!ReadFile(file,data,512,&read,NULL))
                    //    ; //read error
                    //if(written!=pack)
                    //    ; //read error
                    clientSocket->send(data,512);
                    clientSocket->receive(data,256);
                    if(*data!=ACK)
                           ;//transfer error
                }

                infile.read(data,remain);
                //if(!ReadFile(file,data,remain,&read,NULL))
                //   ; //read error
                //if(written!=pack)
                //   ; //read error

                clientSocket->send(data,remain);
                clientSocket->receive(data,256);
                if(*data!=ACK)
                   ;//transfer error

                infile.close();
            }
        }

        delete clientSocket;
    }
    else
    {
        Ip ip(this->info.serverIP);
        ClientSocket clientSocket;
        //clientSocket.setBlock(false);
        clientSocket.connect(this->info.serverIP, this->info.serverPort);

        if(clientSocket.isConnected() == true)
        {
            FileInfo file;
            file.fileName = this->info.fileName;
            //file.filesize =
            //file.filecrc  = this->info.

            string path = extractDirectoryPathFromFile(file.fileName);
            createDirectoryPaths(path);
            ofstream outFile(file.fileName.c_str(), ios_base::binary | ios_base::out);
            if(outFile.is_open() == true)
            {
                char data[513]="";
                memset(data, 0, 256);
                *data=SEND_FILE;
                memcpy(data+1,&file,sizeof(file));

                clientSocket.send(data,256);
                clientSocket.receive(data,256);
                if(*data!=ACK)
                  ;//transfer error

                clientSocket.receive(data,256);
                if(*data == SEND_FILE)
                {
                   memcpy(&file, data+1, sizeof(file));
                   *data=ACK;
                   clientSocket.send(data,256);

                   int remain = file.filesize % 512 ;
                   int packs  = (file.filesize-remain) / 512;

                   while(packs--)
                   {
                      clientSocket.receive(data,512);

                      outFile.write(data, 512);
                      if(outFile.bad())
                      {
                          int ii = 0;
                      }
                      //if(!WriteFile(file,data,512,&written,NULL))
                      //   ; //write error
                      //if(written != pack)
                      //   ; //write error
                      *data=ACK;
                      clientSocket.send(data,256);
                    }
                    clientSocket.receive(data,remain);

                    outFile.write(data, remain);
                    if(outFile.bad())
                    {
                        int ii = 0;
                    }

                    //if(!WriteFile(file,data,remain,&written,NULL))
                    //    ; //write error
                    //if(written!=pack)
                    //    ; //write error
                    *data=ACK;
                    clientSocket.send(data,256);

                    Checksum checksum;
                    checksum.addFile(file.fileName);
                    int32 crc = checksum.getSum();
                    if(file.filecrc != crc)
                    {
                        int ii = 0;
                    }

                    //if(calc_crc(file)!=info.crc)
                    //   ; //transfeer error
                }

                outFile.close();
            }
        }
    }
}


}}//end namespace
