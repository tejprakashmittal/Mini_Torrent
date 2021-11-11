#include<iostream>
#include <stdlib.h>
#include<unordered_map>
#include<openssl/sha.h>
#include<cmath>
#include<string.h>
#include <fstream> 
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

int status =1;
int peer_port;
int tracker_port;
string peer_ip;
string user_id;
string tracker_ip;
string msg="";
string error_msg="";
string calculated_hash="";
bool logged_in=false;

//char buffer[BUFFER];
pthread_mutex_t lock,lock2,lock3;
vector<string> cmd_list;
unordered_map<string,vector<int>> file_chunk_count;
vector<string> parse_buffer(char buffer[]);
vector<pair<string,string>> download_history;
unordered_map<string, string> FilePathMap; // filename -> filepath

struct _download_it{
    string ip,port,dest_file_path,file_name;
    int start_chunk_index,end_chunk_index;
};  //_args

struct _merge_it{
    string dest_file_path,file_name,sha_hash;
    int start_chunk_index,end_chunk_index,chunk_count;
};

vector<string> splitArgs(string str){
	vector<string> ipPort;
    string ip="",port="";
	for(int i=0;i<str.size();i++){
        if(str[i] == ':'){
            ipPort.push_back(ip);
            port = str.substr(i+1,str.size()-i-1);
            ipPort.push_back(port);
        }
        ip.push_back(str[i]);
    }
    return ipPort;
}

vector<string> split_file_args(string filepath){
    FILE *fptr;
    fptr = fopen(filepath.c_str(),"r");
    char buff[500];
    while(!feof(fptr)){
        fread(buff,sizeof(char),500,fptr);
    }
    string str = buff;
    return splitArgs(str);
}

unsigned long long int find_size(string filepath){
    FILE *fp;
    fp = fopen(filepath.c_str(),"r");
    fseek(fp,0,SEEK_SET);
    return ftell(fp);
}

string getFileName(string filepath){
  int pos = filepath.find_last_of('/');
    if(pos!=string::npos){
         return filepath.substr(pos+1);
    }
    return filepath;
}

string& getFile(string filepath) {
	string *s = new string;
	s->reserve(1024);
	fstream fp;
        
	fp.open(filepath.c_str(),std::ios::in);
	if(!(fp.is_open())){
		fprintf(stderr,"Unable to open the file\n");
		//exit(EXIT_FAILURE);
	}
	else {
		string line;
		while(fp >> line){
			s->append(line);
		}
	}
	fp.close();
	return *s;
}

string getSha(string filepath){
	string *bufer = &(getFile(filepath));

	//Genrating hash of the file
	array<unsigned char,SHA_DIGEST_LENGTH> digest;
	SHA_CTX ctx;
	//Initializing
	SHA1_Init(&ctx);
	SHA1_Update(&ctx,bufer->c_str(),bufer->size());
	SHA1_Final(digest.data(),&ctx);

	delete bufer;

	array<char,SHA_DIGEST_LENGTH * 2 +1> mdString;
	for(int i = 0 ; i < SHA_DIGEST_LENGTH ; ++i) {
		sprintf(&(mdString[i*2]),"%02x",(unsigned int)digest[i]);
	}
	//fprintf(stdout,"\n\nHash of the file: %s\n" ,mdString.data());
	string sha_val;

	for(int i=0;i<40;i++){
		sha_val.push_back(mdString[i]);
	}
    //cout<<endl<<sha_val.size();
	return sha_val;
}

string get_bitmap(string ip,string port,string filename){
    /*Make the connetion to the peer of ip and port
    send success message along with filename
    receive the bitmap string in buffer
    parse the buffer int the vector
    return parsed chunk-sized bitmap string
    */
    int s_sock_fd;
    char buff[BUFFER];
    struct sockaddr_in s_serv;
    vector<string> cmd_list_buffer;
    s_serv.sin_port=htons(stoi(port));
    s_serv.sin_family=AF_INET;
    s_serv.sin_addr.s_addr=inet_addr(ip.c_str());
    
    s_sock_fd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s_sock_fd == -1){
        cout<<"Error while socket creation"<<endl; 
    }
    setsockopt(s_sock_fd,SOL_SOCKET,SO_REUSEADDR,&status,sizeof(int));
    if(connect(s_sock_fd,(struct sockaddr*)&s_serv,sizeof(s_serv)) == -1){
        perror(msg.c_str());
        cout<<"Error while connect syscall"<<endl;
        return "";
    }
    string temp_msg="#bitmap#" + filename +'#';
    write(s_sock_fd,temp_msg.c_str(),temp_msg.size());
    read(s_sock_fd,buff,BUFFER);
    cmd_list_buffer = parse_buffer(buff);
    //cout<<cmd_list_buffer[0]<<endl;
    return cmd_list_buffer[0];
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
        fflush(stdout);
        pthread_exit(NULL);
    }
