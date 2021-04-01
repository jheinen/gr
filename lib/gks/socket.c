#ifdef __unix__
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#else
#include <windows.h>
#endif

#include "gks.h"
#include "gkscore.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#define PORT 8410

typedef struct
{
  int s;
  int wstype;
  gks_display_list_t dl;
} ws_state_list;

static gks_state_list_t *gkss;

static int is_running = 0;

#ifdef _WIN32

static DWORD WINAPI gksqt_tread(LPVOID parm)
{
  char *cmd = (char *)parm;
  wchar_t *w_cmd;
  STARTUPINFOW startupInfo = {0};
  PROCESS_INFORMATION processInformation = {NULL, NULL, 0, 0};
  int len = strlen(cmd);
  int w_len = MultiByteToWideChar(CP_UTF8, 0, cmd, len, NULL, 0);

  w_cmd = (wchar_t *)gks_malloc(sizeof(wchar_t) * w_len);

  MultiByteToWideChar(CP_UTF8, 0, cmd, len, w_cmd, w_len);
  startupInfo.cb = sizeof(startupInfo);

  is_running = 1;
  CreateProcessW(NULL, w_cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | DETACHED_PROCESS, NULL, NULL, &startupInfo,
                 &processInformation);
  WaitForSingleObject(processInformation.hThread, INFINITE);
  is_running = 0;

  CloseHandle(processInformation.hProcess);
  CloseHandle(processInformation.hThread);

  free(w_cmd);

  return 0;
}

#else

static void *gksqt_tread(void *arg)
{
  is_running = 1;
  system((char *)arg);
  is_running = 0;
  return NULL;
}

#endif

static int start(const char *cmd)
{
#ifdef _WIN32
  DWORD thread;

  if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gksqt_tread, (void *)cmd, 0, &thread) == NULL) return -1;
#else
  pthread_t thread;

  if (pthread_create(&thread, NULL, gksqt_tread, (void *)cmd)) return -1;
#endif
  return 0;
}

static int connect_socket(int quiet)
{
  int s;
  char *env;
  struct hostent *hp;
  struct sockaddr_in sin;
  int opt;

#if defined(_WIN32)
  WORD wVersionRequested = MAKEWORD(1, 1);
  WSADATA wsaData;

  if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
      fprintf(stderr, "Can't find a usable WinSock DLL\n");
      return -1;
    }
#endif

  s = socket(PF_INET,      /* get a socket descriptor */
             SOCK_STREAM,  /* stream socket           */
             IPPROTO_TCP); /* use TCP protocol        */
  if (s == -1)
    {
      if (!quiet) perror("socket");
      return -1;
    }

  opt = 1;
#ifdef SO_REUSEADDR
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
#endif

  env = (char *)gks_getenv("GKS_CONID");
  if (env)
    if (!*env) env = NULL;
  if (!env) env = (char *)gks_getenv("GKSconid");

  if ((hp = gethostbyname(env != NULL ? env : "127.0.0.1")) == 0)
    {
      if (!quiet) perror("gethostbyname");
      return -1;
    }

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;
  sin.sin_port = htons(PORT);

  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
      if (!quiet) perror("connect");
      return -1;
    }

  return s;
}

static int open_socket(int wstype)
{
  const char *command = NULL, *env;
  int retry_count;
  int max_retry_count = 10;
  char *cmd = NULL;
  int s;

  if (wstype == 411)
    {
      command = gks_getenv("GKS_QT");
      if (command == NULL)
        {
          env = gks_getenv("GRDIR");
          if (env == NULL) env = GRDIR;

          cmd = (char *)gks_malloc(MAXPATHLEN);
#ifndef _WIN32
#ifdef __APPLE__
          sprintf(cmd, "%s/Applications/gksqt.app/Contents/MacOS/gksqt", env);
#else
          sprintf(cmd, "%s/bin/gksqt", env);
#endif
#else
          sprintf(cmd, "%s\\bin\\gksqt.exe", env);
#endif
          command = cmd;
        }
    }

  for (retry_count = 1; retry_count <= max_retry_count; retry_count++)
    {
      if ((s = connect_socket(retry_count != 10)) == -1)
        {
          if (command != NULL && retry_count == 1)
            {
              /* For Julia BinaryBuilder environments the command string can be set to ""
                 because in this case gksqt is started by the GR.jl wrapper script */
              if (*command)
                {
                  if (start(command) != 0) gks_perror("could not auto-start GKS Qt application");
                }
            }
#ifndef _WIN32
          {
            struct timespec delay = {0, 300000000};
            nanosleep(&delay, NULL);
          }
#else
          Sleep(300);
#endif
        }
      else
        break;
    }

  is_running = (retry_count <= max_retry_count);

  if (cmd != NULL) free(cmd);

  return s;
}

