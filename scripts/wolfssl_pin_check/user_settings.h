/* Host mirror of the reader's wolfSSL configuration: Arduino-wolfSSL 5.7.2's
 * user_settings.h, the -DWOLFSSL_* flags in platformio.ini, and the overrides in
 * scripts/patch_wolfssl.py. Keep it in step with those; check.sh refuses to run if
 * platformio.ini lost -DWOLFSSL_SHA384. */
#define NO_FILESYSTEM
#define USE_CERT_BUFFERS_2048
#define HAVE_ECC
#define WOLFSSL_SMALL_STACK
#define MICRO_SESSION_CACHE
#define RSA_LOW_MEM
#define NO_OLD_TLS
#define WOLFSSL_TLS13
#define HAVE_TLS_EXTENSIONS
#define WC_RSA_PSS
#define HAVE_HKDF
#define HAVE_AEAD
#define HAVE_SUPPORTED_CURVES
#define NO_WOLFSSL_SERVER
#define NO_DSA
#define USE_FAST_MATH
#define SINGLE_THREADED
#define WOLFSSL_LOW_MEMORY
#define HAVE_AESGCM
#define HAVE_FFDHE_2048
#define HAVE_CURVE25519
#define HAVE_SNI
#define WOLFSSL_HAVE_SP_ECC
#define WOLFSSL_SP_SMALL
#undef FP_MAX_BITS
#define FP_MAX_BITS 8192
#define WOLFSSL_SHA384
#define WOLFSSL_IGNORE_FILE_WARN
