#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>

#include "libhttp.h"
#include "wq.h"

#define LISTEN_BACKLOG 1024

/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
wq_t work_queue; // Only used by poolserver
int num_threads; // Only used by poolserver
int server_port; // Default value: 8000
char* server_files_directory;
char* server_proxy_hostname;
int server_proxy_port;

/*
 * Serves the contents the file stored at `path` to the client socket `fd`.
 * It is the caller's reponsibility to ensure that the file stored at `path` exists.
 */
void serve_file(int fd, char* path) {

  /* TODO: PART 2 */
  /* PART 2 BEGIN */

  int fd_file = open(path, O_RDONLY);
  if (fd_file == -1) {
    http_start_response(fd, 500);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    return;
  }

  ssize_t total_bytes_read = 0;
  ssize_t bytes_per_read = 1024;
  char* buffer = NULL;

  while (1) {
    char temp[bytes_per_read];
    ssize_t bytes_read = read(fd_file, temp, bytes_per_read);
    if (bytes_read == -1) {
      perror("Failed to read file at path");
      close(fd_file);
      free(buffer);
      http_start_response(fd, 500);
      http_send_header(fd, "Content-Type", "text/html");
      http_end_headers(fd);
      return;
    }

    if (bytes_read == 0) {
      break; // EOF
    }

    char* temp_buffer = realloc(buffer, total_bytes_read + bytes_read);
    if (temp_buffer == NULL) {
      perror("Failed to realloc buffer");
      close(fd_file);
      free(buffer);
      http_start_response(fd, 500);
      http_send_header(fd, "Content-Type", "text/html");
      http_end_headers(fd);
      return;
    }
    buffer = temp_buffer;

    memcpy(buffer + total_bytes_read, temp, bytes_read);
    total_bytes_read += bytes_read;
  }

  close(fd_file);

  char content_length_str[32];
  snprintf(content_length_str, sizeof(content_length_str), "%zu", total_bytes_read);

  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", http_get_mime_type(path));
  http_send_header(fd, "Content-Length", content_length_str);
  http_end_headers(fd);
  ssize_t total_written = 0;
  while (total_written < total_bytes_read) {
    ssize_t bytes_written = write(fd, buffer + total_written, total_bytes_read - total_written);
    if (bytes_written == -1) {
      perror("write failed");
      break;
    }
    total_written += bytes_written;
  }

  free(buffer);
  /* PART 2 END */
}

