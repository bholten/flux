source ../src/dsl.tcl

GET https://www.google.com {
    headers {
	Content-Type: application/json
	Authorization: "Bearer whatever"
	User-Agent: flux
    }

    data {
	{
	    "test": "value"
	}
    }
}


puts $::___flux::main::requests