/*
send download message and receive success message then start downlaoding the required chunk
handlclient_method also needs to be modified to donload only required chunk
*/
    string temp_msg="#download#";
    write(s_sock_fd,temp_msg.c_str(),temp_msg.size());
    bzero(peer_buffer,BUFFER);
    read(s_sock_fd,peer_buffer,BUFFER);
    cmd_list_buffer.clear();
    cmd_list_buffer = parse_buffer(peer_buffer);
    fflush(stdout);
    //cout<<"cmd list size "<<cmd_list_buffer.size()<<endl;
    if(cmd_list_buffer[0] != "success"){
        cout<<"Error while downloading chunk"<<endl;
        fflush(stdout);
        close(s_sock_fd);
        pthread_exit(NULL);
    }
    cmd_list_buffer.clear();

    int start_chunk = temp_struct->start_chunk_index;
    FILE *fp;

            string file_name_with_chunk = '#'+ temp_struct->file_name + '#' + to_string(start_chunk)+'#';
            write(s_sock_fd,file_name_with_chunk.c_str(),file_name_with_chunk.size());
            
            string dest = temp_struct->dest_file_path + temp_struct->file_name + to_string(start_chunk)+".dat";
            fp = fopen(dest.c_str(),"w");
            bzero(peer_buffer,BUFFER);

            int cnts = 512;
            while(cnts > 0)
            {
                int read_count = read(s_sock_fd,peer_buffer,BUFFER);
                //cout<<"Read_Count : "<<read_count<<endl;
                // while(start_chunk != temp_struct->end_chunk_index && read_count < BUFFER)
                // int read_count = read(s_sock_fd,peer_buffer,BUFFER);
                fwrite(peer_buffer,sizeof(char),read_count,fp);
                if(start_chunk != temp_struct->end_chunk_index && read_count < BUFFER){
                    int read_count = read(s_sock_fd,peer_buffer,BUFFER);
                    fwrite(peer_buffer,sizeof(char),read_count,fp);
                }
                //cout<<read_count<<endl;
                //if(read_count < BUFFER) break;
                cnts--;
            }
            // int dest,read_count;
            // dest = open(temp_struct->dest_file_path.c_str(), O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
                    
            // while((read_count = read(s_sock_fd,peer_buffer,BUFFER))>0){
            //     write(dest,peer_buffer,read_count);
            //start_chunk++;
            fclose(fp);
        
        
    // string exit_cmd = "#exit#";
    // write(s_sock_fd,exit_cmd.c_str(),exit_cmd.size());
    // bzero(peer_buffer,BUFFER);
    // read(s_sock_fd,peer_buffer,BUFFER);
    // cmd_list_buffer = parse_buffer(peer_buffer);
    //             //cout<<cmd_list_buffer[0]<<endl;
    // if(cmd_list_buffer[0] == "close"){
    //     shutdown(s_sock_fd,SHUT_RDWR);
    //     close(s_sock_fd);
    // }
    // else{
    //     sleep(1);
    //     cout<<"Lost"<<endl;
    //     //close(s_sock_fd);
    // }
    pthread_mutex_unlock(&lock);
    //return NULL;
    close(s_sock_fd);
    pthread_exit(NULL);
}

