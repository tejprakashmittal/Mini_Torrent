#include<iostream>
#include<string.h>
#include <fcntl.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#define BUFFER 1024
#define QUEUE 2
using namespace std;

int random(int min, int max) //range : [min, max]
{
   static bool first = true;
   if (first) 
   {  
      srand( time(NULL) ); //seeding for the first time only!
      first = false;
   }
   return min + rand() % (( max + 1 ) - min);
}

int peer_port=random(1030,49151);

void handle_client(int client_socket){
    /*send and recv*/
    char buffer[BUFFER];
    string msg="";
    char ch;
    while(1){
        read(client_socket,&buffer,BUFFER);
        //cout<<buffer<<endl;
        msg=buffer;
        write(client_socket,msg.c_str(),msg.size());
        memset(buffer,'\0',BUFFER);  
    }
    /*close socket*/
    close(client_socket);
}

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
    int status=1;
    setsockopt(skt,SOL_SOCKET,SO_REUSEADDR,&status,sizeof(int));

    if(connect(skt,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
        cout<<"Error while connect syscall"<<endl;
        exit(0);
    }
    /*connect*/
    cout<<"Connected to the server----------"<<endl;
    write(skt,&peer_port,sizeof(peer_port));
    /*send and recv*/
    while(1){
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
        else if(input == 3){
            int read_count,source;
            string source_path="./AOS_Assignment3.pdf";
            source = open(source_path.c_str(), O_RDONLY);

            while((read_count = read(source,buffer,BUFFER))>0){
                write(skt,buffer,read_count);
            }
        }
        else if(input == 4){
            read(skt,buffer,BUFFER);
            msg=buffer;
            cout<<msg;
            string file_name;
            cin>>file_name;
            write(skt,file_name.c_str(),file_name.size());

            int dest,read_count;
            string dest_full_path="./"+file_name;
            dest = open(dest_full_path.c_str(), O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
            
            while((read_count = read(skt,buffer,BUFFER))>0){
                write(dest,buffer,read_count);
            }
        }
        else if(input == 5){
            struct sockaddr_in peer;
            char peer_buffer[BUFFER];
            read(skt,&peer,sizeof(peer));
            //peer.sin_port=htons(9909);
            peer.sin_addr.s_addr=INADDR_ANY;
            memset(buffer,'\0',BUFFER);

            int peer_skt=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
            if(peer_skt == -1){
                cout<<"Error while socket creation"<<endl;
                exit(0);
            }
            if(connect(peer_skt,(struct sockaddr*)&peer,sizeof(peer)) == -1){
                cout<<"Error while connect syscall"<<endl;
                exit(0);
            }
            string peer_msg;
            cout<<"Start conversation with your peer -----------"<<endl;
            while(1){
                cin>>peer_msg;
                if(peer_msg == "exit") break;
                write(peer_skt,peer_msg.c_str(),peer_msg.size());
                memset(peer_buffer,'\0',BUFFER);
                read(peer_skt,&peer_buffer,BUFFER);
                peer_msg=peer_buffer;
                cout<<peer_msg<<endl;
                fflush(stdout);
            }        
            close(peer_skt);
        }
        else if(input == 6){
            /*Working as a server ----------------*/
        int server_socket,client_socket;
        struct sockaddr_in server_addr;
        struct sockaddr_in client_addr;

        /*Define server socket*/
        server_socket=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
        setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&status,sizeof(int));
        if(server_socket == -1){
            cout<<"Error while socket creation"<<endl;
            exit(0);
        }
        memset(&server_addr,0,sizeof(server_addr));
        server_addr.sin_family=AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(9909);

        /*Bind*/
        if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
            cout<<"Error while bind call"<<endl;
            exit(0);
        }

        /*Listen*/
        if(listen(server_socket, QUEUE) == -1){
            cout<<"Error while listen syscall"<<endl;
            exit(0);
        }
        cout<<"Welcome, Server is online -----------------"<<endl;
        unsigned int client_addr_length = sizeof(client_addr);
        while(1){
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_length);
            pid_t pid=fork();
            if(pid==0) handle_client(client_socket);
        }

        close(server_socket);
        }
    }
    close(skt);
    return 0;
}