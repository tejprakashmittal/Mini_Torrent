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

    int input,a=6,b=8,r=0;
    cin>>input;
    write(skt,&input,sizeof(input));

    if(input == 1){

        while(1)
        {
            cin>>a>>b;
            write(skt,&a,sizeof(a));
            write(skt,&b,sizeof(b));
            read(skt,&r,sizeof(r));

            cout<<"Multiplication Result is : "<<r<<endl;
            fflush(stdout);
        }
    }
    else if(input == 2){
        while(1){
            cin>>a>>b;
            if(send(skt,&a,sizeof(a),0) == -1);
            if(send(skt,&b,sizeof(b),0) == -1);
            recv(skt,&r,sizeof(r),0);

            cout<<"Addition Result is : "<<r<<endl;
            fflush(stdout); 
        }
    }
    close(skt);
    return 0;
}