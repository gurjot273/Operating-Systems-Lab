/*
Name: Gurjot Singh Suri - 17CS10058
	  Kumar Abhishek - 17CS10022
*/
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

using namespace std;

// Max no. of characters for the command string input
#define MAXSIZE 1024

// Function to split string into strings separated by delimiters
vector<string> splitCommand(string& cmd,char delim){
    vector<string> result;
    string temp="";
    for(int i=0;i<cmd.size();i++){
        if(cmd[i]!=delim){
            temp+=cmd[i];
        }else{
            if(temp!="")
                result.push_back(temp);
            temp="";
        }
    }
    if(temp!="")
        result.push_back(temp);
    return result;
}

// Function to parse command
vector<vector<string> > parseCommand(string& cmd){
    vector<vector<string> > result;
    vector<string> temp;
    // split the command by | 
    vector<string> cmds = splitCommand(cmd,'|');
    for(int i=0;i<cmds.size();i++){
        // remove spaces from each command
        temp=splitCommand(cmds[i],' ');
        if(temp.size()>0)
            result.push_back(temp);
    }
    return result;
}

// Function to execute command
void executeCmd(vector<string>& cmd){
    string inName="", outName="";
    int len=cmd.size();
    for(int i=0;i<cmd.size();i++){
        if(cmd[i]=="<"){
            len=min(i,len);
            // checking if no file specified after <
            if(i+1>=cmd.size() || cmd[i+1]=="<" || cmd[i+1]==">"){
                perror("Syntax error with < ");
                exit(EXIT_FAILURE);
            }
            inName=cmd[i+1];
            // checking if no file specified after >
        }else if(cmd[i]==">"){
            len=min(i,len);
            if(i+1>=cmd.size() || cmd[i+1]=="<" || cmd[i+1]==">"){
                perror("Syntax error with > ");
                exit(EXIT_FAILURE);
            }
            outName=cmd[i+1];
        }
    }
    int inFile=-1;
    if(inName!=""){
        // opening input file
        inFile=open(inName.c_str(),O_RDONLY);
        if(inFile<0){
            perror("Unable to open file ");
            exit(EXIT_FAILURE);   
        }
    }


    int outFile=-1;
    if(outName!=""){
        // opening or creating output file
        outFile=open(outName.c_str(),O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
        if(outFile<0){
            perror("Unable to open file ");
            exit(EXIT_FAILURE);   
        }
    }

    if(inFile>=0){
        dup2(inFile,0);
        close(inFile);
    }

    if(outFile>=0){
        dup2(outFile,1);
        close(outFile);
    }

    char ** execArgs=new char *[len+1];
    // making argument list for executing commands
    for(int i=0;i<len;i++){
        // removing " "
        cmd[i].erase(remove(cmd[i].begin(),cmd[i].end(),'\"'),cmd[i].end());
        cmd[i].erase(remove(cmd[i].begin(),cmd[i].end(),'\''),cmd[i].end());
        execArgs[i]=new char[cmd[i].size()+1];
        strcpy(execArgs[i],cmd[i].c_str());

    }
    execArgs[len]=NULL;
    // executing the command
    execvp(execArgs[0],execArgs);
    perror("Invalid Command ");
    exit(EXIT_FAILURE);
}

int main(){
    char *input=new char[MAXSIZE];
    bool background;
    while(true){
        background=false;
        cout<<"$ ";
        cin.getline(input,MAXSIZE);
        string command="";
        int len=strlen(input);
        // quitting when input command is quit
        if(strcmp(input,"quit")==0)
            break;
        for(int i=0;i<len;i++){
            if(input[i]!='&')
                command+=input[i];
            else
                background=true; // check if & is present in command
        }

        // parsing command
        vector<vector<string> > cmds=parseCommand(command);
        
        int numPipes=cmds.size()-1;

        if(numPipes<0)
            continue;

        int **pipes=new int *[numPipes];

        
        for(int i=0;i<numPipes;i++){
            pipes[i]=new int[2];
            // checking for creation of pipes
            if(pipe(pipes[i])<0){
                perror("Pipe creation failed ");
                exit(EXIT_FAILURE);
            }
        }
        for(int i=0;i<=numPipes;i++){
            pid_t pid = fork();
            if(pid==0){
                if(numPipes==0){ // if no of pipes directly executing command
                    executeCmd(cmds[0]);
                }
                else if(i==0){ // for 1st pipe duplicating writing part with stdout_fileno
                    dup2(pipes[i][1],STDOUT_FILENO);
                    close(pipes[i][1]);
                    executeCmd(cmds[i]);
                }
                else if(i==cmds.size()-1){ // for last pipe duplicating reading part with stdin_fileno
                    dup2(pipes[i-1][0],STDIN_FILENO);
                    close(pipes[i-1][0]);
                    executeCmd(cmds[i]);
                }
                else{ // duplicating reading part from previous pipe and writing from current pipe
                    dup2(pipes[i-1][0],STDIN_FILENO);
                    close(pipes[i-1][0]);
                    dup2(pipes[i][1],STDOUT_FILENO);
                    close(pipes[i][1]);
                    executeCmd(cmds[i]);
                }

            }else{
                if(!background){
                    waitpid(pid,NULL,0); 
                    if(i!=numPipes){
                        close(pipes[i][1]);
                    }
                }else{
                    if(i!=numPipes){
                        close(pipes[i][1]);
                    }
                }
            }
        }
    }
}