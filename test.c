#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct s_client {
	int id;
	char msg[11000];
}				t_client;
t_client g_c[4096];
int max, next__id = 0;
fd_set curr, r_read, r_write;
char r_buff[127032], w_buff[127032];

void putstr_fd(int fd, char *str) {
	int i = 0;
	while(str[i])
		i++;
	write(fd, str, i);
}

void send_all(int src) {
	for (int i = 0; i <= max; i++) {
		if (FD_ISSET(i, &r_write) && i != src)
			send(i, w_buff, strlen(w_buff), 0);
	}
}

int main(int ac, char **av) {

	if (ac != 2) {
		putstr_fd(2, "error\n");
		exit(1);
	}
	bzero(&g_c, sizeof(g_c));
	FD_ZERO(&curr);

	int sockfd, connfd;
	socklen_t len;
	struct sockaddr_in servaddr; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		putstr_fd(2, "error\n");
		exit(1);
	} 

	max = sockfd;
	FD_SET(sockfd, &curr);

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		putstr_fd(2, "error\n");
		exit(1);
	} 

	if (listen(sockfd, 128) != 0) {
		putstr_fd(2, "error\n");
		exit(1);
	}

	while(1) {
		r_read = r_write = curr;
		if (select(max+1, &r_read, &r_write, NULL, NULL) < 0)
			continue;
		for (int s = 0; s <= max; s++) {
			if (FD_ISSET(s, &r_read) && s == sockfd) {
				len = sizeof(servaddr);
				connfd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
				if (connfd < 0) { 
					continue;
				}
				max = connfd > max ? connfd : max;
				g_c[connfd].id = next__id++;
				FD_SET(connfd, &curr);
				sprintf(w_buff, "server: client %d just arrived\n", g_c[connfd].id);
				send_all(connfd);
				break ;
			}

			if (FD_ISSET(s, &r_read) && s != sockfd) {
				int	res = recv(s, r_buff, 127032, 0);
				if (res <= 0)
				{
					sprintf(w_buff, "server: client %d just left\n", g_c[s].id);
					send_all(s);
					FD_CLR(s, &curr);
					close(s);
					break ;
				}
				else
				{
					for (int i = 0, x = strlen(g_c[s].msg); i < res; i++, x++)
					{
						g_c[s].msg[x] = r_buff[i];
						if (g_c[s].msg[x] == '\n')
						{
							g_c[s].msg[x] = '\0';
							sprintf(w_buff, "client %d: %s\n", g_c[s].id, g_c[s].msg);
							send_all(s);
							bzero(&g_c[s].msg, strlen(g_c[s].msg));
							x = -1;
						}
					}
					break ;
				}
			
			}
		}
	}
	return (0);
}