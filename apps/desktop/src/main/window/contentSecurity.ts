import type { Session } from 'electron';
export function installContentSecurityPolicy(session: Session, devOrigin?: string): void {
    const policy = devOrigin
        ? "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; connect-src 'self' ws:; font-src 'self'; img-src 'self' data:"
        : "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; font-src 'self'; img-src 'self' data:; connect-src 'none'; object-src 'none'; base-uri 'none'";
    session.webRequest.onHeadersReceived((details, callback) => callback({ responseHeaders: {
            ...details.responseHeaders, 'Content-Security-Policy': [policy],
        } }));
}
