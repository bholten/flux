workspace local_test

POST http://localhost:3000/local123 {
    headers {
	User-Agent: flux
    }

    data {
	{
	    "test": 123,
	    "Another field": [1, 2, 3]
	}
    }
}

