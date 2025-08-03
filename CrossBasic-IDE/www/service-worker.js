/* Invoke an application update on all clients upon next load/reload by increasing/changing the cacheName below. */
var cacheName = '1.2.0';

/* 1. Save the files to the user's device
   The "install" event is called when the ServiceWorker starts up.
   All ServiceWorker code must be inside events. */

self.addEventListener('install', function(e) {
  console.log('install');

  /* waitUntil tells the browser that the install event is not finished until we have
   cached all of our files */
  e.waitUntil(
    /* Here we call our cache... */
    caches.open(cacheName).then(cache => {
      /* If the request for any of these resources fails, _none_ of the resources will be
         added to the cache. */
      return cache.addAll([
        'index.html'
      ]);
    })
  );
});

/* 2. Intercept requests and return the cached version instead for faster loading */
self.addEventListener('fetch', function(e) {
  e.respondWith(
    /* check if this file exists in the cache */
    caches.match(e.request)
      /* Return the cached file, or else try to get it from the server */
      .then(response => response || fetch(e.request))
  );
});
