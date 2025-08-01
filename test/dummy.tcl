puts "Hello"

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


puts "Hello2"

puts $::flux::requests

