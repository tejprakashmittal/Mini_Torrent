#include<iostream>
#include<cmath>
#include<string.h>
#include<vector>
#include <fcntl.h>
#include<sstream>
#include<sys/socket.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<unistd.h>
#define BUFFER 1024
#define QUEUE 10
using namespace std;

int peer_port;
int status =1;
string peer_ip;
string user_id;
string msg="";
bool logged_in=false;

//char buffer[BUFFER];
pthread_mutex_t lock;
vector<string> cmd_list;

vector<string> parse_buffer(char buffer[]);

struct _download_it{
    string ip,port,dest_file_path,file_name;
    int start_chunk_index,end_chunk_index;
};  //_args

struct _merge_it{
    string dest_file_path,file_name;
    int start_chunk_index,end_chunk_index;
};

string getFileName(string filepath){
  int pos = filepath.find_last_of('/');
    if(pos!=string::npos){
         return filepath.substr(pos+1);
    }
    return filepath;
}

void* download_it(void* args){
    pthread_mutex_lock(&lock);
    vector<string> cmd_list_buffer;
    vector<string> thread_args;
    int s_sock_fd;
    struct sockaddr_in s_serv;
    char peer_buffer[BUFFER];

    struct _download_it *temp_struct = (struct _download_it *)args; 

    s_serv.sin_port=htons(stoi(temp_struct->port));
    s_serv.sin_family=AF_INET;
    s_serv.sin_addr.s_addr=inet_addr(temp_struct->ip.c_str());
    
    s_sock_fd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s_sock_fd == -1){
        cout<<"Error while socket creation"<<endl;
        
    }

    setsockopt(s_sock_fd,SOL_SOCKET,SO_REUSEADDR,&status,sizeof(int));

    if(connect(s_sock_fd,(struct sockaddr*)&s_serv,sizeof(s_serv)) == -1){
        perror(msg.c_str());
        cout<<"Error while connect syscall"<<endl;
        pthread_exit(NULL);
    }

    int start_chunk = temp_struct->start_chunk_index;
    int end_chunk = temp_struct->end_chunk_index;
    FILE *fp;
    while(start_chunk <= end_chunk)
        {
            string file_name_with_chunk = '#'+ temp_struct->file_name + '#' + to_string(start_chunk)+'#';
            write(s_sock_fd,file_name_with_chunk.c_str(),file_name_with_chunk.size());
            
            string dest = temp_struct->dest_file_path + to_string(start_chunk)+".dat";
            fp = fopen(dest.c_str(),"w");
            bzero(peer_buffer,BUFFER);

            int cnts = 512;
            while(cnts > 0)
            {
                int read_count = read(s_sock_fd,peer_buffer,BUFFER);
                //cout<<"Read_Count : "<<read_count<<endl;
                fwrite(peer_buffer,sizeof(char),read_count,fp);
                cout<<read_count<<endl;
                if(read_count < BUFFER) break;
                cnts--;
            }
            // int dest,read_count;
            // dest = open(temp_struct->dest_file_path.c_str(), O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
                    
            // while((read_count = read(s_sock_fd,peer_buffer,BUFFER))>0){
            //     write(dest,peer_buffer,read_count);
            start_chunk++;
            fclose(fp);
        }
    string exit_cmd = "#exit#";
    write(s_sock_fd,exit_cmd.c_str(),exit_cmd.size());
    bzero(peer_buffer,BUFFER);
    read(s_sock_fd,peer_buffer,BUFFER);
    cmd_list_buffer = parse_buffer(peer_buffer);
                //cout<<cmd_list_buffer[0]<<endl;
    if(cmd_list_buffer[0] == "close"){
        shutdown(s_sock_fd,SHUT_RDWR);
        close(s_sock_fd);
    }
    else{
        sleep(1);
        cout<<"Lost"<<endl;
        //close(s_sock_fd);
    }
    pthread_mutex_unlock(&lock);
    return NULL;
    //pthread_exit(NULL);
}

void* merge_it(void* args){
    // FILE *fp;
    // struct _merge_it *temp_struct = (struct _merge_it *)args; 
    // string filepath = temp_struct->dest_file_path + temp_struct->file_name;
    // fp = fopen(filepath.c_str(), "w");
    // //for(int i=0;i<temp_struct.)
    // fclose(fp);
}

