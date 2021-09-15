#include <stdio.h>  /* printf, sprintf */
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <iostream>
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "json.hpp"
#include "helpers.h"
#include "requests.h"

using namespace std;
using namespace nlohmann;

#define PORT 8080
#define APPLICATION "application/json"
#define IP_STRING "34.118.48.238"

// Functia extrage cookies din raspunsul primit de la login
char *extract_cookies(char *response)
{
    char *cookie_line = strstr(response, "Set-Cookie: ");
    char *cookie = strtok(cookie_line, ";");

    int lenght = strlen("Set-Cookie: ");
    cookie += lenght;

    return cookie;
}

// Functie care extrage JWT-token din raspunsul primit la enter_library
char *extract_jwt(char *jwt_response)
{
    json token = json::parse(jwt_response);
    if (token["token"] != NULL)
    {
        char *jwt = strdup(token["token"].dump().c_str());
        char *token = strtok(jwt, "\"");

        return token;
    }
    else
    {
        return NULL;
    }
}

int main(int argc, char *argv[])
{
    int sockfd;
    char *message;
    char *cookie = NULL;
    char *jwt_token = NULL;

    char inputCommand[30];

    while (1)
    {
        // Citesc comanda de la stdin
        fgets(inputCommand, 30, stdin);

        if (strcmp(inputCommand, "register\n") == 0)
        {
            json js;
            string username, password;

            // Afisez si citesc username si password
            cout << "username=";
            std::getline(cin, username);
            cout << "password=";
            std::getline(cin, password);

            // Ii setez in fromatul json conform bibliotecii nlohman
            js["username"] = username;
            js["password"] = password;

            // Mesajul care urmeaza sa fie compus
            char *json_send = strdup(js.dump().c_str());

            /* Deschid conexiunea, construiesc mesajul cu compute_post_request 
            si /auth/register trimit la server */
            sockfd = open_connection(IP_STRING, 8080, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(IP_STRING, "/api/v1/tema/auth/register",
                                           APPLICATION, json_send, NULL, NULL);
            send_to_server(sockfd, message);

            // Primesc si afisez raspunsul
            char *response = receive_from_server(sockfd);
            puts(response);

            // Dealocari si inchidere conexiune
            free(message);
            free(json_send);
            close_connection(sockfd);
        }

        else if (strcmp(inputCommand, "login\n") == 0)
        {
            // Fac acelasi lucru ca in functie precedenta
            json js;
            string username;
            string password;
            cout << "username=";
            std::getline(cin, username);
            cout << "password=";
            std::getline(cin, password);

            js["username"] = username;
            js["password"] = password;

            char *json_send = strdup(js.dump().c_str());

            /* Deschid conexiunea, compun mesajul cu /auth/login */
            sockfd = open_connection(IP_STRING, 8080, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(IP_STRING, "/api/v1/tema/auth/login", APPLICATION, json_send, NULL, NULL);
            send_to_server(sockfd, message);

            char *response = receive_from_server(sockfd);
            puts(response);

            /* Fac verificarea daca credentiale sunt corecte, adica daca 
            basic_extract_json_response(response) nu da NULL extract coookies alftefl nu extrag,
            pentru a evita eroarea mentionata in enunt */
            char *extract_response = basic_extract_json_response(response);
            free(message);

            if (extract_response == NULL)
            {
                free(json_send);
                cookie = extract_cookies(response);
                close_connection(sockfd);
            }
            else
            {
                free(json_send);
                close_connection(sockfd);
            }
        }

        else if (strcmp(inputCommand, "enter_library\n") == 0)
        {
            // Verific daca userul este logat
            if (cookie != NULL)
            {
                /* Deschid conerxiunea, compun mesajul cu get_request si /library/acces
                 si adaug si cookie-uri pentru identificarea clientului */
                sockfd = open_connection(IP_STRING, 8080, AF_INET, SOCK_STREAM, 0);
                message = compute_get_request(IP_STRING, "/api/v1/tema/library/access",
                                              0, cookie, NULL);
                send_to_server(sockfd, message);

                char *response = receive_from_server(sockfd);
                char *extract_response = basic_extract_json_response(response);
                puts(response);

                // Extrag JWT-ul din rasouns
                jwt_token = extract_jwt(extract_response);

                free(message);
                close_connection(sockfd);
            }
            else
            {
                // Daca cookies = NULL nu e connectat nimeni
                cout << "[Error]: Must login\n";
            }
        }

        else if (strcmp(inputCommand, "get_books\n") == 0)
        {
            // Verific daca utilizatorul a intrat in biblioteca
            if (jwt_token != NULL)
            {
                /* Deschid conexiunea, compun mesajul get cu library/books, cookie si jwt*/
                sockfd = open_connection(IP_STRING, 8080, AF_INET, SOCK_STREAM, 0);
                message = compute_get_request(IP_STRING, "/api/v1/tema/library/books", 0, cookie, jwt_token);
                send_to_server(sockfd, message);

                char *response = receive_from_server(sockfd);

                cout << "\n"
                     << response << "\n";

                free(message);
                close_connection(sockfd);
            }
            else
            {
                cout << "[Error]: don't have access to library";
            }
        }

        else if (strcmp(inputCommand, "get_book\n") == 0)
        {
            if (jwt_token != NULL)
            {
                string id;
                // Varibile pentru a compune calea catre carte
                string url = "/api/v1/tema/library/books/";

                // Afisez si citesc id
                cout << "id=";
                std::getline(cin, id);

                // Fac get_rquest in care si construesc calea completa cu append
                sockfd = open_connection(IP_STRING, 8080, AF_INET, SOCK_STREAM, 0);
                message = compute_get_request(IP_STRING, (char *)url.append(id).c_str(), 0, cookie, jwt_token);
                send_to_server(sockfd, message);

                char *response = receive_from_server(sockfd);

                puts(response);
            }
            else
            {
                cout << "[Error]: don't have access to library";
            }
        }

        else if (strcmp(inputCommand, "add_book\n") == 0)
        {
            if (jwt_token != NULL)
            {
                json js;
                string title, author, publisher, genre, page_count;

                // Afisez si citesc fiecare camp necesar pentru o carte
                cout << "title=";
                std::getline(cin, title);
                cout << "author=";
                std::getline(cin, author);
                cout << "publisher=";
                std::getline(cin, publisher);
                cout << "genre=";
                std::getline(cin, genre);
                cout << "page_count=";
                std::getline(cin, page_count);

                // Le setez ca json
                js["title"] = title;
                js["author"] = author;
                js["publisher"] = publisher;
                js["genre"] = genre;
                js["page_count"] = page_count;

                char *json_send = strdup(js.dump().c_str());

                // Compun si trimis mesajul
                sockfd = open_connection(IP_STRING, 8080, AF_INET, SOCK_STREAM, 0);
                message = compute_post_request(IP_STRING, "/api/v1/tema/library/books", APPLICATION, json_send, NULL, jwt_token);
                puts(message);
                send_to_server(sockfd, message);

                char *response = receive_from_server(sockfd);
                puts(response);

                free(message);
                free(json_send);
                close_connection(sockfd);
            }
            else
            {
                cout << "[Error]: don't have access to library";
            }
        }

        else if (strcmp(inputCommand, "delete_book\n") == 0)
        {
            if (jwt_token != NULL)
            {
                // Citesc id-ul
                string id;
                string url = "/api/v1/tema/library/books/";

                cout << "id=";
                std::getline(cin, id);

                // Compun mesajul cu o noua functie de request "delete"
                sockfd = open_connection(IP_STRING, 8080, AF_INET, SOCK_STREAM, 0);
                message = compute_delete_request(IP_STRING, (char *)url.append(id).c_str(), 0, cookie, jwt_token);
                send_to_server(sockfd, message);

                char *response = receive_from_server(sockfd);

                puts(response);
            }
            else
            {
                cout << "[Error]: don't have access to library";
            }
        }

        else if (strcmp(inputCommand, "logout\n") == 0)
        {
            sockfd = open_connection(IP_STRING, 8080, AF_INET, SOCK_STREAM, 0);

            // Compun mesajul cu logout
            message = compute_get_request(IP_STRING, "/api/v1/tema/auth/logout", 0, cookie, NULL);

            send_to_server(sockfd, message);

            char *response = receive_from_server(sockfd);

            puts(response);

            // Setez cookie si jwt_token pe NULL
            cookie = NULL;
            jwt_token = NULL;

            close_connection(sockfd);
            free(message);
        }

        else if (strcmp(inputCommand, "exit\n") == 0)
        {
            // Fac break la while(1) si resetez cookies si jwt
            cookie = NULL;
            jwt_token = NULL;

            if (cookie != NULL)
                free(cookie);

            if (jwt_token != NULL)
                free(jwt_token);

            break;
        }
        else
        {
            cout << "[Error]: Invalid command\n";
        }
    }

    return 0;
}