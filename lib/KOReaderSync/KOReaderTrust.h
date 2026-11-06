#pragma once

#include <cstring>
#include <string>

// Certificate trust for the KOReader sync server. The firmware has no system
// certificate store, so each server it talks to needs a pinned root here.
//
// Only https is accepted, and only for a host listed below. Anything else is
// refused, so sync credentials are never sent over an unverified connection.
//
// Pinned root: ISRG Root X2 (self-signed, valid until 2040-09-17), the root of
// the chain served by sync.crosspointreader.com:
//   sync.crosspointreader.com -> Let's Encrypt YE2 -> ISRG Root YE -> ISRG Root X2
// Source: https://letsencrypt.org/certs/isrg-root-x2.pem
//   SHA-256 fingerprint: 69:72:9B:8E:15:A8:6E:FC:17:7A:57:AF:B7:17:1D:FC:64:AD:D2:8C:2F:CA:8C:F1:50:7E:34:45:3C:CB:14:70
// Renew before 2040, or when the sync server moves to a different CA.

namespace koreader {

inline constexpr char kSyncRootIsrgX2[] =
"-----BEGIN CERTIFICATE-----\n"
"MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw\n"
"CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg\n"
"R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00\n"
"MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT\n"
"ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw\n"
"EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW\n"
"+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9\n"
"ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T\n"
"AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI\n"
"zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW\n"
"tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1\n"
"/q4AaOeMSQ+2b1tbFfLn\n"
"-----END CERTIFICATE-----"
;

// Returns the pinned root PEM for a sync server URL, or nullptr to refuse it.
inline const char* trustRootFor(const std::string& url) {
  static constexpr char kHost[] = "sync.crosspointreader.com";
  static constexpr char kPrefix[] = "https://";
  const size_t prefixLen = sizeof(kPrefix) - 1;
  const size_t hostLen = sizeof(kHost) - 1;
  if (url.compare(0, prefixLen, kPrefix) != 0) return nullptr;
  if (url.compare(prefixLen, hostLen, kHost) != 0) return nullptr;
  const size_t after = prefixLen + hostLen;
  // The host must end here or at a path, port or query, so that
  // sync.crosspointreader.com.example.net does not match.
  if (after < url.size() && url[after] != '/' && url[after] != ':' && url[after] != '?') return nullptr;
  return kSyncRootIsrgX2;
}

}  // namespace koreader
