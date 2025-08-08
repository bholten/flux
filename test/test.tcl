workspace test

GET https://www.google.com {
    headers {
	Content-Type: application/json
	Authorization: "Bearer <token>"
	User-Agent: flux
    }

    data {
	{
	    "test": "value"
	}
    }
}
