#include<stdio.h>
#include<bits/stdc++.h>
#include<iostream>
#include <cstdio>
#include<fstream>
#define SIZE 32768
using namespace std;

int main(){
    char buffer[SIZE];
    bzero(buffer,SIZE);
    FILE *fp,*file_,*file_d;
    file_ = fopen("./new/sample.mp4", "w");
    int i=1;
    while(i <= 665)
    {
        bzero(buffer,SIZE);
        string name="./new/part"+to_string(i)+".dat";
        file_d = fopen(name.c_str(), "r");
    //     fp=file_d;
    //    fseek(fp, 0L, SEEK_END);
    //    int sz=ftell(fp);
    //    cout<<(sz/1024)<<endl;
        fread(buffer, sizeof(char), 32768, file_d);
        fwrite(buffer,sizeof(char),32768,file_);
        // fseek(file_, 0, SEEK_SET);
        // fseek(file_, 0, i*SIZE);
        i++;
    }
    file_d = fopen("./new/part666.dat", "r");
       fp=file_d;
       fseek(fp, 0L, SEEK_END);
       int sz=ftell(fp);
       fseek(file_d, 0L, SEEK_SET);
       //cout<<(sz)<<endl;
       //cout<<endl<<sz<<endl;
    bzero(buffer,SIZE);
        fread(buffer, sizeof(char), sz, file_d);
        //fseek(file_d, 0L, SEEK_SET);
        fwrite(buffer,sizeof(char),sz,file_);
    
        fclose(file_);
        fclose(file_d);
}