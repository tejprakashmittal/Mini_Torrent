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
    FILE *file_,*file_d;
    file_ = fopen("./sample.mp4", "r");
    fseek(file_, 0, SEEK_END);
    int sz = ftell(file_);
    fseek(file_, 0, SEEK_SET);
     int i=1;
    int maxS = sz/32768;
    while(i <= maxS)
    {
        cout<<ftell(file_)<<endl;
        bzero(buffer,SIZE);
        string name="./new/part"+to_string(i)+".dat";
        file_d = fopen(name.c_str(), "w");
        fread(buffer, sizeof(char), 32768, file_);
        //fseek(file_d, 0, SEEK_SET);
        fwrite(buffer,sizeof(char),32768,file_d);
        // fseek(file_, 0, SEEK_SET);
        // fseek(file_, 0, i*SIZE);
        i++;
    }
    sz=sz-maxS*SIZE;
    bzero(buffer,SIZE);
    string name="./new/part"+to_string(i)+".dat";
        file_d = fopen(name.c_str(), "w");
        fread(buffer, sizeof(char), sz, file_);
        fwrite(buffer,sizeof(char),sz,file_d);
        
         fclose(file_);
         fclose(file_d);
}