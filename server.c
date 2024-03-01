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

#define BUFSZ 1024
#define clientmax 10
#define servmax 2

struct equipamento
{
    int ID;
    int CORR;
    int TENS;
    int EFI;
    int POT;
    int POT_UTIL;
};

void usage(int argc, char **argv)
{
    printf("usage: %s <ipv4> <porta do servidor> <porta para clientes>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    struct equipamento equipamentos[100];

    srand(time(NULL));
    for (int i = 0; i < 10; i++)
    { // inicializa os sensores no servidor com valores aleatorios

        equipamentos[i].ID = i;
        equipamentos[i].EFI = rand() % 100;
        equipamentos[i].POT = rand() % 1500;
        equipamentos[i].POT_UTIL = (equipamentos[i].EFI * equipamentos[i].POT) / 100;
    }

    char mensagem[1024];

    char espaco[4] = " ";
    int activity;
    int activity_p2p;
    int activity_input;
    int max_sd;
    int max_sd_p2p;
    fd_set readfds;     // monitorador dos descritores de arquivo dos clientes
    fd_set readfds_p2p; // monitorador dos descritores de arquivo dos servers
    fd_set read_input;  // moniorando comando do teclado
    int sd;
    int sd_p2p;
    int client_socket[clientmax];
    int server_socket[servmax];
    int quant_client = 0;
    int quant_server = 0;
    int cliente_max = 0;
    int server_max = 0;
    char value_num[30];
    char identificador[BUFSZ];
    memset(identificador, 0, BUFSZ);
    int verif = 0;
    char identificador_serv_local[BUFSZ];
    int identificador_serv_local_int = 0;
    char identificador_serv_externo[BUFSZ];
    int identificador_serv_externo_int = 0;
    struct timeval temp_1;
    temp_1.tv_sec = 0;
    temp_1.tv_usec = 10000;
    int limite_de_serv = 0;
    char palavra_entrada_tec[BUFSZ];
    int nao_ler_sele = 0;

    //////////////////////// limpo a lista de sockects conectrados no setrver

    for (int i = 0; i < clientmax; i++)
    {
        client_socket[i] = 0;
    }

    for (int i = 0; i < servmax; i++)
    {
        server_socket[i] = 0;
    }

    if (argc < 3)
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[3], &storage))
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage_p2p;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage_p2p))
    {
        usage(argc, argv);
    }

    //////////// socket() /////////// abertura do socket para clientes
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage)))
    {
        logexit("bind");
    }

    //////////// socket_p2p() /////////// abertura do socket para servidor
    int s_p2p;
    s_p2p = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        logexit("socket");
    }

    ///////// verifica se a porta p2p esta em uso !!!!!!!!!!!!
    struct sockaddr *addr_p2p = (struct sockaddr *)(&storage_p2p);
    int verifica_cone_p2p = bind(s_p2p, addr_p2p, sizeof(storage_p2p));

    if (0 == verifica_cone_p2p) // nao existe logo e possivel conetar
    {
        //////////// listen_p2p() /////////// fica aguardando ate 2 servers na fila

        if (0 != listen(s_p2p, 4))
        {
            logexit("listen");
        }
        printf("No peer found, starting to listen..\n");
        sprintf(identificador_serv_local, "%d", s_p2p);
        identificador_serv_local_int = s_p2p;
    }
    else //////// existe conexao na porta do server _p2p
    {

        struct sockaddr *addr_p2p = (struct sockaddr *)(&storage_p2p); // converto o storage para o endereço certo
        if (0 != connect(s_p2p, addr_p2p, sizeof(storage_p2p)))
        {
            logexit("connect_p2p");
        }

        sprintf(identificador_serv_externo, "%d", s_p2p);
        identificador_serv_externo_int = s_p2p;

        size_t count_p2p = send(s_p2p, "REQ_ADDPEER", strlen("REQ_ADDPEER") + 1, 0);
        if (count_p2p != strlen("REQ_ADDPEER") + 1)
        {
            logexit("send");
        }
        memset(mensagem, 0, BUFSZ);
        count_p2p = recv(s_p2p, mensagem, BUFSZ - 1, 0);

        if (strcmp(mensagem, "Peer limit exceeded") == 0)
        {

            printf("%s\n", mensagem);
            limite_de_serv = 1;
        }
        else
        {
            for (int i = 12; i < strlen(mensagem) - 1; i++)
            {
                identificador_serv_local[i - 12] = mensagem[i];
            }
            memset(mensagem, 0, BUFSZ);
            char num_char[BUFSZ];
            sprintf(num_char, "%d", s_p2p);
            strcpy(mensagem, "New Peer ID: ");
            strcat(mensagem, identificador_serv_local);
            identificador_serv_local_int = atoi(identificador_serv_local);
            printf("%s\n", mensagem);

            memset(mensagem, 0, BUFSZ);
            strcpy(mensagem, "RES_ADDPEER(");
            strcat(mensagem, num_char);
            strcat(mensagem, ")");

            count_p2p = send(s_p2p, mensagem, strlen(mensagem) + 1, 0);
            if (count_p2p != strlen(mensagem) + 1)
            {
                logexit("send");
            }
            printf("Peer %d connected\n", s_p2p);
        }
    }

    //////////// listen() /////////// fica aguardando ate 15 clientes na fila
    if (0 != listen(s, 15))
    {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    while (1)
    {
        
        if (limite_de_serv == 1)
        {
            break;
        }

        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        FD_ZERO(&readfds);
        FD_SET(s, &readfds);

        max_sd = s;

        for (int i = 0; i < clientmax; i++)
        {
            sd = client_socket[i];
            if (sd > 0)
            {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd)
            {
                max_sd = sd;
            }
        }

        temp_1.tv_usec = 50000;

        activity = select(max_sd + 1, &readfds, NULL, NULL, &temp_1);

        if (activity < 0)
        {
            printf("Equipment limit exceeded\n");
        }

        struct sockaddr_storage cstorage_p2p;
        struct sockaddr *caddr_p2p = (struct sockaddr *)(&cstorage_p2p);
        socklen_t caddrlen_p2p = sizeof(cstorage_p2p);
        if (nao_ler_sele == 0)
        {
            FD_ZERO(&readfds_p2p);
            FD_SET(s_p2p, &readfds_p2p);

            max_sd_p2p = s_p2p;

            for (int i = 0; i < servmax; i++)
            {
                sd_p2p = server_socket[i];
                if (sd_p2p > 0)
                {
                    FD_SET(sd_p2p, &readfds_p2p);
                }
                if (sd_p2p > max_sd_p2p)
                {
                    max_sd_p2p = sd_p2p;
                }
            }
            temp_1.tv_usec = 50000;
            activity_p2p = select(max_sd_p2p + 1, &readfds_p2p, NULL, NULL, &temp_1);

            if (activity_p2p < 0)
            {
                printf("Equipment limit exceeded\n");
            }
        }

        FD_ZERO(&read_input);
        FD_SET(STDIN_FILENO, &read_input);

        temp_1.tv_usec = 50000;
        activity_input = select(STDIN_FILENO + 1, &read_input, NULL, NULL, &temp_1);

        if (activity_input == 0)
        {
        }
        if (server_max == 0)
        {
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////recebi connect do server_p2p ////////
        if (verifica_cone_p2p == 0)
        {
            if (FD_ISSET(s_p2p, &readfds_p2p) != 0 && FD_ISSET(s, &readfds) == 0 && FD_ISSET(STDIN_FILENO, &read_input) == 0)
            { // recebecndo conexao com o server

                int csock_p2p = accept(s_p2p, caddr_p2p, &caddrlen_p2p);
                if (csock_p2p == -1)
                {
                    logexit("accept_p2p");
                }
                else
                {
                    quant_server = quant_server + 1;
                    sprintf(identificador_serv_externo, "%d", csock_p2p);
                    identificador_serv_externo_int = csock_p2p;
                }
                int i = 0;
                while (i < servmax)
                {
                    if (server_socket[i] == 0)
                    {
                        server_socket[i] = csock_p2p;
                        server_max = i;
                        break;
                    }
                    i = i + 1;
                }

                char caddrstr_p2p[BUFSZ];
                addrtostr(caddr_p2p, caddrstr_p2p, BUFSZ);

                char buf_p2p[BUFSZ];
                memset(buf_p2p, 0, BUFSZ);
                size_t count_p2p = recv(csock_p2p, buf_p2p, BUFSZ - 1, 0);
                // printf("%s\n",buf_p2p);
                if (count_p2p == 0)
                {
                }

                if (quant_server > servmax - 1)
                {
                    count_p2p = send(csock_p2p, "Peer limit exceeded", strlen("Peer limit exceeded") + 1, 0);
                    server_socket[i] = 0;
                    quant_server = quant_server - 1;
                    close(csock_p2p);
                }
                else if (quant_server <= servmax - 1)
                {

                    printf("Peer %d connected\n", csock_p2p);
                    sprintf(value_num, "%d", csock_p2p);
                    memset(buf_p2p, 0, BUFSZ);
                    strcpy(buf_p2p, "RES_ADDPEER(");
                    strcat(buf_p2p, value_num);
                    strcat(buf_p2p, ")");
                    count_p2p = send(csock_p2p, buf_p2p, strlen(buf_p2p) + 1, 0);

                    char buf_p2p[BUFSZ];
                    memset(buf_p2p, 0, BUFSZ);
                    count_p2p = recv(csock_p2p, buf_p2p, BUFSZ - 1, 0);

                    for (int i = 12; i < strlen(buf_p2p) - 1; i++)
                    {
                        identificador_serv_local[i - 12] = buf_p2p[i];
                    }

                    printf("New Peer ID: %s\n", identificador_serv_local);
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////recebe connect do client ////////////////////////////////
        if (!(FD_ISSET(s_p2p, &readfds_p2p)) && FD_ISSET(s, &readfds) && !(FD_ISSET(STDIN_FILENO, &read_input)))
        { // recebecndo conexao com o cliente

            int csock = accept(s, caddr, &caddrlen);
            if (csock == -1)
            {
                logexit("accept");
            }
            else
            {
                quant_client = quant_client + 1;
            }
            int i = 0;
            while (i < clientmax)
            {
                if (client_socket[i] == 0)
                {
                    client_socket[i] = csock;
                    cliente_max = i;
                    break;
                }
                i = i + 1;
            }

            char caddrstr[BUFSZ];
            addrtostr(caddr, caddrstr, BUFSZ);

            char buf[BUFSZ];
            memset(buf, 0, BUFSZ);
            size_t count = recv(csock, buf, BUFSZ - 1, 0);
            printf("%s\n", buf);

            if (count == 0)
            {
            }

            if (quant_client > clientmax)
            {
                count = send(csock, "Client limit exceeded", strlen("Client limit exceeded") + 1, 0);
                client_socket[cliente_max] = 0;
                quant_client = quant_client - 1;
                close(csock);
            }
            else if (quant_client <= clientmax)
            {

                printf("Client %d added\n", csock);
                sprintf(value_num, "%d", csock);
                memset(buf, 0, BUFSZ);
                strcpy(buf, "New ID: ");
                strcat(buf, value_num);
                count = send(csock, buf, strlen(buf) + 1, 0);
            }
        }
        ////////////////////////////////////////////////////////////////////// recebi comando do teclado no server///////////////////////////////////////////
        if (!(FD_ISSET(s_p2p, &readfds_p2p)) && !(FD_ISSET(s, &readfds)) && FD_ISSET(STDIN_FILENO, &read_input))
        { 
           

            memset(palavra_entrada_tec, 0, BUFSZ);
            fgets(palavra_entrada_tec, BUFSZ, stdin);
            memset(mensagem, 0, BUFSZ);

            if (strcmp(palavra_entrada_tec, "kill\n") == 0)
            {

                if (0 == verifica_cone_p2p)
                { ////////// estou no main

                    if (identificador_serv_externo_int == 0)
                    {
                        memset(mensagem, 0, BUFSZ);
                        strcpy(mensagem, "kill");

                        for (int i = 0; i < clientmax; i++)
                        {

                            sd = client_socket[i];
                            size_t count_p2p = send(sd, mensagem, strlen(mensagem) + 1, 0);
                            close(sd);
                            if (count_p2p == 0)
                            {
                            }
                        }
                        

                        close(s);
                        close(s_p2p);

                        break;
                    }
                    else
                    {

                        strcpy(mensagem, "REQ_DCPEER(");
                        strcat(mensagem, identificador_serv_local);
                        strcat(mensagem, ")");
                        size_t count_p2p = send(identificador_serv_externo_int, mensagem, strlen(mensagem) + 1, 0);
                        if (count_p2p == 0)
                        {
                        }

                        memset(mensagem, 0, BUFSZ);
                        count_p2p = recv(identificador_serv_externo_int, mensagem, BUFSZ - 1, 0); // recebi uma mensagem do client  travou!!!!!!!!!
                        printf("%s\n", mensagem);                                                 // mensagem

                        printf("Peer %d disconnected\n", identificador_serv_local_int);

                        memset(identificador_serv_externo, 0, strlen(identificador_serv_externo));
                        quant_server = quant_server - 1;
                        server_socket[0] = 0;

                        memset(mensagem, 0, BUFSZ);
                        strcpy(mensagem, "kill");

                        for (int i = 0; i < clientmax; i++)
                        {
                            sd = client_socket[i];
                            size_t count_p2p = send(sd, mensagem, strlen(mensagem) + 1, 0);
                            close(sd);
                            if (count_p2p == 0)
                            {
                            }
                        }

                        close(s);
                        close(identificador_serv_externo_int);
                        identificador_serv_externo_int = 0;
                        break; ////// esse da pau
                    }
                }
                else
                {
                    
                    if (identificador_serv_externo_int == 0)
                    {

                        memset(mensagem, 0, BUFSZ);
                        strcpy(mensagem, "kill");

                        for (int i = 0; i < clientmax; i++)
                        {
                            sd = client_socket[i];
                            size_t count_p2p = send(sd, mensagem, strlen(mensagem) + 1, 0);
                            close(sd);
                            if (count_p2p == 0)
                            {
                            }
                        }
                        close(s);
                        close(s_p2p);
                        break;
                    }
                    else
                    {
                       
                        strcpy(mensagem, "REQ_DCPEER(");
                        strcat(mensagem, identificador_serv_local);
                        strcat(mensagem, ")");
                        size_t count_p2p = send(identificador_serv_externo_int, mensagem, strlen(mensagem) + 1, 0);
                        if (count_p2p == 0)
                        {
                        }

                        memset(mensagem, 0, BUFSZ);
                        count_p2p = recv(identificador_serv_externo_int, mensagem, BUFSZ - 1, 0); // recebi uma mensagem do client
                        printf("%s\n", mensagem);                                                 // mensagem
                        printf("Peer %d disconnected\n", identificador_serv_local_int);

                        memset(identificador_serv_externo, 0, strlen(identificador_serv_externo));
                        quant_server = quant_server - 1;
                        server_socket[0] = 0;
                        memset(mensagem, 0, BUFSZ);
                        strcpy(mensagem, "kill");

                        for (int i = 0; i < clientmax; i++)
                        {
                            sd = client_socket[i];
                            size_t count_p2p = send(sd, mensagem, strlen(mensagem) + 1, 0);
                            close(sd);
                            if (count_p2p == 0)
                            {
                            }
                        }

                        close(s);
                        close(identificador_serv_externo_int);
                        identificador_serv_externo_int = 0;
                        break;
                    }
                }
            }
        }

        ////////////// recv() //////////////// server recebe dado do cliente
        for (int i = 0; i < clientmax; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds))
            {

                char buf[BUFSZ];
                int atr_1 = 0;
                memset(buf, 0, BUFSZ);
                size_t count = recv(sd, buf, BUFSZ - 1, 0); // recebi uma mensagem do client

                if (strncmp("REQ_LS", buf, strlen("REQ_LS")) == 0)
                { ////////////// recebi REQ_LS do cliente

                    printf("%s\n", buf);
                    int max = 0;
                    char value[BUFSZ];
                    for (int i = 0; i < 10; i++)
                    {

                        if (max < equipamentos[i].POT_UTIL)
                        {
                            max = equipamentos[i].POT_UTIL;
                            memset(value, 0, BUFSZ);
                            sprintf(value, "%d", equipamentos[i].ID);
                            atr_1 = i;
                        }
                    }
                    memset(buf, 0, BUFSZ);
                    strcpy(buf, "RES_LS ");
                    strcat(buf, identificador_serv_local);
                    strcat(buf, " ");
                    strcat(buf, value);
                    strcat(buf, ": ");
                    memset(value, 0, BUFSZ);
                    sprintf(value, "%d", equipamentos[atr_1].POT_UTIL);
                    strcat(buf, value);
                    strcat(buf, " (");
                    memset(value, 0, BUFSZ);
                    sprintf(value, "%d", equipamentos[atr_1].POT);
                    strcat(buf, value);
                    strcat(buf, " ");
                    memset(value, 0, BUFSZ);
                    sprintf(value, "%d", equipamentos[atr_1].EFI);
                    strcat(buf, value);
                    strcat(buf, ")");

                    count = send(sd, buf, strlen(buf) + 1, 0);
                    if (count != strlen(buf) + 1)
                    {
                        logexit("send");
                    }
                }

                if (strncmp("REQ_LP", buf, strlen("REQ_LP")) == 0)
                { ////////////// recebi REQ_LP do cliente
                    puts(buf);
                    int POTENC_TOTAL = 0;
                    char POT_TOT[BUFSZ];
                    for (int i = 0; i < 10; i++)
                    {
                        POTENC_TOTAL = POTENC_TOTAL + equipamentos[i].POT_UTIL;
                    }

                    memset(buf, 0, BUFSZ);
                    sprintf(POT_TOT, "%d", POTENC_TOTAL);

                    strcpy(buf, "RES_LP ");
                    strcat(buf, identificador_serv_local);
                    strcat(buf, " ");
                    strcat(buf, POT_TOT);

                    count = send(sd, buf, strlen(buf) + 1, 0);
                    if (count != strlen(buf) + 1)
                    {
                        logexit("send");
                    }
                }

                if (strncmp("REQ_ES", buf, strlen("REQ_ES")) == 0)
                { ////////////// recebi REQ_ES do cliente

                    printf("%s\n", buf);

                    memset(buf, 0, BUFSZ);
                    strcpy(buf, "REQ_ES");
                    count = send(identificador_serv_externo_int, buf, strlen(buf) + 1, 0);
                    if (count != strlen(buf) + 1)
                    {
                        logexit("send");
                    }
                    memset(buf, 0, BUFSZ);
                    count = recv(identificador_serv_externo_int, buf, BUFSZ - 1, 0);
                    count = send(sd, buf, strlen(buf) + 1, 0);
                    if (count != strlen(buf) + 1)
                    {
                        logexit("send");
                    }
                }

                if (strncmp("REQ_EP", buf, strlen("REQ_EP")) == 0)
                { ////////////// recebi REQ_EP do cliente
                    printf("%s\n", buf);

                    memset(buf, 0, BUFSZ);
                    strcpy(buf, "REQ_EP");
                    count = send(identificador_serv_externo_int, buf, strlen(buf) + 1, 0);
                    if (count != strlen(buf) + 1)
                    {
                        logexit("send");
                    }
                    memset(buf, 0, BUFSZ);
                    count = recv(identificador_serv_externo_int, buf, BUFSZ - 1, 0);
                    count = send(sd, buf, strlen(buf) + 1, 0);
                    if (count != strlen(buf) + 1)
                    {
                        logexit("send");
                    }
                }

                if (strncmp("REQ_MS", buf, strlen("REQ_MS")) == 0)
                { ////////////// recebi REQ_MS do cliente
                    char res_potencia_local[BUFSZ];
                    char res_potencia_externa[BUFSZ];

                    atr_1 = 0;
                    printf("%s\n", buf);

                    char potencia_local[BUFSZ];
                    int potencia_local_int = 0;
                    for (int i = 0; i < 10; i++)
                    {
                        if (potencia_local_int < equipamentos[i].POT_UTIL)
                        {
                            potencia_local_int = equipamentos[i].POT_UTIL;
                            atr_1 = i;
                        }
                    }

                    memset(res_potencia_local, 0, BUFSZ);
                    strcpy(res_potencia_local, "RES_MS ");
                    strcat(res_potencia_local, identificador_serv_local);
                    strcat(res_potencia_local, " ");
                    sprintf(potencia_local, "%d", atr_1);
                    strcat(res_potencia_local, potencia_local);
                    strcat(res_potencia_local, ": ");
                    memset(potencia_local, 0, BUFSZ);
                    sprintf(potencia_local, "%d", equipamentos[atr_1].POT_UTIL);

                    strcat(res_potencia_local, potencia_local);
                    strcat(res_potencia_local, " (");
                    memset(potencia_local, 0, BUFSZ);
                    sprintf(potencia_local, "%d", equipamentos[atr_1].POT);
                    strcat(res_potencia_local, potencia_local);
                    strcat(res_potencia_local, " ");
                    memset(potencia_local, 0, BUFSZ);
                    sprintf(potencia_local, "%d", equipamentos[atr_1].EFI);
                    strcat(res_potencia_local, potencia_local);
                    strcat(res_potencia_local, ")");

                    memset(buf, 0, BUFSZ);
                    char potencia_externa[BUFSZ];
                    int potencia_extern_int = 0;
                    strcpy(buf, "REQ_ES");
                    count = send(identificador_serv_externo_int, buf, strlen(buf) + 1, 0);
                    if (count != strlen(buf) + 1)
                    {
                        logexit("send");
                    }
                    memset(buf, 0, BUFSZ);
                    count = recv(identificador_serv_externo_int, buf, BUFSZ - 1, 0);
                    printf("%s\n", buf);
                    int saida = 0;
                    for (int i = 12; saida == 0; i++)
                    {
                        if (buf[i] == espaco[0])
                        {
                            saida = 1;
                        }
                        potencia_externa[i - 12] = buf[i];
                    }

                    potencia_extern_int = atoi(potencia_externa);
                    memset(res_potencia_externa, 0, strlen(res_potencia_externa));
                    strcpy(res_potencia_externa, "RES_MS");
                    for (int i = 5; i < strlen(buf); i++)
                    {
                        res_potencia_externa[i] = buf[i];
                    }

                    if (potencia_extern_int > potencia_local_int)
                    {

                        count = send(sd, res_potencia_externa, strlen(res_potencia_externa) + 1, 0);
                        if (count != strlen(res_potencia_externa) + 1)
                        {
                            logexit("send");
                        }
                    }
                    else
                    {

                        count = send(sd, res_potencia_local, strlen(res_potencia_local) + 1, 0);
                        if (count != strlen(res_potencia_local) + 1)
                        {
                            logexit("send");
                        }
                    }
                }

                if (strncmp("REQ_MN", buf, strlen("REQ_MN")) == 0)
                { ////////////// recebi REQ_MN do cliente
                    char RES_POT_TOT_LOCAL[BUFSZ];
                    char RES_POT_TOT_EXTERNO[BUFSZ];
                    int POTENC_TOTAL_LOCAL = 0;
                    int POTENC_TOTAL_EXTERNO = 0;
                    char POT_TOT_LOCAL[BUFSZ];
                    char POT_TOT_EXTERNO[BUFSZ];
                    memset(POT_TOT_LOCAL, 0, BUFSZ);
                    memset(POT_TOT_EXTERNO, 0, BUFSZ);

                    for (int i = 0; i < 10; i++)
                    {
                        POTENC_TOTAL_LOCAL = POTENC_TOTAL_LOCAL + equipamentos[i].POT_UTIL;
                    }

                    memset(RES_POT_TOT_LOCAL, 0, BUFSZ);
                    sprintf(POT_TOT_LOCAL, "%d", POTENC_TOTAL_LOCAL);

                    strcpy(RES_POT_TOT_LOCAL, "RES_LP ");
                    strcat(RES_POT_TOT_LOCAL, identificador_serv_local);
                    strcat(RES_POT_TOT_LOCAL, " ");
                    strcat(RES_POT_TOT_LOCAL, POT_TOT_LOCAL);

                    // PARTE EXTERNA ///////////////////////////
                    memset(RES_POT_TOT_EXTERNO, 0, BUFSZ);
                    strcpy(RES_POT_TOT_EXTERNO, "REQ_EP");
                    count = send(identificador_serv_externo_int, RES_POT_TOT_EXTERNO, strlen(RES_POT_TOT_EXTERNO) + 1, 0);
                    if (count != strlen(buf) + 1)
                    {
                        logexit("send");
                    }
                    memset(RES_POT_TOT_EXTERNO, 0, BUFSZ);

                    count = recv(identificador_serv_externo_int, RES_POT_TOT_EXTERNO, BUFSZ - 1, 0);

                    for (int i = 9; i < strlen(RES_POT_TOT_EXTERNO); i++)
                    {

                        POT_TOT_EXTERNO[i - 9] = RES_POT_TOT_EXTERNO[i];
                    }

                    POTENC_TOTAL_EXTERNO = atoi(POT_TOT_EXTERNO);

                    if (POTENC_TOTAL_EXTERNO > POTENC_TOTAL_LOCAL)
                    {
                        count = send(sd, RES_POT_TOT_EXTERNO, strlen(RES_POT_TOT_EXTERNO) + 1, 0);
                        if (count != strlen(RES_POT_TOT_EXTERNO) + 1)
                        {
                            logexit("send");
                        }
                    }
                    else
                    {
                        count = send(sd, RES_POT_TOT_LOCAL, strlen(RES_POT_TOT_LOCAL) + 1, 0);
                        if (count != strlen(RES_POT_TOT_LOCAL) + 1)
                        {
                            logexit("send");
                        }
                    }
                }


                if (strncmp(buf, "REQ_DC(", 7) == 0)
                { ////////////// recebi kill do cliente
                    verif = 0;
                    printf("%s\n", buf);
                    for (int i = 7; i < strlen(buf) - 1; i++)
                    {
                        identificador[i - 7] = buf[i];
                    }
                    for (int i = 0; i < clientmax; i++)
                    {
                        if (atoi(identificador) == client_socket[i])
                        {
                            size_t count = send(sd, "Successful disconnect", strlen("Successful disconnect") + 1, 0);
                            if (count != strlen("Successful disconnect") + 1)
                            {
                                logexit("send");
                            }

                            strcpy(mensagem, "Client ");
                            strcat(mensagem, identificador);
                            strcat(mensagem, " removed");
                            printf("%s\n", mensagem);
                            close(client_socket[i]);
                            client_socket[i] = 0;
                            quant_client = quant_client - 1;
                            verif = 1;
                        }
                    }
                    if (verif == 0)
                    {
                        size_t count = send(sd, "Client not found", strlen("Client not found") + 1, 0);
                        if (count != strlen("Client not found") + 1)
                        {
                            logexit("send");
                        }
                    }
                }
            }
        }

        ////////////// recv() //////////////// server recebe dado do p2p

        sd_p2p = identificador_serv_externo_int;

        if (FD_ISSET(sd_p2p, &readfds_p2p))
        {
            char value_rece[BUFSZ];
            char buf_p2p[BUFSZ];
            memset(buf_p2p, 0, BUFSZ);
            memset(value_rece, 0, BUFSZ);
            size_t count_p2p = recv(sd_p2p, buf_p2p, BUFSZ - 1, 0); // recebi uma mensagem do server_p2p

            if (strncmp(buf_p2p, "REQ_DCPEER", 9) == 0)
            { // recebi "REQ_DCPEER" do server_p2p

                for (int i = 11; i < strlen(buf_p2p) - 1; i++)
                {
                    value_rece[i - 11] = buf_p2p[i];
                }

                if (identificador_serv_externo_int == atoi(value_rece))
                {
                    if (verifica_cone_p2p == 0)
                    {

                        printf("Peer %d disconnected\n", identificador_serv_externo_int);
                        memset(buf_p2p, 0, BUFSZ);
                        strcpy(buf_p2p, "Successful disconnect");
                        count_p2p = send(identificador_serv_externo_int, buf_p2p, strlen(buf_p2p) + 1, 0);

                        memset(identificador_serv_externo, 0, strlen(identificador_serv_externo));
                        quant_server = quant_server - 1;
                        server_socket[0] = 0;

                        close(identificador_serv_externo_int);
                        identificador_serv_externo_int = 0;
                    }
                    else
                    {

                        printf("Peer %d disconnected\n", identificador_serv_externo_int);
                        memset(buf_p2p, 0, BUFSZ);
                        strcpy(buf_p2p, "Successful disconnect");
                        count_p2p = send(identificador_serv_externo_int, buf_p2p, strlen(buf_p2p) + 1, 0);

                        memset(identificador_serv_externo, 0, strlen(identificador_serv_externo));
                        quant_server = quant_server - 1;
                        server_socket[0] = 0;

                        close(identificador_serv_externo_int);
                        identificador_serv_externo_int = 0;

                        verifica_cone_p2p = 0;
                        // bind(s_p2p, addr_p2p, sizeof(storage_p2p));
                        s_p2p = 0;
                        nao_ler_sele = 1;
                    }
                }
            }

            if (strcmp(buf_p2p, "REQ_ES") == 0)
            { // recebi req_es do servidor p2p

                int atr_p2p = 0;
                int max = 0;
                char value[BUFSZ];
                for (int i = 0; i < 10; i++)
                {

                    if (max < equipamentos[i].POT_UTIL)
                    {
                        max = equipamentos[i].POT_UTIL;
                        memset(value, 0, BUFSZ);
                        sprintf(value, "%d", equipamentos[i].ID);
                        atr_p2p = i;
                    }
                }
                memset(buf_p2p, 0, BUFSZ);
                strcpy(buf_p2p, "RES_ES ");
                strcat(buf_p2p, identificador_serv_local);
                strcat(buf_p2p, " ");
                strcat(buf_p2p, value);
                strcat(buf_p2p, ": ");
                memset(value, 0, BUFSZ);
                sprintf(value, "%d", equipamentos[atr_p2p].POT_UTIL);
                strcat(buf_p2p, value);
                strcat(buf_p2p, " (");
                memset(value, 0, BUFSZ);
                sprintf(value, "%d", equipamentos[atr_p2p].POT);
                strcat(buf_p2p, value);
                strcat(buf_p2p, " ");
                memset(value, 0, BUFSZ);
                sprintf(value, "%d", equipamentos[atr_p2p].EFI);
                strcat(buf_p2p, value);
                strcat(buf_p2p, ")");
                printf("%s\n", buf_p2p);

                count_p2p = send(sd_p2p, buf_p2p, strlen(buf_p2p) + 1, 0);
                if (count_p2p != strlen(buf_p2p) + 1)
                {
                    logexit("send");
                }
            }

            if (strcmp(buf_p2p, "REQ_EP") == 0) ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            {                                   // recebi REQ_EP do servidor p2p

                printf("%s\n", buf_p2p);

                int pot_tot = 0;
                char pot_char[BUFSZ];

                for (int i = 0; i < 10; i++)
                {
                    pot_tot = pot_tot + equipamentos[i].POT_UTIL;
                }
                memset(buf_p2p, 0, BUFSZ);
                strcpy(buf_p2p, "RES_EP ");
                strcat(buf_p2p, identificador_serv_local);
                strcat(buf_p2p, " ");
                sprintf(pot_char, "%d", pot_tot);
                strcat(buf_p2p, pot_char);

                printf("%s\n", buf_p2p);

                count_p2p = send(sd_p2p, buf_p2p, strlen(buf_p2p) + 1, 0);
                if (count_p2p != strlen(buf_p2p) + 1)
                {
                    logexit("send");
                }
            }
        }
    }

    exit(EXIT_SUCCESS);
}