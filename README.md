# Flux

A declarative HTTP API testing framework with a clean scripting DSL.

```lcl
Flux::Suite "User API" {
    Flux::Case "can fetch users" {
        GET https://api.example.com/users {
            header "Authorization: Bearer $token"

            on_response {
                assert_status 200
                assert_contains $body "users"
            }
        }
    }
}

Flux::run
```

## Features

- **Declarative syntax** - Tests read like documentation
- **All HTTP methods** - GET, POST, PUT, DELETE, PATCH, OPTIONS, HEAD
- **JSON support** - Automatic parsing, easy field access with `$body_json`
- **HTTP Signatures** - RFC 9421 support (HMAC, RSA-PSS, ECDSA)
- **SSE streaming** - Server-Sent Events with `on_event` callback
- **Reusable workflows** - Encapsulate complex API patterns in functions
- **State sharing** - Pass data between tests without touching the filesystem

## Installation

```bash
# Clone
git clone https://github.com/bholten/flux.git
cd flux

# Build
cmake -B build
cmake --build build

# Run a test
./build/flux examples/01_basic_get.lcl
```

## Quick Start

### Basic Request

```tcl
GET https://httpbin.org/get {
    on_response {
        assert_status 200
        puts "Response: $body"
    }
}
```

### POST with JSON

```tcl
POST https://httpbin.org/post {
    header "Content-Type: application/json"
    body {{"name": "Alice", "email": "alice@example.com"}}

    on_response {
        assert_status 200
        puts "Created user: [get $body_json data]"
    }
}
```

### Organized Test Suites

```tcl
Flux::Suite "Authentication" {
    Flux::Case "valid credentials return token" {
        POST https://api.example.com/login {
            body {{"email": "user@test.com", "password": "secret"}}

            on_response {
                assert_status 200
                assert_json_has "/token"
            }
        }
    }

    Flux::Case "invalid credentials return 401" {
        POST https://api.example.com/login {
            body {{"email": "user@test.com", "password": "wrong"}}

            on_response {
                assert_status 401
            }
        }
    }
}

Flux::run
```

## Request Options

```tcl
GET https://api.example.com/resource {
    ;; Headers
    header "Authorization: Bearer $token"
    header "Accept: application/json"

    ;; Or multiple headers at once
    headers {
        X-Custom-Header: value
        X-Another: value2
    }

    ;; Request body (for POST, PUT, PATCH)
    body {{"key": "value"}}

    ;; Timeouts
    timeout_ms 5000
    connection_timeout_ms 2000

    ;; Other options
    verbose 1              ;; Enable curl verbose output
    follow_redirects 1     ;; Follow HTTP redirects

    ;; Response handler
    on_response {
        ;; Available variables:
        ;; $status_code    - HTTP status code
        ;; $body           - Response body as string
        ;; $body_json      - Parsed JSON (if content-type is application/json)
        ;; $headers        - Response headers dict
        ;; $content_type   - Content-Type header value
        ;; $effective_url  - Final URL after redirects
        ;; $total_time     - Request duration in seconds
        ;; $is_timeout     - 1 if the request timed out
        ;; $error_code     - libcurl error code (0 if no transport error)
        ;; $error_message  - libcurl error string (empty if no transport error)
        ;; $ok             - 1 when $error_code == 0
        ;; $response       - The whole response dict
    }
}
```

## Assertions

A small vocabulary of bare-name macros that expand in the caller's frame
and read the `on_response` magic vars directly. You don't pass
`$status_code` / `$headers` / `$body` / `$body_json` — the macros pick
them up.

```tcl
on_response {
    ;; Status code
    assert_status 200
    assert_status_in (200 201 202)

    ;; Strings (against $body)
    assert_contains $body "success"
    assert_match "user-\\d+" $body              ;; POSIX regex

    ;; Headers (against $headers; case-insensitive)
    assert_header "Content-Type"
    assert_header_eq "Content-Type" "application/json"

    ;; JSON (against $body_json; RFC 6901 pointer)
    assert_json_has "/data/user/id"
    assert_json "/data/user/name" "Alice"

    ;; Transport-level OK (mainly useful for SSE — the regular request
    ;; path already throws on transport failure)
    assert_response_ok

    ;; Bare `assert` stays available for ad-hoc conditions
    assert [> [String::length $body] 0]
}
```

## Sharing State Between Tests

Each `Flux::Case` runs in an isolated def-target frame, so a bare `let` in a
case body stays local. The recommended way to share data across cases is a
workflow proc that internally memoizes — test code calls the proc and gets a
value back, without knowing whether the call hit the API or returned a
cached result.