void* merge_it(void* args){
    FILE *fp,*_file;
    struct _merge_it *temp_struct = (struct _merge_it *)args; 
    string filepath = temp_struct->dest_file_path + temp_struct->file_name;
    fflush(stdout);
    fp = fopen(filepath.c_str(), "w");
    for(int i=0;i<temp_struct->chunk_count;i++){
        string fpath = temp_struct->dest_file_path + temp_struct->file_name + to_string(i) +".dat";
        _file = fopen(fpath.c_str(),"r");
        fseek(_file,0,SEEK_END);
        int chunk_size = ftell(_file);
        fseek(_file,0,SEEK_SET);
        char buff[chunk_size];
        bzero(buff,chunk_size);
        fread(buff,sizeof(char),chunk_size,_file);
        fwrite(buff,sizeof(char),chunk_size,fp);
        // if(remove(fpath.c_str()) != 0){
        //     cout<<"Error while deleting the chunk"<<endl;
        //     fflush(stdout);
        // }
        fclose(_file);
    }
    fclose(fp);
    sleep(1);
    string sha_hashed = getSha(filepath);
    if(sha_hashed == calculated_hash){
        cout<<"---Downloaded successfully---"<<endl;
        fflush(stdout);
        for(int i=0;i<temp_struct->chunk_count;i++){
            string fpath = temp_struct->dest_file_path + temp_struct->file_name + to_string(i) +".dat";
            if(remove(fpath.c_str()) != 0){
                cout<<"Error while deleting the chunk"<<endl;
                fflush(stdout);
            }
        }
    }
    //cout<<sha_hashed<<endl<<calculated_hash<<endl;
    calculated_hash="";
    return NULL;
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
    pthread_mutex_lock(&lock2);
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
    pthread_mutex_unlock(&lock2);
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
    else if(cmd_list[0] == "logout" && cmd_list.size() == 3){
        if(logged_in == false){
            error_msg = "---Please log in first---";
            return false;
        }
        return true;
    }
    else if(cmd_list[0] == "create_group" && cmd_list.size() == 3){
        if(logged_in == false){
            error_msg = "---Please log in first---";
            return false;
        }
        return true;
    }
    else if(cmd_list[0] == "join_group" && cmd_list.size() == 3){
        if(logged_in == false){
            error_msg = "---Please log in first---";
            return false;
        }
        return true;
    }
    else if(cmd_list[0] == "leave_group" && cmd_list.size() == 3){
        if(logged_in == false){
            error_msg = "---Please log in first---";
            return false;
        }
        return true;
    }
    else if(cmd_list[0] == "accept_request" && cmd_list.size() == 4){
        if(logged_in == false){
            error_msg = "---Please log in first---";
            return false;
        }
        return true;
    }
    else if(cmd_list[0] == "list_requests" && cmd_list.size() == 3){
        if(logged_in == false){
            error_msg = "---Please log in first---";
            return false;
        }
        return true;
    }
    else if(cmd_list[0] == "list_groups" && cmd_list.size() == 1){
        if(logged_in == false){
            error_msg = "---Please log in first---";
            return false;
        }
        return true;
    }
    else if(cmd_list[0] == "exit" or cmd_list[0] == "stop_share" or cmd_list[0] == "show_downloads" or cmd_list[0] == "my_groups" or cmd_list[0] == "list_files" or cmd_list[0] == "upload_file" or cmd_list[0] == "download_file"){
        return true;
    }
    else{
        return false;
    }
}

