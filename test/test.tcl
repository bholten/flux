workspace test

GET https://www.google.com {
    headers {
	Content-Type: text/html
	User-Agent: flux
    }
}

GET https://jsonplaceholder.typicode.com/posts/1 {
    headers {
	Accept-Encoding: gzip
	User-Agent: flux
    }
}

GET https://jsonplaceholder.typicode.com/posts/2 {
    headers {
	Accept-Encoding: gzip
	User-Agent: flux
    }
}

