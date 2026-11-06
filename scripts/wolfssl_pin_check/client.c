// Host check for the pinned release hosts: connects the way the reader's
// SecureClient does (one PEM root loaded from a buffer, SNI, TLS 1.3 with an X25519
// key share) and prints OK or the wolfSSL error. See check.sh.
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <time.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/asn_public.h>

// FAKE_EPOCH (seconds) stands in for the reader's clock; unset means this computer's clock.
static long fakeEpoch = -1;
static time_t clockNow(time_t* t) {
  const time_t v = fakeEpoch < 0 ? time(NULL) : (time_t)fakeEpoch;
  if (t) *t = v;
  return v;
}

int main(int argc, char** argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s host root.pem tls13|tls12|auto\n", argv[0]);
    return 2;
  }
  const char* host = argv[1];
  const char* method = argv[3];

  FILE* f = fopen(argv[2], "rb");
  if (!f) return 2;
  static char pem[8192];
  const size_t pemLen = fread(pem, 1, sizeof(pem) - 1, f);
  fclose(f);

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo* res = NULL;
  if (getaddrinfo(host, "443", &hints, &res) != 0) {
    printf("ERROR dns\n");
    return 2;
  }
  const int fd = socket(res->ai_family, SOCK_STREAM, 0);
  if (connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
    printf("ERROR tcp\n");
    return 2;
  }

  if (getenv("FAKE_EPOCH")) fakeEpoch = atol(getenv("FAKE_EPOCH"));
  wc_SetTimeCb(clockNow);
  wolfSSL_Init();
  WOLFSSL_METHOD* m = !strcmp(method, "tls13")   ? wolfTLSv1_3_client_method()
                      : !strcmp(method, "tls12") ? wolfTLSv1_2_client_method()
                                                 : wolfSSLv23_client_method();
  WOLFSSL_CTX* ctx = wolfSSL_CTX_new(m);
  const int loaded = wolfSSL_CTX_load_verify_buffer(ctx, (const unsigned char*)pem, (long)pemLen, WOLFSSL_FILETYPE_PEM);
  if (loaded != WOLFSSL_SUCCESS) {
    printf("FAIL  root did not load (%d)\n", loaded);
    return 1;
  }
  WOLFSSL* ssl = wolfSSL_new(ctx);
  wolfSSL_set_fd(ssl, fd);
  wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, host, strlen(host));
  if (strcmp(method, "tls12") != 0) wolfSSL_UseKeyShare(ssl, WOLFSSL_ECC_X25519);

  const int r = wolfSSL_connect(ssl);
  if (r == WOLFSSL_SUCCESS) {
    printf("OK    %s %s\n", wolfSSL_get_version(ssl), wolfSSL_get_cipher(ssl));
    return 0;
  }
  const int e = wolfSSL_get_error(ssl, r);
  char buf[100];
  wolfSSL_ERR_error_string(e, buf);
  printf("FAIL  error %d (%s)\n", e, buf);
  return 1;
}