void* handle_client(void *args){
    /*send and recv*/
    //pthread_mutex_lock(&lock3);
    int client_socket=*((int*)args);
    char buffer[BUFFER];
    vector<string> cmd_list_buffer;
    vector<string> selection;
    bzero(buffer,BUFFER);
    read(client_socket,buffer,BUFFER);
    //cout<<"server_side_biffer1 "<<buffer<<endl;
    selection = parse_buffer(buffer);

    if(selection[0] == "bitmap"){
        string bitmap="#";
        if(file_chunk_count.find(selection[1]) != file_chunk_count.end())
        {
            for(int i=0;i<file_chunk_count[selection[1]].size();i++){
                bitmap += to_string(file_chunk_count[selection[1]][i]);
            }
            bitmap += '#';
        }
        write(client_socket,bitmap.c_str(),bitmap.size());
    }
    else if(selection[0] == "download")
    {
        string temp_msg = "#success#";
        write(client_socket,temp_msg.c_str(),temp_msg.size());
       // while(1)
       // {
            cmd_list_buffer.clear();
            bzero(buffer,BUFFER);
            read(client_socket,buffer,BUFFER);
            // cout<<"server_side_biffer2 "<<buffer<<endl;
            // fflush(stdout);
            cmd_list_buffer = parse_buffer(buffer);
            string file_name = cmd_list_buffer[0];
            int chunk_no = stoi(cmd_list_buffer[1].c_str());
            //cout<<"chunk no. -> "<<chunk_no<<endl;
            //fflush(stdout);

            bool file_check = true;
            // cout<<"File_Check -- "<<file_check<<endl;
            // fflush(stdout);
            for(int i=0;i<file_chunk_count[file_name].size();i++){
                if(file_chunk_count[file_name][i] == 0){
                    file_check == false;
                    break;
                }
            }
            if(file_check == false){
                string source_path = FilePathMap[file_name] + to_string(chunk_no) + ".dat";
                //string source_path="./"+file_name + to_string(chunk_no);
                FILE *fp;
                fp = fopen(source_path.c_str(),"r");
                fseek(fp,0,SEEK_END);
                int chunk_size = ftell(fp);
                fseek(fp,0,SEEK_SET);
                bzero(buffer,BUFFER);

                int cnts = chunk_size/BUFFER;
                int rem = chunk_size - cnts*BUFFER;
                while(cnts > 0){
                    fread(buffer, sizeof(char), BUFFER, fp);
                    write(client_socket,buffer,BUFFER);
                    cnts--;
                }
                if(rem > 0){
                    fread(buffer, sizeof(char), rem, fp);
                    write(client_socket,buffer,rem);
                }
                fclose(fp);
            }
            else{
                string source_path = FilePathMap[file_name];
                //string source_path="./"+file_name;
                FILE *fp;
                fp = fopen(source_path.c_str(),"r");
                //fseek(fp,0,SEEK_SET);
                fseek(fp,0,SEEK_END);
                double fsize = ftell(fp);
                int total_chunks = ceil(fsize/524288);

                fseek(fp,chunk_no*524288,SEEK_SET);
                bzero(buffer,BUFFER);
                if(chunk_no < total_chunks-1){
                    int cnts = 512;
                    while(cnts > 0){
                        size_t cct = fread(buffer, sizeof(char), BUFFER, fp);
                        // cout<<"Chunk no reading "<<chunk_no<<" "<<cct<<endl;
                        // if (ferror (fp))
                        // printf ("Error Writing to myfile.txt\n");
                        // cout<<"socket "<<client_socket<<endl;
                        //fflush(stdout);

                       ssize_t cct2 = write(client_socket,buffer,BUFFER);
                    //    while(cct2 < BUFFER)
                    //    cct2 = write(client_socket,buffer,BUFFER);
                        //cout<<"Chunk no writing "<<chunk_no<<" "<<cct2<<endl;
                        cnts--;
                    }
                }
                else{
                    fseek(fp,(total_chunks-1)*524288,SEEK_SET);
                    int buf_size = fsize - (total_chunks-1)*524288;
                    //double _buf_size = buf_size;
                    int cnts = buf_size/BUFFER;
                    int rem = buf_size - cnts*BUFFER;
                    //cout<<"Chunk No "<<chunk_no<<" "<<"size -- "<<buf_size<<" "<<"remaining_bytes -- "<<rem<<endl;
                    while(cnts > 0){
                        fread(buffer, sizeof(char), BUFFER, fp);
                        write(client_socket,buffer,BUFFER);
                        cnts--;
                    }
                    if(rem > 0){
                        fread(buffer, sizeof(char), rem, fp);
                        write(client_socket,buffer,rem);
                    }
                    //cout<<"Last Chunk"<<endl;
                    //fflush(stdout);
                //}
                }
            fclose(fp);
        }
    }
    // bzero(buffer,BUFFER);
    // read(client_socket,buffer,BUFFER);
    // cmd_list_buffer.clear();
    // cmd_list_buffer = parse_buffer(buffer);
    // if(cmd_list_buffer[0] == "exit"){
    //             msg = "#close#";
    //             write(client_socket,msg.c_str(),msg.size());
    //             shutdown(client_socket,SHUT_RDWR);
    //             close(client_socket);
    //             pthread_exit(NULL);
    //             //shutdown(client_socket,SHUT_RDWR);
    //         }
    
    // int read_count,source;

    // source = open(source_path.c_str(), O_RDONLY);

    // while((read_count = read(source,buffer,BUFFER))>0){
    //     write(client_socket,buffer,read_count);
    // }
    /*close file and socket*/
    //close(client_socket);
    close(client_socket);
    //pthread_mutex_unlock(&lock3);
    pthread_exit(NULL);
}

