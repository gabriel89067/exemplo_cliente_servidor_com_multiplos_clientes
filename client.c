#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/random.h>
#define BUFSZ 1024 // tamanho do buffer

void usage(int argc, char **argv)
{
    printf("use: %s <IP do servidor> <porta do servidor>", argv[0]);
    printf("exemplo: %s 127.0.0.1 5151", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{

    char pala_1_1[20] = "show localmaxsensor";
    char pala_1_2[20] = "show localpotency";
    char pala_2[25] = "show externalmaxsensor";
    char pala_3_1[25] = "show externalpotency";
    char pala_3_2[25] = "show globalmaxsensor";
    char pala_4_1[25] = "show globalmaxnetwork";
    char espaco[5] = " ";
    char mensagem[1024];
    fd_set readfds;    
    fd_set read_input;
    struct timeval temp_1;
    temp_1.tv_sec = 0;
    temp_1.tv_usec = 10000;
    int activity;
    int activity_input;

    int pala_conhecid; // se a palavra existir = 1
    char pala_in[50];
    char identificador[BUFSZ];

    if (argc < 3)
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage))
    { // uso o endereço no storage
        usage(argc, argv);
    }

    //////////// socket() /////////// abertura do socket

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);

    if (s == -1)
    { // se o socket da erro gera -1
        logexit("socket");
    }

    //////////// connect() /////////// cliente esta conectando com o server
    struct sockaddr *addr = (struct sockaddr *)(&storage); // converto o storage para o endereço certo
    if (0 != connect(s, addr, sizeof(storage)))
    {
        logexit("connect");
    }
    size_t count = send(s, "REQ_ADD", strlen("REQ_ADD") + 1, 0);
    if (count != strlen("REQ_ADD") + 1)
    {
        logexit("send");
    }
    memset(mensagem, 0, BUFSZ);
     memset(identificador, 0, BUFSZ);
    count = recv(s, mensagem, BUFSZ - 1, 0);

    printf("%s\n", mensagem);

    for (int i = 8; i < strlen(mensagem); i++)
    {
        identificador[i - 8] = mensagem[i];
    }

             
    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ); // funçao que converte endereço para string

    ////////////////////////////////////// loop ///////////////////////////////
    while (1)
    { 
        
        FD_ZERO(&readfds);
        FD_SET(s, &readfds);

        temp_1.tv_usec = 50000;

        activity = select(s + 1, &readfds, NULL, NULL, &temp_1);

        if (activity < 0)
        {
            printf("Equipment limit exceeded\n");
        }
        

        FD_ZERO(&read_input);
        FD_SET(STDIN_FILENO, &read_input);

        temp_1.tv_usec = 50000;
        activity_input = select(STDIN_FILENO + 1, &read_input, NULL, NULL, &temp_1);
         if (activity_input < 0)
        {
            printf("Equipment limit exceeded\n");
        }

        
        if ( FD_ISSET(STDIN_FILENO, &read_input) )
            {
                char serv[BUFSZ];
                char res[BUFSZ];
        memset(pala_in, 0, 50);
        fgets(pala_in, 50, stdin);
        pala_conhecid = 0;
        if (strncmp(pala_1_1, pala_in, strlen(pala_1_1)) == 0)
        { // digitei show localmaxsensor
            pala_conhecid = 1;
            memset(mensagem, 0, 1024);
            strcpy(mensagem, "REQ_LS");
            

            /////////// mensagem = INS_REQ 0 5 100 30  (exemplo) //////////////////

            size_t count = send(s, mensagem, strlen(mensagem) + 1, 0);
            if (count != strlen(mensagem) + 1)
            {
                logexit("send");
            }

            memset(mensagem, 0, 1024);

            count = recv(s, mensagem, BUFSZ - 1, 0);
            memset(res, 0, 1024);
            memset(serv, 0, 1024);
            int esp = 0;
            int i = 0;
            int see = 0;
            int tee = 0;
            
   
            while (1)
            {
                
               if(mensagem[i] == espaco[0]){
                 esp = esp +1 ;
               }
               if(esp == 1){
                serv[see] = mensagem[i];
                see = see + 1;
               }
               if(esp >= 2){
                res[tee] = mensagem[i];
                tee = tee + 1;
               }
               if (i == strlen(mensagem) - 1)
               {
                break;
               }
               
               i = i + 1;

            }

         printf("Local%s sensor%s\n", serv,res);
            
        }
        if (strncmp(pala_1_2, pala_in, strlen(pala_1_2)) == 0)
        { // digitei show localpotency
            pala_conhecid = 1;
            memset(mensagem, 0, 1024);
            strcpy(mensagem, "REQ_LP");

            size_t count = send(s, mensagem, strlen(mensagem) + 1, 0);
            if (count != strlen(mensagem) + 1)
            {
                logexit("send");
            }

            memset(mensagem, 0, 1024);

            count = recv(s, mensagem, BUFSZ - 1, 0);
            ///// FALTA MANIPULAR !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            memset(res, 0, 1024);
            memset(serv, 0, 1024);
            int esp = 0;
            int i = 0;
            int see = 0;
            int tee = 0;
            
   
            while (1)
            {
                
               if(mensagem[i] == espaco[0]){
                 esp = esp +1 ;
               }
               if(esp == 1){
                serv[see] = mensagem[i];
                see = see + 1;
               }
               if(esp >= 2){
                res[tee] = mensagem[i];
                tee = tee + 1;
               }
               if (i == strlen(mensagem) - 1)
               {
                break;
               }
               
               i = i + 1;

            }
            
         printf("Local%s potency:%s\n", serv,res);
        }
        if (strncmp(pala_2, pala_in, strlen(pala_2)) == 0)
        { // digitei show externalmaxsensor
            pala_conhecid = 1;
            memset(mensagem, 0, 1024);
            strcpy(mensagem, "REQ_ES");
            count = send(s, mensagem, strlen(mensagem) + 1, 0);
            if (count != strlen(mensagem) + 1)
            {
                logexit("send");
            }
            count = recv(s, mensagem, BUFSZ - 1, 0);
            ///// FALTA MANIPULAR !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
             
             memset(res, 0, 1024);
            memset(serv, 0, 1024);
            int esp = 0;
            int i = 0;
            int see = 0;
            int tee = 0;
            
   
            while (1)
            {
                
               if(mensagem[i] == espaco[0]){
                 esp = esp +1 ;
               }
               if(esp == 1){
                serv[see] = mensagem[i];
                see = see + 1;
               }
               if(esp >= 2){
                res[tee] = mensagem[i];
                tee = tee + 1;
               }
               if (i == strlen(mensagem) - 1)
               {
                break;
               }
               
               i = i + 1;

            }

         printf("External%s sensor%s\n", serv,res);
        }
        if (strncmp(pala_3_1, pala_in, strlen(pala_3_1)) == 0)
        { // digitei show externalpotency
            pala_conhecid = 1;
            memset(mensagem, 0, 1024);
            strcpy(mensagem, "REQ_EP");
            count = send(s, mensagem, strlen(mensagem) + 1, 0);
            if (count != strlen(mensagem) + 1)
            {
                logexit("send");
            }
            count = recv(s, mensagem, BUFSZ - 1, 0);

             memset(res, 0, 1024);
            memset(serv, 0, 1024);
            int esp = 0;
            int i = 0;
            int see = 0;
            int tee = 0;
            
   
            while (1)
            {
                
               if(mensagem[i] == espaco[0]){
                 esp = esp +1 ;
               }
               if(esp == 1){
                serv[see] = mensagem[i];
                see = see + 1;
               }
               if(esp >= 2){
                res[tee] = mensagem[i];
                tee = tee + 1;
               }
               if (i == strlen(mensagem) - 1)
               {
                break;
               }
               
               i = i + 1;

            }
            
         printf("External%s potency:%s\n", serv,res);
        }
        if (strncmp(pala_3_2, pala_in, strlen(pala_3_2)) == 0)
        { // digitei show globalmaxsensor
            pala_conhecid = 1;
            memset(mensagem, 0, 1024);
            strcpy(mensagem, "REQ_MS");
            count = send(s, mensagem, strlen(mensagem) + 1, 0);
            if (count != strlen(mensagem) + 1)
            {
                logexit("send");
            }
            count = recv(s, mensagem, BUFSZ - 1, 0);
            memset(res, 0, 1024);
            memset(serv, 0, 1024);
            int esp = 0;
            int i = 0;
            int see = 0;
            int tee = 0;
            
   
            while (1)
            {
                
               if(mensagem[i] == espaco[0]){
                 esp = esp +1 ;
               }
               if(esp == 1){
                serv[see] = mensagem[i];
                see = see + 1;
               }
               if(esp >= 2){
                res[tee] = mensagem[i];
                tee = tee + 1;
               }
               if (i == strlen(mensagem) - 1)
               {
                break;
               }
               
               i = i + 1;

            }
            
         printf("global%s sensor:%s\n", serv,res);
        }
        if (strncmp(pala_4_1, pala_in, strlen(pala_4_1)) == 0)
        { // digitei show globalmaxnetwork

            pala_conhecid = 1;
            memset(mensagem, 0, 1024);
            strcpy(mensagem, "REQ_MN");
            count = send(s, mensagem, strlen(mensagem) + 1, 0);
            if (count != strlen(mensagem) + 1)
            {
                logexit("send");
            }
            count = recv(s, mensagem, BUFSZ - 1, 0);
             memset(res, 0, 1024);
            memset(serv, 0, 1024);
            int esp = 0;
            int i = 0;
            int see = 0;
            int tee = 0;
            
   
            while (1)
            {
                
               if(mensagem[i] == espaco[0]){
                 esp = esp +1 ;
               }
               if(esp == 1){
                serv[see] = mensagem[i];
                see = see + 1;
               }
               if(esp >= 2){
                res[tee] = mensagem[i];
                tee = tee + 1;
               }
               if (i == strlen(mensagem) - 1)
               {
                break;
               }
               
               i = i + 1;

            }
            
         printf("global%s potency:%s\n", serv,res);
        }
        if (strncmp("kill", pala_in, 4) == 0)
        {
            memset(mensagem, 0, strlen(mensagem));
            pala_conhecid = 1;
            strcpy(mensagem, "REQ_DC(");
            strcat(mensagem, identificador);
            strcat(mensagem, ")");
            size_t count = send(s, mensagem, strlen(mensagem) + 1, 0);
            if (count != strlen(mensagem) + 1)
            {
                logexit("send");
            }
            memset(mensagem, 0, strlen(mensagem));
            count = recv(s, mensagem, BUFSZ - 1, 0);

            if (strcmp(mensagem, "Client not found") == 0)
            {
                printf("%s\n", mensagem);
            }
            else if (strcmp(mensagem, "Successful disconnect") == 0)
            {
                printf("%s\n", mensagem);
                close(s);
                break;
            }
        }

        if (pala_conhecid == 0)
        { ////////////// close() //////////////// clienet encerra conexao

           memset(mensagem, 0, strlen(mensagem));
            pala_conhecid = 1;
            strcpy(mensagem, "REQ_DC(");
            strcat(mensagem, identificador);
            strcat(mensagem, ")");
            size_t count = send(s, mensagem, strlen(mensagem) + 1, 0);
            if (count != strlen(mensagem) + 1)
            {
                logexit("send");
            }
            memset(mensagem, 0, strlen(mensagem));
            count = recv(s, mensagem, BUFSZ - 1, 0);

            if (strcmp(mensagem, "Client not found") == 0)
            {
                printf("%s\n", mensagem);
            }
            else if (strcmp(mensagem, "Successful disconnect") == 0)
            {
                printf("%s\n", mensagem);
                close(s);
                break;
            }
        }
            }
         if ( FD_ISSET(s, &readfds))
            {
                
            memset(mensagem, 0, strlen(mensagem));
             count = recv(s, mensagem, BUFSZ - 1, 0);    
             if (strncmp("kill", mensagem, strlen(mensagem)) == 0)
             {
              printf("Successful disconnect\n");
              close(s);
              break;
             }}

    }

    exit(EXIT_SUCCESS);
}