void serve_directory(int fd, char* path) {
  /* TODO: PART 3 */
  /* PART 3 BEGIN */
  DIR* dirp = opendir(path);
  if (dirp == NULL) {
    http_start_response(fd, 500);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    return;
  }

  int dir_contains_index_html = 0;
  struct dirent* entry;

  while ((entry = readdir(dirp)) != NULL) {
    if (strcmp(entry->d_name, "index.html") == 0) {
      dir_contains_index_html = 1;
      break;
    }
  }

  if (dir_contains_index_html) {
    /* Add `/index.html` to the end of the path */
    char* index_html = "/index.html";
    size_t path_len = strlen(path);
    size_t index_len = strlen(index_html);

    char* path_including_index_html = malloc(path_len + index_len + 1);
    if (path_including_index_html == NULL) {
      http_start_response(fd, 500);
      http_send_header(fd, "Content-Type", "text/html");
      http_end_headers(fd);
      closedir(dirp);
      return;
    }

    memcpy(path_including_index_html, path, path_len);
    memcpy(path_including_index_html + path_len, index_html, index_len + 1);

    serve_file(fd, path_including_index_html);

    free(path_including_index_html);

    closedir(dirp);
    return;
  }

  rewinddir(dirp);
  const size_t initial_capacity = 8192;
  char* buffer = malloc(initial_capacity);
  if (buffer == NULL) {
    http_start_response(fd, 500);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    closedir(dirp);
  }

  size_t length = 0;
  size_t capacity = initial_capacity;

  length += snprintf(buffer, capacity, "<html><head><title>Index of %s</title></head><body>", path);
  length += snprintf(buffer + length, capacity - length, "<h1>Index of %s</h1><ul>", path);

  /* Buffer for each formatted href */
  char href_buffer[1024];

  /* Parent directory link */
  http_format_href(href_buffer, path, "..");

  /* Listing directory entries hrefs */
  while ((entry = readdir(dirp)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    http_format_href(href_buffer, path, entry->d_name);
    length += snprintf(buffer + length, capacity - length, "<li>%s</li>", href_buffer);
  }

  length += snprintf(buffer + length, capacity - length, "</ul></body></html>");

  closedir(dirp);

  char len_buf[32];
  snprintf(len_buf, sizeof(len_buf), "%zu", length);

  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", "text/html");
  http_send_header(fd, "Content-Length", len_buf);
  http_end_headers(fd);

  ssize_t total_written = 0;
  while (total_written < length) {
    ssize_t bytes_written = write(fd, buffer + total_written, length - total_written);
    if (bytes_written == -1) {
      perror("write failed");
      break;
    }
    total_written += bytes_written;
  }

  free(buffer);
  /* PART 3 END */
}

/*
 * Reads an HTTP request from client socket (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 *
 *   Closes the client socket (fd) when finished.
 */
void handle_files_request(int fd) {

  struct http_request* request = http_request_parse(fd);

  if (request == NULL || request->path[0] != '/') {
    http_start_response(fd, 400);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(fd);
    return;
  }

  if (strstr(request->path, "..") != NULL) {
    http_start_response(fd, 403);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(fd);
    return;
  }

  /* Add `./` to the beginning of the requested path */
  char* path = malloc(2 + strlen(request->path) + 1);
  path[0] = '.';
  path[1] = '/';
  memcpy(path + 2, request->path, strlen(request->path) + 1);

  /*
   * TODO: PART 2 is to serve files. If the file given by `path` exists,
   * call serve_file() on it. Else, serve a 404 Not Found error below.
   * The `stat()` syscall will be useful here.
   *
   * TODO: PART 3 is to serve both files and directories. You will need to
   * determine when to call serve_file() or serve_directory() depending
   * on `path`. Make your edits below here in this function.
   */

  /* PART 2 & 3 BEGIN */

  /* Check if file exists at path */
  struct stat buffer;
  if (stat(path, &buffer) == -1) {
    http_start_response(fd, 404);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(fd);
    return;
  }

  if (S_ISREG(buffer.st_mode) != 0) {
    serve_file(fd, path);
  }

  else if (S_ISDIR(buffer.st_mode) != 0) {
    serve_directory(fd, path);
  }

  /* PART 2 & 3 END */

  close(fd);
  return;
}

struct proxy_fd {
  int from_fd;
  int to_fd;
  int* is_done;
  pthread_mutex_t* done_mutex;
};

void* forward_bytes(void* arg) {
  struct proxy_fd* fds = (struct proxy_fd*)arg;
  char buffer[4096];

  while (1) {
    pthread_mutex_lock(fds->done_mutex);
    if (*(fds->is_done)) {
      pthread_mutex_unlock(fds->done_mutex);
      break;
    }
    pthread_mutex_unlock(fds->done_mutex);

    ssize_t bytes_read = read(fds->from_fd, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
      pthread_mutex_lock(fds->done_mutex);
      if (*(fds->is_done) == 0) {
        *(fds->is_done) = 1;
        shutdown(fds->from_fd, SHUT_RDWR);
        shutdown(fds->to_fd, SHUT_RDWR);
      }
      pthread_mutex_unlock(fds->done_mutex);
      break;
    }

    ssize_t total_written = 0;
    while (total_written < bytes_read) {
      ssize_t bytes_written = write(fds->to_fd, buffer + total_written, bytes_read - total_written);
      if (bytes_written <= 0) {
        pthread_mutex_lock(fds->done_mutex);
        if (*(fds->is_done) == 0) {
          *(fds->is_done) = 1;
          shutdown(fds->from_fd, SHUT_RDWR);
          shutdown(fds->to_fd, SHUT_RDWR);
        }
        pthread_mutex_unlock(fds->done_mutex);
        break;
      }
      total_written += bytes_written;
    }
  }

  return NULL;
}

/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target_fd. HTTP requests from the client (fd) should be sent to the
 * proxy target (target_fd), and HTTP responses from the proxy target (target_fd)
 * should be sent to the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 *
 *   Closes client socket (fd) and proxy target fd (target_fd) when finished.
 */
void handle_proxy_request(int fd) {
  /*
  * The code below does a DNS lookup of server_proxy_hostname and
  * opens a connection to it. Please do not modify.
  */
  struct sockaddr_in target_address;
  memset(&target_address, 0, sizeof(target_address));
  target_address.sin_family = AF_INET;
  target_address.sin_port = htons(server_proxy_port);

  // Use DNS to resolve the proxy target's IP address
  struct hostent* target_dns_entry = gethostbyname2(server_proxy_hostname, AF_INET);

  // Create an IPv4 TCP socket to communicate with the proxy target.
  int target_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (target_fd == -1) {
    fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
    close(fd);
    exit(errno);
  }

  if (target_dns_entry == NULL) {
    fprintf(stderr, "Cannot find host: %s\n", server_proxy_hostname);
    close(target_fd);
    close(fd);
    exit(ENXIO);
  }

  char* dns_address = target_dns_entry->h_addr_list[0];

  // Connect to the proxy target.
  memcpy(&target_address.sin_addr, dns_address, sizeof(target_address.sin_addr));
  int connection_status =
      connect(target_fd, (struct sockaddr*)&target_address, sizeof(target_address));

  if (connection_status < 0) {
    /* Dummy request parsing, just to be compliant. */
    http_request_parse(fd);

    http_start_response(fd, 502);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(target_fd);
    close(fd);
    return;
  }

  /* TODO: PART 4 */
  /* PART 4 BEGIN */
  struct proxy_fd* client_forward = malloc(sizeof(struct proxy_fd));
  if (client_forward == NULL) {
    /* Dummy request parsing, just to be compliant. */
    http_request_parse(fd);

    http_start_response(fd, 502);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    close(target_fd);
    close(fd);
    return;
  }
  struct proxy_fd* target_forward = malloc(sizeof(struct proxy_fd));
  if (target_forward == NULL) {
    /* Dummy request parsing, just to be compliant. */
    http_request_parse(fd);

    http_start_response(fd, 502);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    free(client_forward);
    close(target_fd);
    close(fd);
    return;
  }

  int is_done_val = 0;
  pthread_mutex_t is_done_mutex = PTHREAD_MUTEX_INITIALIZER;

  client_forward->from_fd = fd;
  client_forward->to_fd = target_fd;
  client_forward->done_mutex = &is_done_mutex;
  client_forward->is_done = &is_done_val;

  target_forward->from_fd = target_fd;
  target_forward->to_fd = fd;
  target_forward->done_mutex = &is_done_mutex;
  target_forward->is_done = &is_done_val;

  pthread_t thread_id_client_forward;
  pthread_t thread_id_target_forward;

  pthread_create(&thread_id_client_forward, NULL, forward_bytes, client_forward);
  pthread_create(&thread_id_target_forward, NULL, forward_bytes, target_forward);

  pthread_join(thread_id_client_forward, NULL);
  pthread_join(thread_id_target_forward, NULL);

  close(target_fd);
  close(fd);

  free(client_forward);
  free(target_forward);
  /* PART 4 END */
}

#ifdef POOLSERVER
/*
 * All worker threads will run this function until the server shutsdown.
 * Each thread should block until a new request has been received.
 * When the server accepts a new connection, a thread should be dispatched
 * to send a response to the client.
 */
void* handle_clients(void* void_request_handler) {
  void (*request_handler)(int) = (void (*)(int))void_request_handler;
  /* (Valgrind) Detach so thread frees its memory on completion, since we won't
   * be joining on it. */
  pthread_detach(pthread_self());

  /* TODO: PART 7 */
  /* PART 7 BEGIN */
  int client_socket_number;
  while((client_socket_number = wq_pop(&work_queue))){
    request_handler(client_socket_number);
  }
  return NULL;
  /* PART 7 END */
}

/*
 * Creates `num_threads` amount of threads. Initializes the work queue.
 */
void init_thread_pool(int num_threads, void (*request_handler)(int)) {

  /* TODO: PART 7 */
  /* PART 7 BEGIN */
  wq_init(&work_queue);
  work_queue.num_threads = num_threads;
  work_queue.threads = malloc(sizeof(pthread_t) * num_threads);
  if(work_queue.threads == NULL){
    perror("Failed to allocate thread pool");
    exit(1);
  }
  
  for(size_t i = 0; i < num_threads; i++){
    pthread_create(&work_queue.threads[i], NULL, handle_clients, request_handler);
  }

  /* PART 7 END */
}
#endif

struct thread_request_handler {
  int client_socket;
  void (*request_handler)(int);
};

void* handle_request_thread(void* arg) {
  struct thread_request_handler* h = (struct thread_request_handler*)arg;
  h->request_handler(h->client_socket);
  free(h);
  return NULL;
}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int* socket_number, void (*request_handler)(int)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;

  // Creates a socket for IPv4 and TCP.
  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) ==
      -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  // Setup arguments for bind()
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  /*
   * TODO: PART 1
   *
   * Given the socket created above, call bind() to give it
   * an address and a port. Then, call listen() with the socket.
   * An appropriate size of the backlog is 1024, though you may
   * play around with this value during performance testing.
   */

  /* PART 1 BEGIN */
  if (bind(*socket_number, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
    perror("Failed to bind socket.");
    exit(errno);
  }

  if (listen(*socket_number, LISTEN_BACKLOG) == -1) {
    perror("Failed to accept connections on socket");
    exit(errno);
  }

  /* PART 1 END */
  printf("Listening on port %d...\n", server_port);

#ifdef POOLSERVER
  /*
   * The thread pool is initialized *before* the server
   * begins accepting client connections.
   */
  init_thread_pool(num_threads, request_handler);
#endif

  while (1) {
    client_socket_number = accept(*socket_number, (struct sockaddr*)&client_address,
                                  (socklen_t*)&client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n", inet_ntoa(client_address.sin_addr),
           client_address.sin_port);

#ifdef BASICSERVER
    /*
     * This is a single-process, single-threaded HTTP server.
     * When a client connection has been accepted, the main
     * process sends a response to the client. During this
     * time, the server does not listen and accept connections.
     * Only after a response has been sent to the client can
     * the server accept a new connection.
     */
    request_handler(client_socket_number);

#elif FORKSERVER
    /*
     * TODO: PART 5
     *
     * When a client connection has been accepted, a new
     * process is spawned. This child process will send
     * a response to the client. Afterwards, the child
     * process should exit. During this time, the parent
     * process should continue listening and accepting
     * connections.
     */

    /* PART 5 BEGIN */
    pid_t pid;
    pid = fork();

    if (pid < 0) {
      perror("Error forking process");
      continue;
    } else if (pid == 0) {
      request_handler(client_socket_number);
      shutdown(client_socket_number, SHUT_RDWR);
      close(client_socket_number);
      exit(0);
    }

    /* PART 5 END */

#elif THREADSERVER
    /*
     * TODO: PART 6
     *
     * When a client connection has been accepted, a new
     * thread is created. This thread will send a response
     * to the client. The main thread should continue
     * listening and accepting connections. The main
     * thread will NOT be joining with the new thread.
     */

    /* PART 6 BEGIN */
    struct thread_request_handler* h = malloc(sizeof(struct thread_request_handler));
    if (h == NULL) {
      perror("Failed to allocate thread responsible for handling client connection");
      continue;
    }
    h->client_socket = client_socket_number;
    h->request_handler = request_handler;

    pthread_t tid;
    pthread_create(&tid, NULL, handle_request_thread, h);
    pthread_detach(tid);

    /* PART 6 END */
#elif POOLSERVER
    /*
     * TODO: PART 7
     *
     * When a client connection has been accepted, add the
     * client's socket number to the work queue. A thread
     * in the thread pool will send a response to the client.
     */

    /* PART 7 BEGIN */
    wq_push(&work_queue, client_socket_number);

    /* PART 7 END */
#endif
  }

  shutdown(*socket_number, SHUT_RDWR);
  close(*socket_number);
}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0)
    perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char* USAGE =
    "Usage: ./httpserver --files some_directory/ [--port 8000 --num-threads 5]\n"
    "       ./httpserver --proxy inst.eecs.berkeley.edu:80 [--port 8000 --num-threads 5]\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
  signal(SIGINT, signal_callback_handler);
  signal(SIGPIPE, SIG_IGN);

  /* Default settings */
  server_port = 8000;
  void (*request_handler)(int) = NULL;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char* proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char* colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char* server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--num-threads", argv[i]) == 0) {
      char* num_threads_str = argv[++i];
      if (!num_threads_str || (num_threads = atoi(num_threads_str)) < 1) {
        fprintf(stderr, "Expected positive integer after --num-threads\n");
        exit_with_usage();
      }
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL) {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }

#ifdef POOLSERVER
  if (num_threads < 1) {
    fprintf(stderr, "Please specify \"--num-threads [N]\"\n");
    exit_with_usage();
  }
#endif

  chdir(server_files_directory);
  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
