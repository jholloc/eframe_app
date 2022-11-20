from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import sys


class CustomHTTPRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self) -> None:
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()


with ThreadingHTTPServer(('localhost', 8000), CustomHTTPRequestHandler) as server:
    CustomHTTPRequestHandler.protocol_version = "HTTP/1.0"
    host, port = server.socket.getsockname()[:2]
    url_host = f'[{host}]' if ':' in host else host
    print(
        f"Serving HTTP on {host} port {port} "
        f"(http://{url_host}:{port}/) ..."
    )
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nKeyboard interrupt received, exiting.")
        sys.exit(0)
