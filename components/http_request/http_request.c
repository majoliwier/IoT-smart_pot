#include "http_request.h"

void http_get_request(const char *host)
{
    const char *TAG = "HTTP_GET";
    char request[128];
    char recv_buf[1024];
    int sockfd, ret;
    struct addrinfo hints, *res;

    snprintf(request, sizeof(request),
             "GET / HTTP/1.0\r\n"
             "Host: %s\r\n"
             "\r\n",
             host);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        
    hints.ai_socktype = SOCK_STREAM;

    // rozwiazywanie nazwy hosta na adres ip
    ret = getaddrinfo(host, "80", &hints, &res);
    if (ret != 0 || res == NULL)
    {
        ESP_LOGE(TAG, "Błąd DNS: %d", ret);
        return;
    }

    // tworzenie socketu
    sockfd = socket(res->ai_family, res->ai_socktype, 0);
    if (sockfd < 0)
    {
        ESP_LOGE(TAG, "Nie można utworzyć gniazda");
        freeaddrinfo(res);
        return;
    }

    // laczenie z serwerem
    ret = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Nie można połączyć się z serwerem");
        close(sockfd);
        freeaddrinfo(res);
        return;
    }

    freeaddrinfo(res);

    // wysylanie get
    ret = send(sockfd, request, strlen(request), 0);
    if (ret < 0)
    {
        ESP_LOGE(TAG, "Błąd wysyłania danych");
        close(sockfd);
        return;
    }

    // odbieranie odpowiedzi
    do
    {
        ret = recv(sockfd, recv_buf, sizeof(recv_buf) - 1, 0);
        if (ret > 0)
        {
            recv_buf[ret] = '\0';  
            printf("%s", recv_buf);
        }
    } while (ret > 0);

    if (ret < 0)
    {
        ESP_LOGE(TAG, "Błąd odbierania danych");
    }

    close(sockfd);
}