int init_client_mode(){
    char buffer[BUFFER];
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr(tracker_ip.c_str());
    server_addr.sin_port=htons(tracker_port);

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
    vector<string> ip_port_args = splitArgs(argv[1]);
    if(ip_port_args.size() < 2){
        cout<<"Invalid IP or PORT"<<endl;
        exit(0);
    }
    peer_ip = ip_port_args[0];                               
    peer_port=stoi(ip_port_args[1]);

    ip_port_args.clear();
    ip_port_args = split_file_args(argv[2]);

    if(ip_port_args.size() < 2){
        cout<<"Invalid Tracker IP or PORT"<<endl;
        exit(0);
    }

    tracker_ip = ip_port_args[0];
    tracker_port = stoi(ip_port_args[1]);

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
            input+=' '+peer_ip+' '+to_string(peer_port);
        }
        else if(input.substr(0,6) == "logout"){  
            input+=' '+peer_ip+' '+to_string(peer_port);
        }
        else if(input.substr(0,10) == "stop_share"){
            input+=' '+peer_ip+' '+to_string(peer_port);
        }
        else if(input.substr(0,11) == "upload_file"){
            input+=' '+peer_ip+' '+to_string(peer_port);
        }
        else if(input.substr(0,13) == "download_file"){
            input+=' '+user_id+' '+peer_ip+' '+to_string(peer_port);
        }
        else if(input.substr(0,12) == "create_group"){
            input+=' '+user_id;
        }
        else if(input.substr(0,10) == "join_group"){
            input+=' '+user_id;
        }
        else if(input.substr(0,9) == "my_groups"){
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
        //fflush(stdout);

        if(validate_command()){

            if(cmd_list[0] == "upload_file"){
                int chunks = chunkCount(cmd_list[1]);
                file_chunk_count[getFileName(cmd_list[1])].resize(chunks,1);
                //cout<<"chunks -- "<<chunks<<endl;
                string sha_val = getSha(cmd_list[1]);
                //cout<<sha_val<<endl;
                cmd+=to_string(chunks)+'#'+sha_val+'#';
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
            else if(cmd_list[0] == "logout"){
                bzero(buffer, BUFFER);
                read(skt,buffer,BUFFER);
                msg=buffer;
                cout<<msg<<endl;
                logged_in = false;
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
                vector<string> _cmd_list_buffer;
                bzero(buffer,BUFFER);
                read(skt,buffer,BUFFER);
                _cmd_list_buffer = parse_buffer(buffer);

                if(_cmd_list_buffer.size() > 0){
                    for(auto itr:_cmd_list_buffer){
                        cout<<itr<<" ";
                    }
                }
                else cout<<"---You are not the member of any group---";
                cout<<endl;
                fflush(stdout);
            }
            else if(cmd_list[0] == "upload_file"){
                FilePathMap[getFileName(cmd_list[1])] = cmd_list[1];
                bzero(buffer,BUFFER);
                read(skt,buffer,BUFFER);
                cout<<buffer<<endl;
                fflush(stdout);
            }
            else if(cmd_list[0] == "stop_share"){
                char _buffer[BUFFER];
                bzero(_buffer,BUFFER);
                read(skt,_buffer,BUFFER);
                cout<<_buffer<<endl;
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
                if(cmd_list.size() >= 6){
                    char buff[BUFFER];
                    bzero(buff,BUFFER);
                    read(skt,buff,BUFFER);
                    cmd_list_buffer = parse_buffer(buff);
                    if(cmd_list_buffer.size() > 0){
                        // for(auto itr:cmd_list_buffer){
                        //     cout<<itr<<" ";
                        // }
                        download_history.push_back({cmd_list[1],cmd_list[2]});
                        // cout<<cmd_list_buffer[cmd_list_buffer.size()-2]<<endl;
                        // cout<<cmd_list_buffer[cmd_list_buffer.size()-1]<<endl;
                        // cout<<cmd_list_buffer.size()<<endl;
                        // fflush(stdout);

                        int chunk_count = stoi(cmd_list_buffer[cmd_list_buffer.size()-2]);
                        int peers_count = (cmd_list_buffer.size()-2)/2;
                        //cout<<"Number of peers -> "<<peers_count<<endl;

                        vector<vector<pair<string,string>>> bitmap_collection(chunk_count);
                        for(int i=0;i<cmd_list_buffer.size()-3;i=i+2){
                            string bitmap = get_bitmap(cmd_list_buffer[i],cmd_list_buffer[i+1],cmd_list[2]);
                            if(bitmap.size() != chunk_count){
                                cout<<"bitmap size is less than chunk count"<<endl;
                                fflush(stdout);
                            }
                            for(int j=0;j<bitmap.size();j++){
                                if(bitmap[j] == '1')
                                bitmap_collection[j].push_back({cmd_list_buffer[i],cmd_list_buffer[i+1]});
                            }
                        }
                        //cout<<"bitmap collection size "<<bitmap_collection.size()<<endl;
                        pthread_t target_tid[int(chunk_count)];
                        struct _download_it _args[int(chunk_count)]; 
                        for(int i=0;i<chunk_count;i++){
                            int index = rand() % bitmap_collection[i].size();
                            _args[i].start_chunk_index = i;
                            _args[i].end_chunk_index = chunk_count-1;
                            _args[i].ip = bitmap_collection[i][index].first;
                            _args[i].port = bitmap_collection[i][index].second;
                            _args[i].file_name = cmd_list[2];
                            _args[i].dest_file_path = cmd_list[3];
                            pthread_create(&target_tid[i], NULL, download_it, (void*)&_args[i]);
                            //pthread_join(target_tid[i],NULL);
                        }
                    // cout<<"Join required -- "<<endl;
                    // fflush(stdout);
                    /*
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
                        */
                        //pthread_join(target_tid,NULL);
                        //pthread_mutex_destroy(&lock);
                        struct _merge_it ptr;
                        ptr.file_name = cmd_list[2];
                        ptr.dest_file_path = cmd_list[3];
                        ptr.sha_hash = cmd_list_buffer[cmd_list_buffer.size()-1];
                        //cout<<"ptr->sha "<<ptr.sha_hash<<endl;
                        calculated_hash = cmd_list_buffer[cmd_list_buffer.size()-1];
                        ptr.start_chunk_index = 0;
                        ptr.end_chunk_index = chunk_count -1;
                        ptr.chunk_count = chunk_count;
                        file_chunk_count[cmd_list[2]].resize(chunk_count,0);
                        for(int i=0;i<chunk_count;i++)
                        {
                            pthread_join(target_tid[i],NULL);
                            file_chunk_count[cmd_list[2]][i] = 1;
                            FilePathMap[cmd_list[2]] = cmd_list[3] + cmd_list[2];
                        }
                        pthread_create(&target_tid[chunk_count], NULL, merge_it, (void*)&ptr);
                    }
                    else cout<<"---File is not available or Peers are Ofline or You are not part of group---";
                }
                else cout<<"---Invalid Command---";
                cout<<endl;
                fflush(stdout);
            }
            else if(cmd_list[0] == "show_downloads"){
                if(download_history.size() > 0){
                    for(int i=0;i<download_history.size();i++){
                        string filename_ = download_history[i].second;
                        if(file_chunk_count.size() > 0 && (file_chunk_count.find(filename_) != file_chunk_count.end())){
                            bool complete = true;
                            for(int j=0;j<file_chunk_count[filename_].size();j++){
                                if(file_chunk_count[filename_][j] != 1){
                                    complete = false;
                                    break;
                                }
                            }
                            if(complete){
                                cout<<"[C] ";
                                printf("[%s] ",download_history[i].first.c_str());
                                cout<<filename_<<endl;
                            }
                            else{
                                cout<<"[D] ";
                                printf("[%s] ",download_history[i].first.c_str());
                                cout<<filename_<<endl;
                            }
                        }
                    }
                }
                else{
                    cout<<"No download history"<<endl;
                }
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
        else{
            if(error_msg.size() > 0){
                cout<<error_msg<<endl;
                error_msg="";
            }
            else 
                cout<<"Invalid Command"<<endl;
        }
        fflush(stdout);
    }
    close(skt);
    return 0;
}