static int send_socket(int s, char *buf, int size)
{
  int sent, n = 0;

  for (sent = 0; sent < size; sent += n)
    {
      if ((n = send(s, buf + sent, size - sent, 0)) == -1)
        {
          perror("send");
          is_running = 0;
          return -1;
        }
    }
  return sent;
}

static int read_socket(int s, char *buf, int size)
{
  int read, n = 0;
  for (read = 0; read < size; read += n)
    {
      if ((n = recv(s, buf + read, size - read, 0)) == -1)
        {
          perror("read");
          is_running = 0;
          return -1;
        }
    }
  return read;
}

static int close_socket(int s)
{
#if defined(_WIN32)
  closesocket(s);
#else
  close(s);
#endif
#if defined(_WIN32) && !defined(__GNUC__)
  WSACleanup();
#endif
  return 0;
}

void gks_drv_socket(int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc,
                    char *chars, void **ptr)
{
  ws_state_list *wss;

  wss = (ws_state_list *)*ptr;

  switch (fctid)
    {
    case 2:
      gkss = (gks_state_list_t *)*ptr;
      wss = (ws_state_list *)gks_malloc(sizeof(ws_state_list));

      wss->wstype = ia[2];
      wss->s = open_socket(ia[2]);
      if (wss->s == -1)
        {
          gks_perror("can't connect to GKS socket application\n");

          gks_free(wss);
          wss = NULL;

          ia[0] = ia[1] = 0;
        }
      else
        {
          *ptr = wss;
          if (wss->wstype == 411)
            {
              /* get workstation information */
              int nbytes;
              struct
              {
                int nbytes;
                double mwidth;
                double mheight;
                int width;
                int height;
                char name[6];
              } workstation_information = {sizeof(workstation_information), 0, 0, 0, 0, ""};
              if (read_socket(wss->s, (char *)&nbytes, sizeof(int)) == sizeof(int) &&
                  nbytes == workstation_information.nbytes)
                {
                  read_socket(wss->s, (char *)&workstation_information + sizeof(int), nbytes - (int)sizeof(int));
                  ia[0] = workstation_information.width;
                  ia[1] = workstation_information.height;
                  r1[0] = workstation_information.mwidth;
                  r2[0] = workstation_information.mheight;
                }
            }
        }
      break;

    case 3:
      if (wss->wstype == 411)
        {
          int null = 0;
          send_socket(wss->s, (char *)&null, sizeof(int));
        }
      close_socket(wss->s);
      if (wss->dl.buffer)
        {
          free(wss->dl.buffer);
        }
      gks_free(wss);
      wss = NULL;
      break;

    case 8:
      if (ia[1] & GKS_K_PERFORM_FLAG)
        {
          if (!is_running)
            {
              close_socket(wss->s);
              wss->s = open_socket(wss->wstype);
              if (wss->s != -1 && wss->wstype == 411)
                {
                  /* workstation information was already read during OPEN_WS */
                  int nbytes;
                  if (read_socket(wss->s, (char *)&nbytes, sizeof(int)) == sizeof(int))
                    {
                      char *buf = gks_malloc(nbytes - (int)sizeof(int));
                      read_socket(wss->s, buf, nbytes - (int)sizeof(int));
                      gks_free(buf);
                    }
                }
            }
          send_socket(wss->s, (char *)&wss->dl.nbytes, sizeof(int));
          send_socket(wss->s, wss->dl.buffer, wss->dl.nbytes);
        }
      break;
    }

  if (wss != NULL)
    {
      gks_dl_write_item(&wss->dl, fctid, dx, dy, dimx, ia, lr1, r1, lr2, r2, lc, chars, gkss);
    }
}
