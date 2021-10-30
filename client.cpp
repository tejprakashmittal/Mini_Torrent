#include<iostream>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#define BUFFER 1024
using namespace std;

int main(int argc,char *argv[]){
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    server_addr.sin_port=htons(9900);

    char buffer[BUFFER];
    string msg="";

    memset(buffer,'\0',sizeof(buffer[0]));
    //strcpy(buffer, argv[1]);
    //msg = argv[1];
    //cout<<buffer<<endl;
    /*Socket creation*/

    int skt=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(skt == -1){
        cout<<"Error while socket creation"<<endl;
        exit(0);
    }

    if(connect(skt,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
        cout<<"Error while connect syscall"<<endl;
        exit(0);
    }
    /*connect*/
    cout<<"Connected to the server----------"<<endl;
    /*send and recv*/

    

    read(skt,buffer,BUFFER);
    for(int i=0;buffer[i]!='\0';i++) msg.push_back(buffer[i]);
    cout<<msg<<endl;
    fflush(stdout);
    msg="";
    while(1)
    {
        cin>>msg;
        memset(buffer,'\0',BUFFER);
        if(send(skt,msg.c_str(),msg.size(),0) == -1) cout<<"Error while sending msg"<<endl;
        msg.clear();
        if(recv(skt,buffer,BUFFER,0) == -1) cout<<"Error while receiving msg"<<endl;

        for(int i=0;buffer[i]!='\0';i++) msg.push_back(buffer[i]);
        cout<<msg<<endl;
        fflush(stdout);
        msg.clear();
        cin>>msg;
    }
    close(skt);
    return 0;
}