void split_command(string cmd_str){
  cmd_list.clear();
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

vector<string> parse_buffer(char buffer[]){
    vector<string> cmd_list_buffer;
    string cmd="";
    for(int i=1;i<BUFFER && buffer[i]!='\0';i++){
        if(buffer[i]=='#'){
            cmd_list_buffer.push_back(cmd);
            cmd="";
        }
        else
        cmd.push_back(buffer[i]);
    }
    return cmd_list_buffer;
}

int chunkCount(string filepath){
    FILE *fp;
    fp = fopen(filepath.c_str(),"r");
    fseek(fp,0,SEEK_END);
    double fsize = ftell(fp);
    int chunks = ceil(fsize/524288);
    fclose(fp);
    return chunks;
}

bool validate_command(){
    if(cmd_list[0] == "create_user" && cmd_list.size() == 3){
        return true;
    }
    else if(cmd_list[0] == "login" && cmd_list.size() == 5){
        return true;
    }
    else if(cmd_list[0] == "create_group" && cmd_list.size() == 3){
        return true;
    }
    else if(cmd_list[0] == "join_group" && cmd_list.size() == 3){
        return true;
    }
    else if(cmd_list[0] == "leave_group" && cmd_list.size() == 3){
        return true;
    }
    else if(cmd_list[0] == "accept_request" && cmd_list.size() == 4){
        return true;
    }
    else if(cmd_list[0] == "list_requests" && cmd_list.size() == 3){
        return true;
    }
    else if(cmd_list[0] == "list_groups" && cmd_list.size() == 1){
        return true;
    }
    else if(cmd_list[0] == "exit" or cmd_list[0] == "list_files" or cmd_list[0] == "upload_file" or cmd_list[0] == "download_file"){
        return true;
    }
    else{
        return false;
    }
}

void* handle_client(void *args){
    /*send and recv*/
    int client_socket=*((int*)args);
    char buffer[BUFFER];
    vector<string> cmd_list_buffer;
    
    while(1)
    {
        cmd_list_buffer.clear();
        bzero(buffer,BUFFER);
        read(client_socket,buffer,BUFFER);
        cmd_list_buffer = parse_buffer(buffer);

        if(cmd_list_buffer[0] == "exit"){
            msg = "#close#";
            write(client_socket,msg.c_str(),msg.size());
            shutdown(client_socket,SHUT_RDWR);
            close(client_socket);
            pthread_exit(NULL);
            //shutdown(client_socket,SHUT_RDWR);
        }

        string file_name=cmd_list_buffer[0];
        int chunk_no = atoi(cmd_list_buffer[1].c_str());
        cout<<"chunk no : "<<chunk_no<<endl;
        fflush(stdout);
        string source_path="./"+file_name;

        FILE *fp;
        fp = fopen(source_path.c_str(),"r");
        fseek(fp,0,SEEK_SET);
        fseek(fp,0,SEEK_END);
        double fsize = ftell(fp);
        int total_chunks = ceil(fsize/524288);

        fseek(fp,chunk_no*524288,SEEK_SET);
        bzero(buffer,BUFFER);
        if(chunk_no < total_chunks-1){
            int cnts = 512;
            while(cnts > 0){
                fread(buffer, sizeof(char), BUFFER, fp);
                write(client_socket,buffer,BUFFER);
                cnts--;
            }
        }
        else{
            fseek(fp,(total_chunks-1)*524288,SEEK_SET);
            int buf_size = fsize - (total_chunks-1)*524288;
            //double _buf_size = buf_size;
            int cnts = buf_size/BUFFER;
            int rem = buf_size - cnts*BUFFER;
            while(cnts > 0){
                fread(buffer, sizeof(char), BUFFER, fp);
                write(client_socket,buffer,BUFFER);
                cnts--;
            }
            if(rem > 0){
                fread(buffer, sizeof(char), rem, fp);
                write(client_socket,buffer,rem);
            }
            cout<<"Last Chunk"<<endl;
            fflush(stdout);
        }
        fclose(fp);
    }

    // int read_count,source;

    // source = open(source_path.c_str(), O_RDONLY);

    // while((read_count = read(source,buffer,BUFFER))>0){
    //     write(client_socket,buffer,read_count);
    // }
    /*close file and socket*/
    close(client_socket);
    pthread_exit(NULL);
}

int init_client_mode(){
    char buffer[BUFFER];
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    server_addr.sin_port=htons(9900);

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

    setsockopt(skt,SOL_SOCKET,SO_REUSEADDR,&status,sizeof(int));

    if(connect(skt,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
        cout<<"Error while connect syscall"<<endl;
        exit(0);
    }
    /*connect*/
    cout<<"Connected to the server----------"<<endl;
    cout<<"type command below"<<endl;
    return skt;
}

void* init_server_mode(void* args){
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

int main(int argc,char *argv[]){
    vector<string> cmd_list_buffer;
    char buffer[BUFFER];
    peer_ip=argv[1];
    peer_port=atoi(argv[2]);
    
    int skt = init_client_mode();
    pthread_t t_serv_id;
    pthread_create(&t_serv_id,NULL,init_server_mode,NULL);

    //write(skt,&peer_port,sizeof(peer_port));
    /*send and recv*/
    while(1){
        // string input="";
        // cin>>input;
		bzero(buffer, BUFFER);
		fgets(buffer, BUFFER, stdin);
        string input=buffer;
        if(input.substr(0,5) == "login"){
            //fflush(stdout);
            input+=' '+peer_ip+' '+to_string(peer_port);
        }
        else if(input.substr(0,11) == "upload_file"){
            input+=' '+peer_ip+' '+to_string(peer_port);
        }
        else if(input.substr(0,12) == "create_group"){
            input+=' '+user_id;
        }
        else if(input.substr(0,10) == "join_group"){
            input+=' '+user_id;
        }
        else if(input.substr(0,14) == "accept_request"){
            input+=' '+user_id;
        }
        else if(input.substr(0,13) == "list_requests"){
            input+=' '+user_id;
        }
        else if(input.substr(0,11) == "leave_group"){
            input+=' '+user_id;
        }

        split_command(input);
        //string cmd='#'+cmd_list[0]+'#';
        string cmd = parse_cmd_list();
        //cout<<cmd;
        fflush(stdout);

        if(validate_command()){

            if(cmd_list[0] == "upload_file"){
                int chunks = chunkCount(cmd_list[1]);
                cmd+=to_string(chunks)+'#';
            }

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
                logged_in = true;
                user_id = cmd_list[1];
                cmd_list.clear();
            }
            else if(cmd_list[0] == "create_group"){

            }
            else if(cmd_list[0] == "join_group"){
                cout<<"---Request sent---"<<endl;
                fflush(stdout);
            }
            else if(cmd_list[0] == "leave_group"){

            }
            else if(cmd_list[0] == "accept_request"){

            }
            else if(cmd_list[0] == "list_requests"){
                cout<<"-----------------------------------------------------------------------------"<<endl;
                bzero(buffer,BUFFER);
                read(skt,buffer,BUFFER);
                cmd_list_buffer = parse_buffer(buffer);
                if(cmd_list_buffer.size() > 0){
                    for(auto itr:cmd_list_buffer)
                    {
                        cout<<itr<<" ";
                    }
                    fflush(stdout);
                    cout<<endl;
                }
                else{
                    cout<<"-----No pending requests-------"<<endl;
                    fflush(stdout);
                }
            }
            else if(cmd_list[0] == "list_groups"){
                cout<<"-----------------------------------------------------------------------------"<<endl;
                bzero(buffer, BUFFER);
                read(skt,buffer,BUFFER);
                cmd_list_buffer = parse_buffer(buffer);

                if(cmd_list_buffer.size() > 0){
                    for(auto itr:cmd_list_buffer){
                        cout<<itr<<" ";
                    }
                    cout<<endl;
                }
                fflush(stdout);
            }
            else if(cmd_list[0] == "my_groups"){
                cout<<"-----------------------------------------------------------------------------"<<endl;
                bzero(buffer,BUFFER);
                read(skt,buffer,BUFFER);
                cmd_list_buffer = parse_buffer(buffer);

                if(cmd_list_buffer.size() > 0){
                    for(auto itr:cmd_list_buffer){
                        cout<<itr<<" ";
                    }
                }
                else cout<<"---You are not the member of any group---";
                cout<<endl;
                fflush(stdout);
            }
            else if(cmd_list[0] == "upload_file"){
                bzero(buffer,BUFFER);
                read(skt,buffer,BUFFER);
                cout<<buffer<<endl;
                fflush(stdout);
            }
            else if(cmd_list[0] == "list_files"){
                cout<<"-----------------------------------------------------------------------------"<<endl;
                if(cmd_list.size() >= 2){
                    bzero(buffer,BUFFER);
                    read(skt,buffer,BUFFER);
                    cmd_list_buffer = parse_buffer(buffer);

                    if(cmd_list_buffer.size() > 0){
                        for(auto itr:cmd_list_buffer){
                            cout<<itr<<" ";
                        }
                    }
                    else cout<<"---No files under requested group---";
                }
                else{
                    cout<<"---Invalid Command---";
                }
                cout<<endl;
                fflush(stdout);
            }
            else if(cmd_list[0] == "download_file"){
                if(cmd_list.size() >= 4){
                    char buff[BUFFER];
                    bzero(buff,BUFFER);
                    read(skt,buff,BUFFER);
                    cmd_list_buffer = parse_buffer(buff);
                    if(cmd_list_buffer.size() > 0){
                        // for(auto itr:cmd_list_buffer){
                        //     cout<<itr<<" ";
                        // }
                        //cout<<cmd_list_buffer.size()<<endl;
                        //fflush(stdout);
                        double chunk_count = stoi(cmd_list_buffer[cmd_list_buffer.size()-1]);
                        int peers_count = (cmd_list_buffer.size()-1)/2;
                        cout<<"Number of peers -> "<<peers_count<<endl;
                        vector<int> chunk_map(chunk_count,0);

                        int chunk_per_peer = ceil(chunk_count/peers_count);

                        int i=0,j=0,k=0;
                        struct _download_it _args[peers_count];
                        pthread_t target_tid[50];
                        while(i < chunk_count && j < cmd_list_buffer.size()-2){
                            if (pthread_mutex_init(&lock, NULL) != 0) {
                                printf("\n mutex init has failed\n");
                            }
                            cout<<"This is value of i : "<<i<<endl;

                            _args[k].dest_file_path = cmd_list[3];
                            _args[k].file_name = cmd_list[2];
                            _args[k].ip = cmd_list_buffer[j];
                            _args[k].port = cmd_list_buffer[++j];
                            _args[k].start_chunk_index = i;
                            _args[k].end_chunk_index = i + chunk_per_peer - 1;

                            while(_args[k].end_chunk_index >= chunk_count) _args[k].end_chunk_index--;

                            pthread_create(&target_tid[j], NULL, download_it, (void*)&_args[k]);
                            //pthread_join(target_tid[j],NULL);
                            // pid_t pid = fork();
                            // if(pid == 0) download_it((void*)&_args);
                            // wait(NULL);
                            i = i + chunk_per_peer;
                            j++,k++;
                        }
                        pthread_join(target_tid[j-1],NULL);
                        pthread_mutex_destroy(&lock);
                        struct _merge_it ptr;
                        ptr.file_name = cmd_list[2];
                        ptr.dest_file_path = cmd_list[3];
                        ptr.start_chunk_index = 0;
                        ptr.end_chunk_index = chunk_count -1;
                        pthread_create(&target_tid[j], NULL, merge_it, (void*)&ptr);
                    }
                    else cout<<"---File is not available to download---";
                }
                else cout<<"---Invalid Command---";
                cout<<endl;
                fflush(stdout);
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
                cmd_list_buffer = parse_buffer(peer_buffer);
                
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
            else if(cmd_list[0] == "exit"){
                bzero(buffer,BUFFER);
                read(skt,buffer,BUFFER);
                cmd_list_buffer = parse_buffer(buffer);
                //cout<<cmd_list_buffer[0]<<endl;
                if(cmd_list_buffer[0] == "close"){
                    shutdown(skt,SHUT_RDWR);
                    close(skt);
                }
            }
            else{
                cout<<"Invalid Command"<<endl;
            }
        }
        else cout<<"Invalid Command"<<endl;
        fflush(stdout);
    }
    close(skt);
    return 0;
}