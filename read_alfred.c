#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

bool read_data(FILE *file, int sock, struct sockaddr *server) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int32_t size = 0;

  while ((read = getline(&line, &len, file)) != -1) {
    if (strstr(line, "command not found") != NULL) {
      fprintf(stderr, "alfred was not found\n");
      return false;
    }
    else {
      char *c;
      char *tok = strtok(line, "\"");

      if (tok != NULL) {
        // Magic number: I know a priori it's three tokens in. should realy search for it to be robust
        for (int i = 0; i < 3 && tok != NULL; (tok = strtok(NULL, "\"")) && ++i);

        c = tok;
        tok = strtok(NULL, " ");

        size = tok - c;

        if (tok != NULL) {
          sendto(sock, c, size, 0, server, sizeof(struct sockaddr));
#ifdef DEBUG
          printf("data: %s\n", c);
          printf("%d\n", c - line);
          printf("%d\n", size);
#endif
        }
      }
    }

    free(line);
    line = NULL;
    len = 0;
  }

  return true;
}

void help(char program[]) {
   fprintf(stderr, "%s -h <hostname> -p <port> [ -a <alfred channel> ]\n\n", program);
   exit(EXIT_FAILURE);
}

int main (int argc, char **argv) {
  int sock;
  struct sockaddr_in server;

  FILE *proc = NULL;

  char c;
  uint16_t port = 0;
  char host[256] = { '\0' };

  const char alfred_bin[] = "alfred";
  int32_t alfred_channel = 64;
  char cmd[256];

  if (getuid() != 0) {
    fprintf(stderr, "not root\n");
    help(argv[0]);
  }

  if (argc < 3) {
    help(argv[0]);
  }

  while ((c = getopt(argc, argv, "o:p:a:h")) != -1) {
    switch (c) {
      case 'p':
        port = (uint16_t) atoi(optarg);
        break;

      case 'o':
        strncpy(host, optarg, 256);
        break;

      case 'a':
        alfred_channel = atoi(optarg);
        break;

      case 'h':
      case '*':
      case '?':
        help(argv[0]);
        break;
    }
  }

  if (port == 0 || strlen(host) == 0) {
    help(argv[0]);
  }

  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    return EXIT_FAILURE;
  }

  struct hostent *hp;
  if ((hp = gethostbyname(host)) == NULL) {
    return EXIT_FAILURE;
  }

  memcpy(&server.sin_addr, hp->h_addr_list[0], hp->h_length);
  server.sin_family = AF_INET;
  server.sin_port = htons((short) port);

  snprintf(cmd, 256, "%s -r %d 2>&1", alfred_bin, alfred_channel);
  proc = popen(cmd, "r");

  if (!proc) {
    fprintf(stderr, "couldn't open process pipe\n");
    close(sock);
    return EXIT_FAILURE;
  }

  if (read_data(proc, sock, (struct sockaddr*) &server)) {
    if (pclose(proc) != 0) {
      close(sock);
      fprintf (stderr, "error closing stream\n");
      return EXIT_FAILURE;
    }
  }
  else {
    pclose(proc);
    close(sock);
    return EXIT_FAILURE;
  }

  close(sock);
  return EXIT_SUCCESS;
}
