// source: https://developer.mozilla.org/en-US/docs/Web/Progressive_web_apps/Guides/Caching
// Any copyright is dedicated to the Public Domain: https://creativecommons.org/publicdomain/zero/1.0/

const cache_name = "openvocs_v1"

async function network_first(request) {
    try {
        const network_response = await fetch(request);
        if (network_response.ok) {
            const cache = await caches.open(cache_name);
            cache.put(request, network_response.clone());
        }
        return network_response;
    } catch (error) {
        const cached_response = await caches.match(request);
        return cached_response || Response.error();
    }
}

async function cache_first(request) {
    const cached_response = await caches.match(request);
    if (cached_response) {
        return cached_response;
    }
    try {
        const network_response = await fetch(request);
        if (network_response.ok) {
            const cache = await caches.open(cache_name);
            cache.put(request, network_response.clone());
        }
        return network_response;
    } catch (error) {
        return Response.error();
    }
}

async function cache_first_with_refresh(request) {
    const network_response_promise = fetch(request).then(async (network_response) => {
        if (network_response.ok) {
            const cache = await caches.open(cache_name);
            cache.put(request, network_response.clone());
        }
        return network_response;
    });

    const cached_response = await caches.match(request);
    if (cached_response)
        return cached_response;
    return await network_response_promise;
}

self.addEventListener("fetch", (event) => {
    event.respondWith(network_first(event.request));
});

self.addEventListener("activate", (event) => {
    event.waitUntil(
        caches.keys().then((cache_names) => {
            return Promise.all(
                cache_names.map((name) => {
                    if (name !== cache_name)
                        return caches.delete(name);
                }),
            );
        }),
    );
});