```tcl
namespace auth {
    proc token {} {
        ;; Body runs once per key. Subsequent calls return the cached value.
        Flux::cache "auth:default" {
            var t {}
            POST https://api.example.com/login {
                body {{"email": "test@example.com", "password": "secret"}}
                on_response {
                    assert_status 200
                    set! t [get $body_json "token"]
                }
            }
            $t                ;; last expression IS the cached value
        }
    }
}

Flux::Suite "API Workflow" {
    Flux::Case "access protected resource" {
        let token [auth::token]

        GET https://api.example.com/profile {
            header "Authorization: Bearer $token"
            on_response { assert_status 200 }
        }
    }
}
```

Low-level primitives are available when the macro shape doesn't fit:

```tcl
Flux::cache_set "key" $value
let v [Flux::cache_get "key"]
if [Flux::cache_has? "key"] { ... }
Flux::cache_clear                  ;; drop all cached values
```

## Reusable Workflows

Pull workflow procs into a separate file when they're shared across suites:

```tcl
;; lib/workflows.lcl
namespace auth {
    proc login {email password} {
        Flux::cache "auth:$email" {
            var result #{}
            POST https://api.example.com/login {
                body [subst {{"email": "$email", "password": "$password"}}]
                on_response {
                    assert_status 200
                    set! result #{
                        token [get $body_json "token"]
                        user_id [get $body_json "user_id"]
                    }
                }
            }
            $result
        }
    }
}
```

Use it in tests:

```tcl
require lib/workflows.lcl

Flux::Suite "Dashboard" {
    Flux::Case "user can view dashboard" {
        let _auth [auth::login "user@example.com" "password"]

        GET https://api.example.com/dashboard {
            header "Authorization: Bearer [get $_auth token]"
            on_response { assert_status 200 }
        }
    }
}
```

## HTTP Signatures (RFC 9421)

Sign requests for APIs requiring cryptographic authentication:

```tcl
let secret "my-shared-secret"

POST https://api.example.com/signed-endpoint {
    header "Content-Type: application/json"
    body {{"action": "transfer", "amount": 100}}

    http_signature {
        key_id "my-key-id"
        key_pem $secret
        algorithm "hmac-sha256"
        cover [list "@method" "@authority" "@path" "content-type"]
    }

    on_response {
        assert_status 200
    }
}
```

Supported algorithms:
- `hmac-sha256` - Symmetric (shared secret)
- `rsa-pss-sha256`, `rsa-pss-sha512` - RSA private key
- `ecdsa-p256-sha256`, `ecdsa-p384-sha384` - EC private key

## SSE Streaming

Handle Server-Sent Events:

```tcl
var event_count 0

GET https://sse.example.com/stream {
    timeout_ms 5000

    on_event {
        ;; Available: $data, $event_type, $id, $retry
        set! event_count [+ $event_count 1]
        puts "Event $event_count: $data"
    }

    on_response {
        puts "Stream ended after $event_count events"
    }
}
```

## Configuration

```tcl
;; Stop on first failure (default: 1)
Flux::configure stop_on_failure 0

;; Run all collected tests
Flux::run
```

## Running the Tests

The `test/` directory holds the deterministic test suite — `.lcl` files
that run against the bundled `docker-compose.yaml` echo server. They're
distinct from `examples/`, which exercise real-world endpoints
(httpbin, Wikipedia, etc.) and exist primarily as documentation.

```bash
docker compose up -d                    # start ealen/echo-server on :8080
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure -j 8
docker compose down                     # tear down when finished
```

Each `test/*.lcl` becomes one `ctest` case (`flux_<name>`); ctest runs
them in parallel under `-j N`. CI (GitHub Actions, `.github/workflows/ci.yml`)
does the same on every push/PR.

## Examples

See the `examples/` directory:

| Example | Description |
|---------|-------------|
| `01_basic_get.lcl` | Simple GET request |
| `02_headers.lcl` | Custom headers |
| `03_post_json.lcl` | POST with JSON body |
| `05_json_response.lcl` | JSON parsing and access |
| `08_assertions.lcl` | All assertion types |
| `10_http_signatures.lcl` | RFC 9421 signatures |
| `11_sse_streaming.lcl` | Server-Sent Events |
| `12_workflow_cache.lcl` | Workflow memoization with `Flux::cache` |
| `13_reusable_workflows.lcl` | Workflow libraries |

## LCL Language Basics

Flux uses LCL (Lexical Command Language), a Tcl-inspired language with lexical scoping:

```tcl
;; Comments start with ;;

;; Variables
let name "Alice"              ;; immutable
var counter 0                 ;; mutable
set! counter [+ $counter 1]   ;; mutation

;; Commands use brackets
let result [String::upper $name]

;; Lists and dicts
let items (a b c)
let user #{name "Alice" age 30}

;; Access
let first [get $items 0]
let user_name [get $user name]

;; Control flow
if [> $x 0] {
    puts "positive"
} else {
    puts "non-positive"
}

;; Functions
proc greet {name} {
    puts "Hello, $name!"
}
```

For more information, see: https://github.com/bholten/lcl

## License

MIT
