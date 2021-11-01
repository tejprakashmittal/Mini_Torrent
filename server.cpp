#include<unordered_map>
#include<vector>
#include<iostream>
#include<string.h>
#include <fcntl.h>
#include<pthread.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#define QUEUE 2
#define BUFFER 1024
using namespace std;

//unordered_map<short,sockaddr_in> ip_data;
vector<sockaddr_in> client_data;
vector<string> cmd_list;
unordered_map<string,string> uid_ip_port;
unordered_map<string,string> users_cred;

struct arg_struct {
    int socket_fd;
    struct sockaddr_in client_addr;
}args;

void parse_buffer(char buffer[]){
    cmd_list.clear();
    string cmd="";
    for(int i=1;i<BUFFER && buffer[i]!='\0';i++){
        if(buffer[i]=='#'){
            cmd_list.push_back(cmd);
            cmd="";
        }
        else
        cmd.push_back(buffer[i]);
    }
}

bool verify(string uname,string pass){
    if(users_cred.find(uname) != users_cred.end()){
        if(users_cred[uname] == pass) return true;
    }
    return false;
}

void *handle_client(void *args){
    /*send and recv*/
    struct arg_struct *arg=(struct arg_struct *)args;
    //ip_data[ntohs(arg->client_addr.sin_port)] = arg->client_addr;
    client_data.push_back(arg->client_addr);
    int client_socket = arg->socket_fd;
    int input,a,b,r=5;
    char buffer[BUFFER];
    string msg="";
    while(1){
        //memset(buffer,'\0',BUFFER);
        bzero(buffer,BUFFER);
        read(client_socket,buffer,BUFFER);
        if(buffer[0]!='#') {
            cout<<buffer<<endl;
            cout<<"Invalid Input";
            fflush(stdout);
            exit(0);
        }
        parse_buffer(buffer);
        fflush(stdout);
        if(cmd_list[0] == "create_user"){
            //memset(buffer,'\0',BUFFER);
            //sleep(1);
            //read(client_socket,&buffer,BUFFER);
            //parse_buffer(buffer);
            users_cred[cmd_list[1]] = cmd_list[2];
            cout<<"User created : "<<cmd_list[1];
            fflush(stdout);
            string resp="User_Created_Successfully";
            //cout<<users_cred[cmd_list[1]];
            //fflush(stdout);
            write(client_socket,resp.c_str(),resp.size());
            cmd_list.clear();
        }
        else if(cmd_list[0] == "login"){
            // cmd_list.clear();
            // memset(buffer,'\0',BUFFER);
            // read(client_socket,buffer,BUFFER);
            // parse_buffer(buffer);
            if(verify(cmd_list[1],cmd_list[2])){
                uid_ip_port[cmd_list[1]] = '#'+cmd_list[3]+'#'+cmd_list[4]+'#';
                cout<<"User logged in : "<<cmd_list[1];
                fflush(stdout);
                msg="Login Successful";
                write(client_socket,msg.c_str(),msg.size());
            }
            cmd_list.clear();       
        }
        else if(input == 3){
            int dest,read_count;
            string dest_full_path="./AOS_Assignment3.pdf";
            dest = open(dest_full_path.c_str(), O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
            
            while((read_count = read(client_socket,buffer,BUFFER))>0){
                write(dest,buffer,read_count);
            }
        }
        else if(input == 4){
            msg="Enter file name - ";
            write(client_socket,msg.c_str(),msg.size());
            
            read(client_socket,buffer,BUFFER);
            string file_name=buffer;
            string source_path="./"+file_name;
            int read_count,source;

            source = open(source_path.c_str(), O_RDONLY);

            while((read_count = read(source,buffer,BUFFER))>0){
                write(client_socket,buffer,read_count);
            }
        }
        else if(cmd_list[0] == "client"){
            write(client_socket,uid_ip_port[cmd_list[1]].c_str(),uid_ip_port[cmd_list[1]].size());
        }
    }
    /*close socket*/
    close(client_socket);
    pthread_exit(NULL);
}

void handle_client(int client_socket){
    /*send and recv*/
    char buffer[BUFFER];
    string msg="";
    char ch;
    while(1){
        if(recv(client_socket,buffer,BUFFER,0) == -1) cout<<"Error while receiving msg"<<endl;  
        //for(int i=0;buffer[i]!='\0';i++) msg.push_back(buffer[i]);
        //memset(buffer,'\0',sizeof(buffer[0]));
        // cout<<msg<<endl;
        // fflush(stdout);
        msg.clear();
        if(send(client_socket,&buffer,BUFFER,0) == -1) cout<<"Error while sending msg"<<endl;
        memset(buffer,'\0',BUFFER);    
    }
    /*close socket*/
    close(client_socket);
    pthread_exit(NULL);
}

int main(){
    int server_socket,client_socket;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    /*Define server socket*/
    server_socket=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    int status = 1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&status,sizeof(int));
    if(server_socket == -1){
        cout<<"Error while socket creation"<<endl;
        exit(0);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(9900);

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

    /*Accept in loop*/
    unsigned int client_addr_length = sizeof(client_addr);
    while(1){
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_length);
        // pid_t pid=fork();
        // if(pid == 0) handle_client(client_socket);
            pthread_t tid;
            args.client_addr=server_addr;
            args.socket_fd=client_socket;
            pthread_create(&tid, NULL, handle_client, (void *)&args);
            //pthread_join(tid[i], NULL);
            //pthread_exit(NULL);
    }
    close(server_socket);
    /*call handle_cleint for each client*/
}