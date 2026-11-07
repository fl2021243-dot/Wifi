// ══════════════════════════════════════════════════════════
//  RK NILOY — Service Worker
//  Caches profile.png, music.mp3, fonts → instant repeat loads
// ══════════════════════════════════════════════════════════

const CACHE_NAME = 'rk-niloy-cache-v1';

// Files to pre-cache on install
const PRECACHE_URLS = [
    './',
    './index.html',
    './profile.png',
    './music.mp3'
];

// ── Install: pre-cache critical assets ──
self.addEventListener('install', (event) => {
    event.waitUntil(
        caches.open(CACHE_NAME)
            .then((cache) => {
                console.log('[SW] Pre-caching critical assets');
                return cache.addAll(PRECACHE_URLS);
            })
            .then(() => self.skipWaiting())
    );
});

// ── Activate: clean old caches ──
self.addEventListener('activate', (event) => {
    event.waitUntil(
        caches.keys().then((cacheNames) => {
            return Promise.all(
                cacheNames.map((cacheName) => {
                    if (cacheName !== CACHE_NAME) {
                        console.log('[SW] Deleting old cache:', cacheName);
                        return caches.delete(cacheName);
                    }
                })
            );
        }).then(() => self.clients.claim())
    );
});

// ── Fetch: cache-first for images/audio, network-first for HTML ──
self.addEventListener('fetch', (event) => {
    const request = event.request;
    const url = new URL(request.url);

    // Only handle same-origin requests
    if (url.origin !== location.origin) return;

    // Cache-first strategy for images & audio (fast!)
    if (/\.(png|jpg|jpeg|gif|webp|svg|mp3|m4a|wav|ogg)$/i.test(url.pathname)) {
        event.respondWith(
            caches.match(request).then((cachedResponse) => {
                if (cachedResponse) {
                    return cachedResponse; // instant from cache
                }
                return fetch(request).then((networkResponse) => {
                    // Cache the new fetch
                    if (networkResponse && networkResponse.status === 200) {
                        const responseClone = networkResponse.clone();
                        caches.open(CACHE_NAME).then((cache) => {
                            cache.put(request, responseClone);
                        });
                    }
                    return networkResponse;
                }).catch(() => {
                    // Offline fallback for images
                    return new Response('', { status: 404 });
                });
            })
        );
        return;
    }

    // Network-first for HTML (always fresh content)
    if (request.mode === 'navigate' || url.pathname.endsWith('.html')) {
        event.respondWith(
            fetch(request)
                .then((networkResponse) => {
                    const responseClone = networkResponse.clone();
                    caches.open(CACHE_NAME).then((cache) => {
                        cache.put(request, responseClone);
                    });
                    return networkResponse;
                })
                .catch(() => caches.match(request))
        );
        return;
    }
});