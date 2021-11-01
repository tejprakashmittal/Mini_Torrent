#include<iostream>
#include<string.h>
#include<vector>
#include <fcntl.h>
#include<sstream>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#define BUFFER 1024
#define QUEUE 2
using namespace std;

int peer_port;
string peer_ip;

vector<string> cmd_list;
vector<string> cmd_list_buffer;

void split_command(string cmd_str){
  istringstream ss(cmd_str);
  string word;
  while(ss >> word){
    cmd_list.push_back(word);
  }
}

int toInt(string str){
    stringstream sst(str);
    int x=0;
    sst>>x;
    return x;
}

string parse_cmd_list(){
    string cmd="#";
    for(int i=0;i<cmd_list.size();i++){
        cmd+=cmd_list[i]+'#';
    }
    return cmd;
}

void parse_buffer(char buffer[]){
    string cmd="";
    for(int i=1;i<BUFFER && buffer[i]!='\0';i++){
        if(buffer[i]=='#'){
            cmd_list_buffer.push_back(cmd);
            cmd="";
        }
        else
        cmd.push_back(buffer[i]);
    }
}

void* handle_client(void *args){
    /*send and recv*/
    int client_socket=*((int*)args);
    char buffer[BUFFER];
    string msg="";
    char ch;
    while(1){
        bzero(buffer,BUFFER);
        read(client_socket,&buffer,BUFFER);
        //cout<<buffer<<endl;
        msg=buffer;
        cout<<msg<<endl;
        fflush(stdout);
        bzero(buffer, BUFFER);
		fgets(buffer, BUFFER, stdin);
        string input=buffer;
        write(client_socket,input.c_str(),msg.size());
    }
    /*close socket*/
    close(client_socket);
    pthread_exit(NULL);
}

int main(int argc,char *argv[]){

    peer_ip=argv[1];
    peer_port=atoi(argv[2]);

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
    cout<<"type command below"<<endl;
    //write(skt,&peer_port,sizeof(peer_port));
    /*send and recv*/
    while(1){
        // string input="";
        // cin>>input;
		bzero(buffer, BUFFER);
		fgets(buffer, BUFFER, stdin);
        string input=buffer;
        if(input.substr(0,5) == "login"){
            fflush(stdout);
            input+=' '+peer_ip+' '+to_string(peer_port);
        }
        split_command(input);
        //string cmd='#'+cmd_list[0]+'#';
        string cmd = parse_cmd_list();
        //cout<<cmd;
        fflush(stdout);
        write(skt,cmd.c_str(),cmd.size());

        if(cmd_list[0] == "create_user"){
            // string user_pass='#'+cmd_list[1]+'#'+cmd_list[2]+'#';
            // write(skt,user_pass.c_str(),user_pass.size());
            memset(buffer,'\0',BUFFER);
            read(skt,buffer,BUFFER);
            msg=buffer;
            cout<<msg<<endl;
            cmd_list.clear();
        }
        else if(cmd_list[0] == "login"){
            //string uauth_nd_ip_port='#'+cmd_list[1]+'#'+cmd_list[2]+'#'+peer_ip+'#'+to_string(peer_port)+'#';
            //write(skt,uauth_nd_ip_port.c_str(),uauth_nd_ip_port.size());
            bzero(buffer, BUFFER);
            read(skt,buffer,BUFFER);
            msg=buffer;
            cout<<msg<<endl;
            cmd_list.clear();
        }
        else if(cmd_list[0] == "file_upload"){
            int read_count,source;
            string source_path="./AOS_Assignment3.pdf";
            source = open(source_path.c_str(), O_RDONLY);

            while((read_count = read(source,buffer,BUFFER))>0){
                write(skt,buffer,read_count);
            }
        }
        else if(cmd_list[0] == "file_download"){
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
        else if(cmd_list[0] == "client"){
            struct sockaddr_in peer;
            char peer_buffer[BUFFER];

            bzero(peer_buffer,BUFFER);
            read(skt,peer_buffer,BUFFER);
            parse_buffer(peer_buffer);
            
            peer.sin_port=htons(toInt(cmd_list_buffer[1]));
            peer.sin_family=AF_INET;
            peer.sin_addr.s_addr=inet_addr(cmd_list_buffer[0].c_str());
            memset(buffer,'\0',BUFFER);

            int peer_skt=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
            if(peer_skt == -1){
                cout<<"Error while socket creation"<<endl;
                exit(0);
            }
            setsockopt(peer_skt,SOL_SOCKET,SO_REUSEADDR,&status,sizeof(int));
            if(connect(peer_skt,(struct sockaddr*)&peer,sizeof(peer)) == -1){
                perror(msg.c_str());
                cout<<"Error while connect syscall"<<endl;
                exit(0);
            }
            string peer_msg;
            cout<<"Start conversation with your peer -----------"<<endl;
            while(1){
                bzero(peer_buffer, BUFFER);
		        fgets(peer_buffer, BUFFER, stdin);
                peer_msg=peer_buffer;

                if(peer_msg == "exit") break;

                write(peer_skt,peer_msg.c_str(),peer_msg.size());

                bzero(peer_buffer, BUFFER);
                read(peer_skt,&peer_buffer,BUFFER);

                peer_msg=peer_buffer;
                cout<<peer_msg<<endl;
                fflush(stdout);
            }        
            close(peer_skt);
        }
        else if(cmd_list[0] == "server"){
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
        server_addr.sin_addr.s_addr = inet_addr(peer_ip.c_str());
        server_addr.sin_port = htons(peer_port);

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
        cout<<"Welcome, Peer is online -----------------"<<endl;
        unsigned int client_addr_length = sizeof(client_addr);
        while(1){
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_length);
            pthread_t pid;
            pthread_create(&pid,NULL,handle_client,(void*)&client_socket);
            //if(pid==0) handle_client(client_socket);
        }

        close(server_socket);
        }
    }
    close(skt);
    return 0;
}